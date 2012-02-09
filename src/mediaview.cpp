#include "mediaview.h"
#include "model/track.h"
#include "model/artist.h"
#include "../mainwindow.h"
#include "droparea.h"
#include "minisplitter.h"
#include "constants.h"
#include "lastfm.h"

namespace The {
QMap<QString, QAction*>* globalActions();
QMap<QString, QMenu*>* globalMenus();
}

MediaView::MediaView(QWidget *parent) : QWidget(parent) {

    activeTrack = 0;

#ifdef APP_DEMO
    tracksPlayed = 0;
#endif

    QBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);

    splitter = new MiniSplitter(this);
    splitter->setChildrenCollapsible(false);

    // playlist model
    playlistModel = new PlaylistModel(this);
    connect(playlistModel, SIGNAL(activeRowChanged(int, bool)), SLOT(activeRowChanged(int, bool)));
    connect(playlistModel, SIGNAL(playlistFinished()), SLOT(playlistFinished()));

    // finder
    finderWidget = new FinderWidget(this);
    finderWidget->setPlaylistModel(playlistModel);
    splitter->addWidget(finderWidget);

    // drop area
    dropArea = new DropArea(this);
    dropArea->setPlaylistModel(playlistModel);
    dropArea->hide();

    // playlist view
    playlistView = new PlaylistView(this);
    playlistView->setEmptyPlaylistMessage(tr("Drop items here"));
    playlistView->setDropArea(dropArea);
    playlistView->setPlaylistModel(playlistModel);
    // connect(playlistView, SIGNAL(needDropArea()), SLOT(showDropArea()));

    // playlist widget, handles the playlist/droparea layout
    PlaylistArea *playlistWidget = new PlaylistArea(playlistView, dropArea, this);
    splitter->addWidget(playlistWidget);

    finderWidget->setPlaylistView(playlistView);

    splitter->setStretchFactor(0, 6);
    splitter->setStretchFactor(1, 1);

    // restore splitter state
    QSettings settings;
    splitter->restoreState(settings.value("splitter").toByteArray());

    layout->addWidget(splitter);
    setLayout(layout);

    splitter->setHandleWidth(1);

    errorTimer = new QTimer(this);
    errorTimer->setSingleShot(true);
    errorTimer->setInterval(3000);
    connect(errorTimer, SIGNAL(timeout()), playlistModel, SLOT(skipForward()));

}

void MediaView::setMediaObject(Phonon::MediaObject *mediaObject) {
    this->mediaObject = mediaObject;
    // connect(mediaObject, SIGNAL(aboutToFinish()), this, SLOT(aboutToFinish()));
    connect(mediaObject, SIGNAL(finished()), SLOT(playbackFinished()));
    connect(mediaObject, SIGNAL(stateChanged(Phonon::State, Phonon::State)),
            SLOT(stateChanged(Phonon::State, Phonon::State)));
    //connect(mediaObject, SIGNAL(currentSourceChanged(Phonon::MediaSource)),
    // this, SLOT(currentSourceChanged(Phonon::MediaSource)));
}

void MediaView::saveSplitterState() {
    QSettings settings;
    settings.setValue("splitter", splitter->saveState());
}

void MediaView::stateChanged(Phonon::State newState, Phonon::State /*oldState*/) {

    // qDebug() << "Phonon state: " << newState << oldState;

    switch (newState) {

    case Phonon::ErrorState:
        qDebug() << "Phonon error:" << mediaObject->errorString() << mediaObject->errorType();
        handleError(mediaObject->errorString());
        break;

    case Phonon::PlayingState:
        //qDebug("playing");
        break;

    case Phonon::StoppedState:
        // reset window title
        window()->setWindowTitle(Constants::NAME);
        break;

    case Phonon::PausedState:
        //qDebug("paused");
        break;

    case Phonon::BufferingState:
        //qDebug("buffering");
        break;

    case Phonon::LoadingState:
        //qDebug("loading");
        break;

    }
}

