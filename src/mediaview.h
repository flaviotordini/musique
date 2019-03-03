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

#include <QtWidgets>

#include "media.h"

#include "finderwidget.h"
#include "playlistmodel.h"
#include "playlistview.h"
#include "playlistwidget.h"
#include "view.h"

class DropArea;

class MediaView : public View {
    Q_OBJECT

public:
    MediaView(QWidget *parent);
    void saveSplitterState();
    void setMedia(Media *media);
    Track *getActiveTrack() { return activeTrack; }
    PlaylistModel *getPlaylistModel() { return playlistModel; }

public slots:
    void appear();
    void disappear();
    void playPause();
    void trackRemoved();
    void search(QString query);

private slots:
    void activeRowChanged(int row, bool manual, bool startPlayback);
    void stateChanged(Media::State newState);
    void handleError(QString message);
    void playlistFinished();
    void playbackFinished();
    void trackFinished();
    void aboutToFinish();
    void currentSourceChanged();

private:
    QSplitter *splitter;
    Media *media;
    FinderWidget *finderWidget;
    PlaylistView *playlistView;
    PlaylistModel *playlistModel;
    PlaylistArea *playlistWidget;
    QTimer *errorTimer;
    DropArea *dropArea;
    Track *activeTrack;
};

#endif // MEDIAVIEW_H
