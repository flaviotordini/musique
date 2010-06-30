#ifndef FILTERINGFILESYSTEMMODEL_H
#define FILTERINGFILESYSTEMMODEL_H

#include <QtGui>

class FilteringFileSystemModel : public QSortFilterProxyModel {

    Q_OBJECT

public:
    FilteringFileSystemModel(QObject *parent = 0);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

};

#endif // FILTERINGFILESYSTEMMODEL_H
