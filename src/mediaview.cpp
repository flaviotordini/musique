/* $BEGIN_LICENSE

This file is part of Musique.
Copyright 2013, Flavio Tordini <flavio.tordini@gmail.com>

Musique is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Musique is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Musique.  If not, see <http://www.gnu.org/licenses/>.

$END_LICENSE */

#include "mediaview.h"
#include "constants.h"
#include "droparea.h"
#include "iconutils.h"
#include "lastfm.h"
#include "mainwindow.h"
#include "minisplitter.h"
#include "model/album.h"
#include "model/artist.h"
#include "model/track.h"
#ifdef APP_ACTIVATION
#include "activation.h"
#endif
#include "database.h"
#ifdef APP_EXTRA
#include "extra.h"
#endif

#include "idle.h"

MediaView::MediaView(QWidget *parent) : View(parent) {
    activeTrack = nullptr;

#ifdef APP_ACTIVATION
    tracksPlayed = 0;
#endif

    QBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);

    splitter = new MiniSplitter(this);
    splitter->setChildrenCollapsible(false);

    // playlist model
    playlistModel = new PlaylistModel(this);
    connect(playlistModel, SIGNAL(activeRowChanged(int, bool, bool)),
            SLOT(activeRowChanged(int, bool, bool)));
    connect(playlistModel, SIGNAL(playlistFinished()), SLOT(playlistFinished()));

    // finder
    finderWidget = new FinderWidget(this);
    finderWidget->setPlaylistModel(playlistModel);
    splitter->addWidget(finderWidget);

    // drop area
    /*
    dropArea = new DropArea(this);
    dropArea->setPlaylistModel(playlistModel);
    dropArea->hide();
    */

    // playlist view
    playlistView = new PlaylistView(this);
    playlistView->setEmptyPlaylistMessage(tr("Drop items here"));
    // playlistView->setDropArea(dropArea);
    playlistView->setPlaylistModel(playlistModel);
    // connect(playlistView, SIGNAL(needDropArea()), SLOT(showDropArea()));

    // playlist widget, handles the playlist/droparea layout
    PlaylistArea *playlistWidget = new PlaylistArea(playlistView, nullptr, this);
    splitter->addWidget(playlistWidget);

    finderWidget->setPlaylistView(playlistView);

    splitter->setStretchFactor(0, 8);
    splitter->setStretchFactor(1, 0);

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

void MediaView::setMedia(Media *media) {
    this->media = media;
    connect(media, &Media::finished, this, &MediaView::playbackFinished);
    connect(media, &Media::stateChanged, this, &MediaView::stateChanged);
    connect(media, &Media::aboutToFinish, this, &MediaView::aboutToFinish);
    connect(media, &Media::sourceChanged, this, &MediaView::currentSourceChanged);
}

void MediaView::saveSplitterState() {
    QSettings settings;
    settings.setValue("splitter", splitter->saveState());
}

void MediaView::stateChanged(Media::State newState) {
    // qDebug() << "Phonon state: " << newState << oldState;

    switch (newState) {
    /*
    case MM::ErrorState:
        qDebug() << "Phonon error:" << mediaObject->errorString() << mediaObject->errorType();
        handleError(mediaObject->errorString());
        break;
    */
    case Media::PlayingState:
        // qDebug("playing");
        break;

    case Media::StoppedState:
        // reset window title
        window()->setWindowTitle(Constants::NAME);
        break;

    case Media::PausedState:
        // qDebug("paused");
        break;

    case Media::BufferingState:
        // qDebug("buffering");
        break;

    case Media::LoadingState:
        // qDebug("loading");
        break;
    }

    if (newState == Media::PlayingState) {
        bool res = Idle::preventDisplaySleep(QString("%1 is playing").arg(Constants::NAME));
        if (!res) qWarning() << "Error disabling idle display sleep" << Idle::displayErrorMessage();
    } else {
        bool res = Idle::allowDisplaySleep();
        if (!res) qWarning() << "Error enabling idle display sleep" << Idle::displayErrorMessage();
    }
}

