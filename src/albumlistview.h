#ifndef ALBUMLISTVIEW_H
#define ALBUMLISTVIEW_H

#include <QtGui>
#include <QtSql>
#include "basefinderview.h"
#include "albumsqlmodel.h"

class AlbumListView : public BaseFinderView {

    Q_OBJECT

public:
    AlbumListView(QWidget *parent);
    void setModel(AlbumSqlModel *model) { BaseFinderView::setModel(model); sqlModel = model; }
    void updateQuery(bool transition = false);
    void setShowToolBar(bool show) { showToolBar = show; }

    enum SortBy {
        SortByTitle = 0,
        SortByArtist,
        SortByYear,
        SortByPopularity
    };

public slots:
    void appear();
    void disappear();

private slots:
    void setSortBy(SortBy sortBy);
    void setSortByTitle() { setSortBy(SortByTitle); }
    void setSortByArtist() { setSortBy(SortByArtist); }
    void setSortByYear() { setSortBy(SortByYear); }
    void setSortByPopularity() { setSortBy(SortByPopularity); }
    void setReversedOrder(bool reversedOrder);
    void preloadThumbs();

private:
    void setupToolbar();
    QToolBar *toolBar;
    AlbumSqlModel *sqlModel;
    SortBy sortBy;
    bool reversedOrder;
    bool showToolBar;
};

#endif // ALBUMLISTVIEW_H
