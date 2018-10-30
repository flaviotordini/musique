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

#include "filesystemmodel.h"
#include "database.h"
#include "trackmimedata.h"

FileSystemModel::FileSystemModel(QObject *parent) : QFileSystemModel(parent) {}

QVariant FileSystemModel::data(const QModelIndex &index, int role) const {
    Folder *folder = nullptr;
    Track *track = nullptr;
    QString path;

    switch (role) {
    case Finder::ItemTypeRole:
        if (isDir(index))
            return Finder::ItemTypeFolder;
        else
            return Finder::ItemTypeTrack;

    case Finder::DataObjectRole:
        path = QFileSystemModel::data(index, QFileSystemModel::FilePathRole).toString();
        // qDebug() << "model path" << path << path.isEmpty();
        if (isDir(index)) {
            folder = Folder::forPath(path);
            return QVariant::fromValue(QPointer<Folder>(folder));
        } else {
            path.remove(Database::instance().collectionRoot() + "/");
            track = Track::forPath(path);
            return QVariant::fromValue(QPointer<Track>(track));
        }

    case Qt::StatusTipRole:
        if (!isDir(index)) {
            path = QFileSystemModel::data(index, QFileSystemModel::FilePathRole).toString();
            path.remove(Database::instance().collectionRoot() + "/");
            track = Track::forPath(path);
            return track->getStatusTip();
        }
        return QVariant();

    default:
        return QFileSystemModel::data(index, role);
    }
}

Item *FileSystemModel::itemAt(const QModelIndex &index) const {
    if (isDir(index)) {
        const FolderPointer folderPointer =
                index.data(Finder::DataObjectRole).value<FolderPointer>();
        Folder *folder = folderPointer.data();
        return qobject_cast<Item *>(folder);
    } else {
        const TrackPointer trackPointer = index.data(Finder::DataObjectRole).value<TrackPointer>();
        Track *track = trackPointer.data();
        return qobject_cast<Item *>(track);
    }
}

// --- Sturm und drang ---

Qt::DropActions FileSystemModel::supportedDropActions() const {
    return Qt::CopyAction;
}

Qt::ItemFlags FileSystemModel::flags(const QModelIndex &index) const {
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
    if (index.isValid()) {
        return (defaultFlags | Qt::ItemIsDragEnabled);
    } else
        return defaultFlags;
}

QStringList FileSystemModel::mimeTypes() const {
    QStringList types;
    types << TRACK_MIME;
    return types;
}

QMimeData *FileSystemModel::mimeData(const QModelIndexList &indexes) const {
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
