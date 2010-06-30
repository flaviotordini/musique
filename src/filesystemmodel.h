#ifndef FILESYSTEMMODEL_H
#define FILESYSTEMMODEL_H

#include <QFileSystemModel>
#include "model/folder.h"
#include "finderwidget.h"

class FileSystemModel : public QFileSystemModel {

    Q_OBJECT

public:
    FileSystemModel(QObject *parent = 0);
    QVariant data(const QModelIndex &item, int role) const;
    Item* itemAt(const QModelIndex &index) const {
        if (isDir(index)) {
            const FolderPointer folderPointer = index.data(Finder::DataObjectRole).value<FolderPointer>();
            Folder *folder = folderPointer.data();
            return dynamic_cast<Item*>(folder);
        } else {
            const TrackPointer trackPointer = index.data(Finder::DataObjectRole).value<TrackPointer>();
            Track *track = trackPointer.data();
            return dynamic_cast<Item*>(track);
        }
    }

    void clear() {
        //reset();
    }
    void setHoveredRow(int row);

public slots:
    void clearHover();
    void enterPlayIconHover();
    void exitPlayIconHover();

protected:
    // drag and drop
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QStringList mimeTypes() const;
    Qt::DropActions supportedDropActions() const;
    QMimeData* mimeData( const QModelIndexList &indexes ) const;

    int hoveredRow;
    QTimeLine * timeLine;
    bool playIconHovered;

private slots:
    void updatePlayIcon();

};

#endif // FILESYSTEMMODEL_H
