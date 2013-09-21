#ifndef ARTISTLISTVIEW_H
#define ARTISTLISTVIEW_H

#include <QtGui>
#include <QtSql>
#include "basefinderview.h"
#include "artistsqlmodel.h"

class ArtistListView : public BaseFinderView {

    Q_OBJECT

public:
    ArtistListView(QWidget *parent);
    void setModel(ArtistSqlModel *model) { BaseFinderView::setModel(model); sqlModel = model; }
    void updateQuery(bool transition = false);

    enum SortBy {
        SortByName = 0,
        SortByAlbumCount,
        SortByTrackCount,
        SortByYear,
        SortByPopularity
    };

public slots:
    void appear();
    void disappear();

private slots:
    void setSortBy(SortBy sortBy);
    void setSortByName() { setSortBy(SortByName); }
    void setSortByTrackCount() { setSortBy(SortByTrackCount); }
    void setSortByAlbumCount() { setSortBy(SortByAlbumCount); }
    void setSortByYear() { setSortBy(SortByYear); }
    void setSortByPopularity() { setSortBy(SortByPopularity); }
    void setReversedOrder(bool reversedOrder);
    void preloadThumbs();

private:
    void setupToolbar();
    QToolBar *toolBar;
    ArtistSqlModel *sqlModel;
    SortBy sortBy;
    bool reversedOrder;

};

#endif // ARTISTLISTVIEW_H
