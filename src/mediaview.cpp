#include "mediaview.h"
#include "model/track.h"
#include "model/artist.h"
#include "../mainwindow.h"
#include "droparea.h"
#include "minisplitter.h"

namespace The {
    QMap<QString, QAction*>* globalActions();
    QMap<QString, QMenu*>* globalMenus();
}

MediaView::MediaView(QWidget *parent) : QWidget(parent) {

    QBoxLayout *layout = new QHBoxLayout();
    layout->setMargin(0);

    splitter = new QSplitter(this);
    splitter->setChildrenCollapsible(false);

    // playlist model
    playlistModel = new PlaylistModel(this);
    connect(playlistModel, SIGNAL(activeRowChanged(int)), SLOT(activeRowChanged(int)));

    // finder
    finderWidget = new FinderWidget(this);
    finderWidget->setPlaylistModel(playlistModel);
    splitter->addWidget(finderWidget);

    // playlist view
    playlistView = new PlaylistView(this);
    playlistView->setPlaylistModel(playlistModel);

    // drop area
    dropArea = new DropArea(this);
    dropArea->hide();

    // playlist widget, handles the playlist/droparea layout
    PlaylistWidget *playlistWidget = new PlaylistWidget(playlistView, dropArea, this);
    splitter->addWidget(playlistWidget);

    finderWidget->setPlaylistView(playlistView);

    // restore splitter state
    QSettings settings;
    splitter->restoreState(settings.value("splitter").toByteArray());

    layout->addWidget(splitter);
    setLayout(layout);

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
        // videoAreaWidget->showVideo();
        break;

    case Phonon::StoppedState:
        //qDebug("stopped");
        // play() has already been called when setting the source
        // but Phonon on Linux needs a little more help to start playback
        // mediaObject->play();
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

void MediaView::activeRowChanged(int row) {

    errorTimer->stop();

    Track *track = playlistModel->trackAt(row);
    if (!track) return;

    // go!
    QString path = track->getAbsolutePath();
    // path = "file:/" + QUrl(path).toEncoded();
    qDebug() << "Playing" << path;
    // Phonon::MediaSource source(path);
    mediaObject->setCurrentSource(path);
    mediaObject->play();

    // ensure active item is visible
    QModelIndex index = playlistModel->indexForTrack(track);
    playlistView->scrollTo(index, QAbstractItemView::EnsureVisible);

    // track title as window title
    MainWindow* mainWindow = dynamic_cast<MainWindow*>(window());
    if (mainWindow) {
        Artist *artist = track->getArtist();
        QString windowTitle = track->getTitle();
        if (artist) {
            windowTitle += " - " + artist->getName();
        }
        mainWindow->setWindowTitle(windowTitle);

        // update info view
        mainWindow->updateContextualView(track);
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
    qDebug() << "playPause";
    // qDebug() << "pause() called" << mediaObject->state();
    switch( mediaObject->state() ) {
    case Phonon::StoppedState:
        qDebug() << "try";
        playlistModel->skipForward();
    case Phonon::PlayingState:
        mediaObject->pause();
        break;
    default:
        mediaObject->play();
        break;
    }
}
