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

#ifndef ALBUMLISTVIEW_H
#define ALBUMLISTVIEW_H

#include "albumsqlmodel.h"
#include "finderlistview.h"
#include <QtWidgets>

class AlbumListView : public FinderListView {
    Q_OBJECT

public:
    AlbumListView(QWidget *parent);
    void setModel(AlbumSqlModel *model) {
        FinderListView::setModel(model);
        sqlModel = model;
    }
    void updateQuery(bool transition = false);
    void setShowToolBar(bool show) { showToolBar = show; }

    enum SortBy { SortByTitle = 0, SortByArtist, SortByYear, SortByPopularity };

protected:
    void showEvent(QShowEvent *e);
    void hideEvent(QHideEvent *e);

private slots:
    void setSortBy(SortBy sortBy);
    void setSortByTitle() { setSortBy(SortByTitle); }
    void setSortByArtist() { setSortBy(SortByArtist); }
    void setSortByYear() { setSortBy(SortByYear); }
    void setSortByPopularity() { setSortBy(SortByPopularity); }
    void setReversedOrder(bool reversedOrder);
    void preloadThumbs();

private:
    QToolBar *toolBar;
    AlbumSqlModel *sqlModel;
    SortBy sortBy;
    bool reversedOrder;
    bool showToolBar;
};

#endif // ALBUMLISTVIEW_H
