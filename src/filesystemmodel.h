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
