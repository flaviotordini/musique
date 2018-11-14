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

#include "searchmodel.h"

#include "model/album.h"
#include "model/artist.h"

#include "albumsqlmodel.h"
#include "artistsqlmodel.h"
#include "filesystemmodel.h"
#include "filteringfilesystemmodel.h"
#include "tracksqlmodel.h"

#include "database.h"
#include "trackmimedata.h"

#include "finderwidget.h"

SearchModel::SearchModel(QObject *parent) : QAbstractListModel(parent) {
    finder = qobject_cast<FinderWidget *>(parent);

    artistListModel = new ArtistSqlModel(this);
    albumListModel = new AlbumSqlModel(this);
    trackListModel = new TrackSqlModel(this);

    fileSystemModel = new FileSystemModel(this);
    fileSystemModel->setResolveSymlinks(true);
    fileSystemModel->setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    FilteringFileSystemModel *proxyModel = new FilteringFileSystemModel(this);
    proxyModel->setSourceModel(fileSystemModel);
}

int SearchModel::rowCount(const QModelIndex &parent) const {
    return artistListModel->rowCount(parent) + albumListModel->rowCount(parent) +
           trackListModel->rowCount(parent);
}

QVariant SearchModel::data(const QModelIndex &index, int role) const {
    const int row = index.row();
    const int artistRowCount = artistListModel->rowCount(index.parent());
    const int albumRowCount = albumListModel->rowCount(index.parent());
    const int trackRowCount = trackListModel->rowCount(index.parent());

    switch (role) {
    default:
        if (row >= 0 && row < artistRowCount) {
            return artistListModel->data(index, role);
        } else if (row >= artistRowCount && row < artistRowCount + albumRowCount) {
            return albumListModel->data(createIndex(row - artistRowCount, index.column()), role);
        } else if (row >= artistRowCount + albumRowCount &&
                   row < artistRowCount + albumRowCount + trackRowCount) {
            return trackListModel->data(
                    createIndex(row - artistRowCount - albumRowCount, index.column()), role);
        }
    }

    return QVariant();
}

void SearchModel::search(const QString &query) {
    beginResetModel();

    QString likeQuery = "%" + query + "%";

    QSqlQuery q(Database::instance().getConnection());
    q.prepare("select id from artists where name like ? and trackCount>0 order by trackCount desc");
    q.bindValue(0, likeQuery);
    q.exec();
    artistListModel->setQuery(q);
    if (artistListModel->lastError().isValid()) qDebug() << artistListModel->lastError();

    q.prepare("select id from albums where (title like ? or year=?) and trackCount>0 order by year "
              "desc, trackCount desc");
    q.bindValue(0, likeQuery);
    q.bindValue(1, query);
    q.exec();
    albumListModel->setQuery(q);
    if (albumListModel->lastError().isValid()) qDebug() << albumListModel->lastError();

    q.prepare("select id from tracks where title like ? order by track, path");
    q.bindValue(0, likeQuery);
    q.exec();
    trackListModel->setQuery(q);
    if (trackListModel->lastError().isValid()) qDebug() << trackListModel->lastError();

    endResetModel();
}

Item *SearchModel::itemAt(const QModelIndex &index) const {
    Item *item = nullptr;

    int itemType = index.data(Finder::ItemTypeRole).toInt();
    if (itemType == Finder::ItemTypeArtist) {
        const ArtistPointer pointer = index.data(Finder::DataObjectRole).value<ArtistPointer>();
        item = qobject_cast<Item *>(pointer.data());
    } else if (itemType == Finder::ItemTypeAlbum) {
        const AlbumPointer pointer = index.data(Finder::DataObjectRole).value<AlbumPointer>();
        item = qobject_cast<Item *>(pointer.data());
    } else if (itemType == Finder::ItemTypeFolder) {
    } else if (itemType == Finder::ItemTypeTrack) {
        const TrackPointer pointer = index.data(Finder::DataObjectRole).value<TrackPointer>();
        item = qobject_cast<Item *>(pointer.data());
    }

    return item;
}

// --- Events ---

void SearchModel::itemActivated(const QModelIndex &index) {
    int itemType = index.data(Finder::ItemTypeRole).toInt();
    if (itemType == Finder::ItemTypeArtist) {
        const ArtistPointer pointer = index.data(Finder::DataObjectRole).value<ArtistPointer>();
        finder->artistActivated(pointer.data());
    } else if (itemType == Finder::ItemTypeAlbum) {
        const AlbumPointer pointer = index.data(Finder::DataObjectRole).value<AlbumPointer>();
        finder->albumActivated(pointer.data());
    } else if (itemType == Finder::ItemTypeFolder) {
    } else if (itemType == Finder::ItemTypeTrack) {
        const TrackPointer pointer = index.data(Finder::DataObjectRole).value<TrackPointer>();
        finder->trackActivated(pointer.data());
    }
}

void SearchModel::itemPlayed(const QModelIndex &index) {
    Item *item = itemAt(index);
    if (!item) return;
    QVector<Track *> tracks = item->getTracks();
    finder->addTracksAndPlay(tracks);
}

// --- Sturm und drang ---

Qt::DropActions SearchModel::supportedDropActions() const {
    return Qt::MoveAction;
}

Qt::ItemFlags SearchModel::flags(const QModelIndex &index) const {
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
    if (index.isValid()) {
        return (defaultFlags | Qt::ItemIsDragEnabled);
    } else
        return defaultFlags;
}

QStringList SearchModel::mimeTypes() const {
    return TrackMimeData::types();
}

QMimeData *SearchModel::mimeData(const QModelIndexList &indexes) const {
    TrackMimeData *mime = new TrackMimeData();

    for (const QModelIndex &index : indexes) {
        Item *item = itemAt(index);
        if (item) {
            // qDebug() << item->getTracks();
            mime->addTracks(item->getTracks());
        }
    }

    return mime;
}
