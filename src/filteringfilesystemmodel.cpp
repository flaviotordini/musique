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

#include "filteringfilesystemmodel.h"
#include "finderwidget.h"
#include "model/folder.h"
#include "model/track.h"

FilteringFileSystemModel::FilteringFileSystemModel(QObject *parent)
    : QSortFilterProxyModel(parent) {}

bool FilteringFileSystemModel::filterAcceptsRow(int sourceRow,
                                                const QModelIndex &sourceParent) const {
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    int itemType = sourceModel()->data(index, Finder::ItemTypeRole).toInt();
    if (itemType == Finder::ItemTypeFolder) {
        return true;
        /*
        const FolderPointer folderPointer = dataObject.value<FolderPointer>();
        Folder *folder = folderPointer.data();
        if (!folder) return true;
        qDebug() << "testing" << folder->getPath();
        return (folder->getTotalLength() > 0);
        */
    }

    const QVariant dataObject = sourceModel()->data(index, Finder::DataObjectRole);
    const TrackPointer trackPointer = dataObject.value<TrackPointer>();
    Track *track = trackPointer.data();
    return track != nullptr;
}

QVariant FilteringFileSystemModel::data(const QModelIndex &index, int role) const {
    switch (role) {
    default:
        return QSortFilterProxyModel::data(index, role);
    }
}
