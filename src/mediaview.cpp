#include "mediaview.h"
#include "model/track.h"
#include "model/artist.h"
#include "../mainwindow.h"
#include "droparea.h"
#include "minisplitter.h"
#include "constants.h"

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
    playlistView->setDropArea(dropArea);
    playlistView->setPlaylistModel(playlistModel);
    // connect(playlistView, SIGNAL(needDropArea()), SLOT(showDropArea()));

    // playlist widget, handles the playlist/droparea layout
    PlaylistWidget *playlistWidget = new PlaylistWidget(playlistView, dropArea, this);
    splitter->addWidget(playlistWidget);

    finderWidget->setPlaylistView(playlistView);

    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

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
    connect(mediaObject, SIGNAL(finished()), playlistModel, SLOT(skipForward()));
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
        window()->setWindowTitle(Constants::APP_NAME);
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

#ifdef APP_DEMO
    if (tracksPlayed > 8) demoExpired();
    else tracksPlayed++;
#endif

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

#ifdef APP_DEMO
void MediaView::demoExpired() {
    mediaObject->pause();

    QMessageBox msgBox;
    msgBox.setIconPixmap(QPixmap(":/images/app.png").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    msgBox.setText(tr("This is just the demo version of %1.").arg(Constants::APP_NAME) + " " +
                   tr("It allows you to play a few tracks so you can test the application and see if it works for you.")
                   );
    msgBox.setModal(true);

    QPushButton *quitButton = msgBox.addButton(tr("Continue"), QMessageBox::RejectRole);
    QPushButton *buyButton = msgBox.addButton(tr("Get the full version"), QMessageBox::ActionRole);

    msgBox.exec();

    if (msgBox.clickedButton() == buyButton) {
        QDesktopServices::openUrl(QString(Constants::WEBSITE) + "#download");
    }

    tracksPlayed = 4;
}
#endif
