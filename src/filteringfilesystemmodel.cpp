#include "filteringfilesystemmodel.h"
#include "finderwidget.h"
#include "model/track.h"
#include "model/folder.h"

FilteringFileSystemModel::FilteringFileSystemModel(QObject *parent) :
        QSortFilterProxyModel(parent) { }

bool FilteringFileSystemModel::filterAcceptsRow(
        int sourceRow,
        const QModelIndex &sourceParent) const {

    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    const QVariant dataObject = sourceModel()->data(index, Finder::DataObjectRole);
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

    const TrackPointer trackPointer = dataObject.value<TrackPointer>();
    Track *track = trackPointer.data();
    if (track) return true;
    else return false;
}