void MediaView::activeRowChanged(int row, bool manual) {

    errorTimer->stop();

    Track *track = playlistModel->trackAt(row);
    if (!track) return;

    connect(track, SIGNAL(removed()), SLOT(trackRemoved()));
    activeTrack = track;

    // go!
    QString path = track->getAbsolutePath();
    // qDebug() << "Playing" << path;
    mediaObject->setCurrentSource(path);
    mediaObject->play();

    track->setStartTime(QDateTime::currentDateTime().toTime_t());

    // ensure active item is visible
    QModelIndex index = playlistModel->indexForTrack(track);
    if (manual)
        playlistView->scrollTo(index, QAbstractItemView::EnsureVisible);
    else
        playlistView->scrollTo(index, QAbstractItemView::PositionAtCenter);

    // update info view
    MainWindow* mainWindow = dynamic_cast<MainWindow*>(window());
    if (mainWindow) {
        mainWindow->updateContextualView(track);
    }

    // track title as window title
    Artist *artist = track->getArtist();
    QString windowTitle = track->getTitle();
    if (artist) {
        windowTitle += " - " + artist->getName();
    }
    window()->setWindowTitle(windowTitle);

    // enable/disable actions
    The::globalActions()->value("stopafterthis")->setEnabled(true);

    // scrobbling
    QSettings settings;
    if (settings.value("scrobbling").toBool() &&
            LastFm::instance().isAuthorized()) {
        LastFm::instance().nowPlaying(track);
    }

}

void MediaView::handleError(QString message) {
    // recover from errors by skipping to the next track
    errorTimer->start();

    MainWindow* mainWindow = dynamic_cast<MainWindow*>(window());
    if (mainWindow) mainWindow->statusBar()->showMessage(message);
}

void MediaView::appear() {
    finderWidget->appear();
}

void MediaView::playPause() {
    // qDebug() << "playPause() state" << mediaObject->state();

    int state = mediaObject->state();
    if (state == Phonon::PlayingState) {
        // qDebug() << "Playing, let's pause";
        mediaObject->pause();
    } else if (state == Phonon::PausedState) {
        // qDebug() << "Paused, let's play";
        mediaObject->play();
    } else {
        // we're currently not playing, let's rock
        // qDebug() << "Not playing";
        playlistModel->skipForward();
    }

}

void MediaView::trackRemoved() {
    // get the Track that sent the signal
    Track *track = static_cast<Track*>(sender());
    if (!track) {
        qDebug() << "MediaView::trackRemoved()" << "Cannot get sender track";
        return;
    }
    if (track == activeTrack)
        activeTrack = 0;
}

void MediaView::playlistFinished() {
    MainWindow* mainWindow = dynamic_cast<MainWindow*>(window());
    if (mainWindow) mainWindow->statusBar()->showMessage(tr("Playlist finished"));
}

void MediaView::playbackFinished() {
#ifdef APP_DEMO
    if (tracksPlayed > 1) demoMessage();
    else tracksPlayed++;
#endif

    // scrobbling
    bool needScrobble = false;
    Track *track = 0;
    QSettings settings;
    if (settings.value("scrobbling").toBool() &&
            LastFm::instance().isAuthorized()) {
        track = playlistModel->getActiveTrack();
        needScrobble = true;
    }

    QAction* stopAfterThisAction = The::globalActions()->value("stopafterthis");
    if (stopAfterThisAction->isChecked()) {
        stopAfterThisAction->setChecked(false);
    } else playlistModel->skipForward();

    if (needScrobble && track) LastFm::instance().scrobble(track);

}

#ifdef APP_DEMO

static QPushButton *continueButton;

void MediaView::demoMessage() {
    mediaObject->pause();

    QMessageBox msgBox(this);
    msgBox.setIconPixmap(QPixmap(":/data/app.png").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    msgBox.setText(tr("This is just the demo version of %1.").arg(Constants::NAME));
    msgBox.setInformativeText(tr("It allows you to play a few tracks so you can test the application and see if it works for you."));
    msgBox.setModal(true);
    // make it a "sheet" on the Mac
    msgBox.setWindowModality(Qt::WindowModal);

    continueButton = msgBox.addButton("5", QMessageBox::RejectRole);
    continueButton->setEnabled(false);
    QPushButton *buyButton = msgBox.addButton(tr("Get the full version"), QMessageBox::ActionRole);

    QTimeLine *timeLine = new QTimeLine(6000, this);
    timeLine->setCurveShape(QTimeLine::LinearCurve);
    timeLine->setFrameRange(5, 0);
    connect(timeLine, SIGNAL(frameChanged(int)), SLOT(updateContinueButton(int)));
    timeLine->start();

    msgBox.exec();

    if (msgBox.clickedButton() == buyButton) {
        QDesktopServices::openUrl(QUrl(QString(Constants::WEBSITE) + "#download"));
    }

    tracksPlayed = 0;

    delete timeLine;

}

void MediaView::updateContinueButton(int value) {
    if (value == 0) {
        continueButton->setText(tr("Continue"));
        continueButton->setEnabled(true);
    } else {
        continueButton->setText(QString::number(value));
    }
}

#endif

void MediaView::search(QString query) {
    finderWidget->showSearch(query);
}
