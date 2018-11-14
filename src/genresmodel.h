#ifndef METAGENRESMODEL_H
#define METAGENRESMODEL_H

#include <QtCore>

#include "finderwidget.h"
#include "genres.h"

class GenresModel : public QAbstractItemModel {
    Q_OBJECT

public:
    GenresModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &index) const;
    int columnCount(const QModelIndex &parent) const;

public slots:
    void refreshIndex(const QModelIndex &index) { emit dataChanged(index, index); }

protected:
    Qt::DropActions supportedDropActions() const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QStringList mimeTypes() const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;

private:
    Item *itemForIndex(const QModelIndex &index) const;

    Genres genres;
};

#endif // METAGENRESMODEL_H
