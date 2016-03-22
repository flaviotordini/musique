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

#ifndef SEARCHMODEL_H
#define SEARCHMODEL_H

#include <QtWidgets>

class ArtistSqlModel;
class AlbumSqlModel;
class TrackSqlModel;
class FileSystemModel;
class FinderWidget;
class Item;

class SearchModel : public QAbstractListModel {

    Q_OBJECT

public:
    SearchModel(QObject *parent = 0);
    void search(QString query);
    void setHoveredRow(int row);

public slots:
    void clearHover();
    void enterPlayIconHover();
    void exitPlayIconHover();

protected:
    int rowCount(const QModelIndex & /* parent */) const;
    QVariant data(const QModelIndex &item, int role) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const { return 1; }

private slots:
    void updatePlayIcon();

    void itemEntered(const QModelIndex &index);
    void itemActivated(const QModelIndex &index);
    void itemPlayed(const QModelIndex &index);

private:
    Item* itemAt(const QModelIndex &index) const;

    ArtistSqlModel *artistListModel;
    AlbumSqlModel *albumListModel;
    TrackSqlModel *trackListModel;
    FileSystemModel *fileSystemModel;

    // drag and drop
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QStringList mimeTypes() const;
    Qt::DropActions supportedDropActions() const;
    QMimeData* mimeData( const QModelIndexList &indexes ) const;

    int hoveredRow;
    QTimeLine * timeLine;
    bool playIconHovered;

    FinderWidget *finder;

};

#endif // SEARCHMODEL_H
