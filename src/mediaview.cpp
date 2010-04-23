#include "mediaview.h"
#include "model/track.h"
#include "model/artist.h"
#include "../mainwindow.h"

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

    // playlistWidget = new PlaylistWidget(playlistView);

    splitter->addWidget(playlistView);

    // restore splitter state
    QSettings settings;
    splitter->restoreState(settings.value("splitter").toByteArray());

    layout->addWidget(splitter);
    setLayout(layout);

    errorTimer = new QTimer(this);
    errorTimer->setSingleShot(true);
    errorTimer->setInterval(3000);
    connect(errorTimer, SIGNAL(timeout()), playlistView, SLOT(skipForward()));

}

void MediaView::setMediaObject(Phonon::MediaObject *mediaObject) {
    this->mediaObject = mediaObject;
    // Phonon::createPath(this->mediaObject, videoWidget);
    // connect(mediaObject, SIGNAL(aboutToFinish()), this, SLOT(aboutToFinish()));
    connect(mediaObject, SIGNAL(finished()), playlistView, SLOT(skipForward()));
    connect(mediaObject, SIGNAL(stateChanged(Phonon::State, Phonon::State)),
            this, SLOT(stateChanged(Phonon::State, Phonon::State)));
    //connect(mediaObject, SIGNAL(currentSourceChanged(Phonon::MediaSource)),
    // this, SLOT(currentSourceChanged(Phonon::MediaSource)));
    // connect(mediaObject, SIGNAL(bufferStatus(int)), loadingWidget, SLOT(bufferStatus(int)));
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
    qDebug() << path;
    // Phonon::MediaSource source(path);
    mediaObject->setCurrentSource(path);
    mediaObject->play();

    // ensure active item is visible
    QModelIndex index = playlistModel->index(row, 0, QModelIndex());
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
    }

}

void MediaView::handleError(QString message) {
    // TODO videoAreaWidget->showError(message);
    // recover from errors by skipping to the next video
    errorTimer->start();
}

void MediaView::appear() {
    finderWidget->appear();
}
