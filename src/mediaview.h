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

#ifndef MEDIAVIEW_H
#define MEDIAVIEW_H

#include <QtGui>
#include <phonon/mediaobject.h>

#include "view.h"
#include "finderwidget.h"
#include "playlistview.h"
#include "playlistmodel.h"
#include "playlistwidget.h"

class DropArea;

class MediaView : public QWidget, public View {

    Q_OBJECT

public:
    MediaView(QWidget *parent);
    void saveSplitterState();
    void setMediaObject(Phonon::MediaObject *mediaObject);
    Track* getActiveTrack() { return activeTrack; }
    PlaylistModel* getPlaylistModel() { return playlistModel; }

public slots:
    void appear();
    void disappear();
    void playPause();
    void trackRemoved();
    void search(QString query);

private slots:
    void activeRowChanged(int row, bool manual, bool startPlayback);
    void stateChanged(Phonon::State newState, Phonon::State oldState);
    void handleError(QString message);
    void playlistFinished();
    void playbackFinished();
    void trackFinished();
    void aboutToFinish();
    void currentSourceChanged(Phonon::MediaSource mediaSource);
#ifdef APP_ACTIVATION
    void updateContinueButton(int);
#endif

private:
    QSplitter *splitter;
    Phonon::MediaObject *mediaObject;
    FinderWidget *finderWidget;
    PlaylistView *playlistView;
    PlaylistModel *playlistModel;
    PlaylistArea *playlistWidget;
    QTimer *errorTimer;
    DropArea *dropArea;
    Track *activeTrack;

#ifdef APP_ACTIVATION
    void demoMessage();
    int tracksPlayed;
#endif

};

#endif // MEDIAVIEW_H