void MediaView::activeRowChanged(int row, bool manual, bool startPlayback) {
    errorTimer->stop();

    Track *track = playlistModel->trackAt(row);
    if (!track) {
        activeTrack = nullptr;
        MainWindow::instance()->getAction("contextual")->setEnabled(false);
        return;
    }

    connect(track, SIGNAL(removed()), SLOT(trackRemoved()));
    activeTrack = track;
    MainWindow::instance()->getAction("contextual")->setEnabled(true);

    // go!
    if (startPlayback) {
        if (manual) media->clearQueue();
        QString path = track->getAbsolutePath();
        qDebug() << "Playing" << path;
        media->play(path);
    }

    track->setStartTime(QDateTime::currentDateTimeUtc().toTime_t());

    // ensure active item is visible
    QModelIndex index = playlistModel->indexForTrack(track);
    if (manual)
        playlistView->scrollTo(index, QAbstractItemView::EnsureVisible);
    else
        playlistView->scrollTo(index, QAbstractItemView::PositionAtCenter);

    // update info view
    MainWindow::instance()->updateContextualView(track);

    // track title as window title
    Artist *artist = track->getArtist();
    QString windowTitle = track->getTitle();
    if (artist) {
        windowTitle += " - " + artist->getName();
    }
    window()->setWindowTitle(windowTitle);

    // enable/disable actions
    MainWindow::instance()->getAction("stopafterthis")->setEnabled(true);

#ifdef APP_EXTRA
    QString artistName = track->getArtist() ? track->getArtist()->getName() : "";
    QString albumName = track->getAlbum() ? track->getAlbum()->getName() : "";
    Extra::notify(track->getTitle(), artistName, albumName);
#endif

    // scrobbling
    QSettings settings;
    if (settings.value("scrobbling").toBool() && LastFm::instance().isAuthorized()) {
        LastFm::instance().nowPlaying(track);
    }
}

void MediaView::handleError(QString message) {
    // recover from errors by skipping to the next track
    errorTimer->start();
    MainWindow::instance()->showMessage(message);
}

void MediaView::appear() {
    finderWidget->appear();
}

void MediaView::disappear() {
    finderWidget->disappear();
}

void MediaView::playPause() {
    // qDebug() << "playPause() state" << mediaObject->state();

    auto state = media->state();
    if (state == Media::PlayingState) {
        qDebug() << "Playing, let's pause";
        media->pause();
    } else if (state == Media::PausedState) {
        qDebug() << "Paused, let's play";
        media->play();
    } else {
        // we're currently not playing, let's rock
        qDebug() << "Not playing";
        playlistModel->skipForward();
    }
}

void MediaView::trackRemoved() {
    // get the Track that sent the signal
    Track *track = static_cast<Track *>(sender());
    if (!track) {
        qDebug() << "MediaView::trackRemoved()"
                 << "Cannot get sender track";
        return;
    }
    if (track == activeTrack) activeTrack = nullptr;
}

void MediaView::playlistFinished() {
    MainWindow::instance()->showMessage(tr("Playlist finished"));

    QAction *a = MainWindow::instance()->getAction("contextual");
    if (a->isChecked()) MainWindow::instance()->hideContextualView();
    a->setEnabled(false);
}

void MediaView::playbackFinished() {
    trackFinished();
    QAction *stopAfterThisAction = MainWindow::instance()->getAction("stopafterthis");
    if (stopAfterThisAction->isChecked()) {
        stopAfterThisAction->setChecked(false);
    } else if (!media->hasQueue()) {
        qDebug() << "Empty queue. Manually skipping forward";
        playlistModel->skipForward();
    }
}

void MediaView::trackFinished() {
#ifdef APP_ACTIVATION
    if (!Activation::instance().isActivated()) {
        if (tracksPlayed > 1)
            demoMessage();
        else
            tracksPlayed++;
    }
#endif

    // scrobbling
    bool needScrobble = false;
    Track *track = nullptr;
    QSettings settings;
    if (settings.value("scrobbling").toBool() && LastFm::instance().isAuthorized()) {
        track = playlistModel->getActiveTrack();
        needScrobble = true;
    }

    if (needScrobble && track) LastFm::instance().scrobble(track);
}

void MediaView::aboutToFinish() {
    QAction *stopAfterThisAction = MainWindow::instance()->getAction("stopafterthis");
    if (!stopAfterThisAction->isChecked()) {
        Track *nextTrack = playlistModel->getNextTrack();
        if (nextTrack) {
            QString absolutePath = nextTrack->getAbsolutePath();
            qDebug() << "Enqueuing" << absolutePath;
            media->enqueue(absolutePath);
        }
    }
}

void MediaView::currentSourceChanged() {
    QString path = media->file();
    qDebug() << path;
    QString collectionRoot = Database::instance().collectionRoot();
    path = path.mid(collectionRoot.length() + 1);
    Track *track = Track::forPath(path);
    if (track) {
        int row = playlistModel->rowForTrack(track);
        if (row >= 0) playlistModel->setActiveRow(row, false, false);
    }
}

#ifdef APP_ACTIVATION

static QPushButton *continueButton;

void MediaView::demoMessage() {
    mediaObject->pause();

    QMessageBox msgBox(this);
    msgBox.setIconPixmap(IconUtils::pixmap(":/data/app.png")
                                 .scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    msgBox.setText(tr("This is just the demo version of %1.").arg(Constants::NAME));
    msgBox.setInformativeText(tr("It allows you to play a few tracks so you can test the "
                                 "application and see if it works for you."));
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
    } else
        mediaObject->play();

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
