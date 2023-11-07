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

#include "tracksqlmodel.h"
#include "finderwidget.h"
#include "playlistmodel.h"

TrackSqlModel::TrackSqlModel(QObject *parent) : BaseSqlModel(parent) {}

QVariant TrackSqlModel::data(const QModelIndex &index, int role) const {
    Track *track = nullptr;
    int trackId = 0;

    switch (role) {
    case Qt::DisplayRole:
        trackId = QSqlQueryModel::data(QSqlQueryModel::index(index.row(), 0)).toInt();
        track = Track::forId(trackId);
        return track->getTitle();

    case Finder::ItemTypeRole:
        return Finder::ItemTypeTrack;

    case PlaylistRoles::DataObjectRole:
    case Finder::DataObjectRole:
        trackId = QSqlQueryModel::data(QSqlQueryModel::index(index.row(), 0)).toInt();
        track = Track::forId(trackId);
        return QVariant::fromValue(QPointer<Track>(track));

    case Qt::StatusTipRole:
        trackId = QSqlQueryModel::data(QSqlQueryModel::index(index.row(), 0)).toInt();
        track = Track::forId(trackId);
        return track->getStatusTip();
    }

    return QVariant();
}
