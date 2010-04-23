#ifndef ARTISTSQLMODEL_H
#define ARTISTSQLMODEL_H

#include "basesqlmodel.h"
#include "model/artist.h"
#include "finderwidget.h"

class ArtistSqlModel : public BaseSqlModel {
    
    Q_OBJECT
    
public:
    ArtistSqlModel(QObject *parent = 0);
    QVariant data(const QModelIndex &item, int role) const;
    Item* itemAt(const QModelIndex &index) const {
        const ArtistPointer itemPointer = index.data(Finder::DataObjectRole).value<ArtistPointer>();
        return dynamic_cast<Item*>(itemPointer.data());
    }
};

#endif
