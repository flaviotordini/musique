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

#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include <QtGui>

class PlaylistModel;
class Track;
class DropArea;

class PlaylistView : public QListView {

    Q_OBJECT

public:
    PlaylistView(QWidget *parent);
    void setPlaylistModel(PlaylistModel *model);
    void setDropArea(DropArea *dropArea) {
        this->dropArea = dropArea;
    }
    void setEmptyPlaylistMessage(QString emptyMessage) {
        this->emptyMessage = emptyMessage;
    }

signals:
    void needDropArea();

public slots:
    void removeSelected();
    void moveUpSelected();
    void moveDownSelected();
    void itemActivated(const QModelIndex &index);
    void selectionChanged (const QItemSelection & selected, const QItemSelection & deselected);
    void updatePlaylistActions();
    void selectTracks(QList<Track*> tracks);

protected:
    void paintEvent(QPaintEvent *event);
    /*
    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    */

private:
    PlaylistModel *playlistModel;
    DropArea *dropArea;
    bool willHideDropArea;
    QString emptyMessage;
    QLabel* overlayLabel;

};

#endif // PLAYLISTVIEW_H
