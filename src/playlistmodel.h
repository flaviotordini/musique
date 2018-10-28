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

#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include "model/track.h"
#include <QtWidgets>

namespace Playlist {

enum PlaylistDataRoles { DataObjectRole = Qt::UserRole, ActiveItemRole, HoveredItemRole };

}

class PlaylistModel : public QAbstractListModel {
    Q_OBJECT

public:
    PlaylistModel(QWidget *parent);

    // inherited from QAbstractListModel
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool removeRows(int position, int rows, const QModelIndex &parent);

    // drag and drop
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QStringList mimeTypes() const;
    Qt::DropActions supportedDropActions() const;
    Qt::DropActions supportedDragActions() const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    bool dropMimeData(const QMimeData *data,
                      Qt::DropAction action,
                      int row,
                      int column,
                      const QModelIndex &parent);

    // custom methods
    void setActiveRow(int row, bool manual = false, bool startPlayback = true);
    bool rowExists(int row) const { return ((row >= 0) && (row < tracks.size())); }
    void removeIndexes(const QModelIndexList &indexes);
    int rowForTrack(Track *track);
    QModelIndex indexForTrack(Track *track);
    void move(const QModelIndexList &indexes, bool up);

    Track *trackAt(int row) const;
    Track *getActiveTrack() const;
    int getTotalLength() { return Track::getTotalLength(tracks); }

    // IO methods
    bool saveTo(QTextStream &stream) const;
    bool loadFrom(QTextStream &stream);

public slots:
    void addTrack(Track *track);
    void addTracks(QList<Track *> newTracks);
    void clear();
    void skipBackward();
    void skipForward();
    Track *getNextTrack();
    void trackRemoved();

signals:
    void activeRowChanged(int row, bool manual, bool startPlayback);
    void needSelectionFor(QList<Track *>);
    void itemChanged(int total);
    void playlistFinished();

private:
    void addShuffledTrack(Track *track);

    QList<Track *> tracks;
    QList<Track *> playedTracks;

    int activeRow;
    Track *activeTrack;

    QString errorMessage;
};

#endif // PLAYLISTMODEL_H
