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

#ifndef FILTERINGFILESYSTEMMODEL_H
#define FILTERINGFILESYSTEMMODEL_H

#include <QtGui>

class FilteringFileSystemModel : public QSortFilterProxyModel {

    Q_OBJECT

public:
    FilteringFileSystemModel(QObject *parent = 0);
    void setHoveredRow(int row);

public slots:
    void clearHover();
    void enterPlayIconHover();
    void exitPlayIconHover();

protected:
    QVariant data(const QModelIndex &index, int role) const;
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

    int hoveredRow;
    QTimeLine * timeLine;
    bool playIconHovered;

private slots:
    void updatePlayIcon();

};

#endif // FILTERINGFILESYSTEMMODEL_H
