#ifndef ALBUMSQLMODEL_H
#define ALBUMSQLMODEL_H

#include "basesqlmodel.h"
#include "model/album.h"
#include "finderwidget.h"

class AlbumSqlModel : public BaseSqlModel {

    Q_OBJECT

public:
    AlbumSqlModel(QObject *parent = 0);
    QVariant data(const QModelIndex &item, int role) const;
    Item* itemAt(const QModelIndex &index) const {
        const AlbumPointer itemPointer = index.data(Finder::DataObjectRole).value<AlbumPointer>();
        return dynamic_cast<Item*>(itemPointer.data());
    }

};

#endif // ALBUMSQLMODEL_H
