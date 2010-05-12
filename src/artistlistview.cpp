#include "artistlistview.h"
#include "finderitemdelegate.h"
#include "database.h"
#include <QtSql>
#include "artistsqlmodel.h"

ArtistListView::ArtistListView(QWidget *parent) : BaseFinderView(parent) {
    setWindowTitle(tr("Artists"));
}

void ArtistListView::appear() {
    ArtistSqlModel *artistListModel = static_cast<ArtistSqlModel*>(model());
    if (artistListModel)
        artistListModel->setQuery("select id from artists where trackCount > 1 order by trackCount desc",
                                  Database::instance().getConnection());
}
