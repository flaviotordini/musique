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

#include "albumsqlmodel.h"
#include "mainwindow.h"
#include "model/album.h"

AlbumSqlModel::AlbumSqlModel(QObject *parent) : BaseSqlModel(parent) {}

QVariant AlbumSqlModel::data(const QModelIndex &index, int role) const {
    switch (role) {
    case Finder::ItemTypeRole:
        return Finder::ItemTypeAlbum;

    case Finder::ItemObjectRole:
        return QVariant::fromValue(QPointer<Item>(albumForIndex(index)));

    case Finder::DataObjectRole:
        return QVariant::fromValue(QPointer<Album>(albumForIndex(index)));

    case Qt::StatusTipRole:
        return albumForIndex(index)->getStatusTip();
    }

    return QVariant();
}

Album *AlbumSqlModel::albumForIndex(const QModelIndex &index) const {
    int id = QSqlQueryModel::data(index).toInt();
    return Album::forId(id);
}
