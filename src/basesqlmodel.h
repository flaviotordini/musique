#ifndef BASESQLMODEL_H
#define BASESQLMODEL_H

#include <QSqlQueryModel>
#include "model/item.h"

class BaseSqlModel : public QSqlQueryModel {

    Q_OBJECT

public:
    BaseSqlModel(QObject *parent = 0);
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

    virtual Item* itemAt(const QModelIndex &index) const = 0;

    int hoveredRow;
    QTimeLine * timeLine;
    bool playIconHovered;

private slots:
    void updatePlayIcon();


};

#endif // BASESQLMODEL_H
