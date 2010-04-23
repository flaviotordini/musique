#ifndef TRACKSQLMODEL_H
#define TRACKSQLMODEL_H

#include "basesqlmodel.h"
#include "model/track.h"
#include "finderwidget.h"

class TrackSqlModel : public BaseSqlModel {
    
    Q_OBJECT
    
public:
    TrackSqlModel(QObject *parent = 0);
    QVariant data(const QModelIndex &item, int role) const;
    Item* itemAt(const QModelIndex &index) const {
        const TrackPointer itemPointer = index.data(Finder::DataObjectRole).value<TrackPointer>();
        return dynamic_cast<Item*>(itemPointer.data());
    }
};

#endif
