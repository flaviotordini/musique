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

#include "artistlistview.h"
#include "artistsqlmodel.h"
#include "database.h"
#include "finderitemdelegate.h"
#include "iconutils.h"
#include "mainwindow.h"
#ifdef APP_EXTRA
#include "extra.h"
#endif

namespace {
const char *sortByKey = "artistSortBy";
const char *reverseOrderKey = "artistReverseOrder";
} // namespace

ArtistListView::ArtistListView(QWidget *parent) : BaseFinderView(parent) {
    setupToolbar();
}

void ArtistListView::appear() {
    QStatusBar *statusBar = MainWindow::instance()->statusBar();
#ifdef APP_EXTRA
    Extra::fadeInWidget(statusBar, statusBar);
#endif
    BaseFinderView::appear();
    statusBar->insertPermanentWidget(0, toolBar);
    toolBar->show();

    // QTimer::singleShot(500, this, SLOT(preloadThumbs()));
}

void ArtistListView::disappear() {
    QStatusBar *statusBar = MainWindow::instance()->statusBar();
#ifdef APP_EXTRA
    Extra::fadeInWidget(statusBar, statusBar);
#endif
    // clearThumbs();
    BaseFinderView::disappear();
    statusBar->removeWidget(toolBar);
}

void ArtistListView::setupToolbar() {
    toolBar = new QToolBar();
    toolBar->hide();
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolBar->setIconSize(QSize(16, 16));

    QSettings settings;
    sortBy = static_cast<SortBy>(settings.value(sortByKey, SortByTrackCount).toInt());

    QMenu *sortMenu = new QMenu(this);
    QActionGroup *sortGroup = new QActionGroup(this);

    QAction *sortByTrackCountAction = new QAction(tr("Track Count"), this);
    sortByTrackCountAction->setActionGroup(sortGroup);
    sortByTrackCountAction->setCheckable(true);
    if (sortBy == SortByTrackCount) sortByTrackCountAction->setChecked(true);
    connect(sortByTrackCountAction, SIGNAL(triggered()), SLOT(setSortByTrackCount()));
    sortMenu->addAction(sortByTrackCountAction);

    QAction *sortByAlbumCountAction = new QAction(tr("Album Count"), this);
    sortByAlbumCountAction->setActionGroup(sortGroup);
    sortByAlbumCountAction->setCheckable(true);
    if (sortBy == SortByAlbumCount) sortByAlbumCountAction->setChecked(true);
    connect(sortByAlbumCountAction, SIGNAL(triggered()), SLOT(setSortByAlbumCount()));
    sortMenu->addAction(sortByAlbumCountAction);

    QAction *sortByNameAction = new QAction(tr("Name"), this);
    sortByNameAction->setActionGroup(sortGroup);
    sortByNameAction->setCheckable(true);
    if (sortBy == SortByName) sortByNameAction->setChecked(true);
    connect(sortByNameAction, SIGNAL(triggered()), SLOT(setSortByName()));
    sortMenu->addAction(sortByNameAction);

    QAction *sortByYearAction = new QAction(tr("Year"), this);
    sortByYearAction->setActionGroup(sortGroup);
    sortByYearAction->setCheckable(true);
    if (sortBy == SortByYear) sortByYearAction->setChecked(true);
    connect(sortByYearAction, SIGNAL(triggered()), SLOT(setSortByYear()));
    sortMenu->addAction(sortByYearAction);

    QAction *sortByPopularityAction = new QAction(tr("Popularity"), this);
    sortByPopularityAction->setActionGroup(sortGroup);
    sortByPopularityAction->setCheckable(true);
    if (sortBy == SortByPopularity) sortByPopularityAction->setChecked(true);
    connect(sortByPopularityAction, SIGNAL(triggered()), SLOT(setSortByPopularity()));
    sortMenu->addAction(sortByPopularityAction);

    sortMenu->addSeparator();

    reversedOrder = settings.value(reverseOrderKey, false).toBool();

    QAction *reversedOrderAction = new QAction(tr("Reversed Order"), this);
    reversedOrderAction->setCheckable(true);
    if (reversedOrder) reversedOrderAction->setChecked(true);
    connect(reversedOrderAction, SIGNAL(triggered(bool)), SLOT(setReversedOrder(bool)));
    sortMenu->addAction(reversedOrderAction);

    QToolButton *sortButton = new QToolButton(this);
    sortButton->setText(tr("Sort by"));
    sortButton->setIcon(IconUtils::icon("sort"));
    sortButton->setIconSize(QSize(16, 16));
    sortButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    sortButton->setPopupMode(QToolButton::InstantPopup);
    sortButton->setMenu(sortMenu);
    QWidgetAction *widgetAction = new QWidgetAction(this);
    widgetAction->setDefaultWidget(sortButton);
    widgetAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_O));
    toolBar->addAction(widgetAction);
}

void ArtistListView::updateQuery(bool transition) {
    QString sql = "select id from artists where trackCount>0";

    switch (sortBy) {
    case SortByTrackCount:
        sql += " order by trackCount";
        if (!reversedOrder) sql += " desc";
        sql += ", listeners";
        if (!reversedOrder) sql += " desc";
        break;
    case SortByAlbumCount:
        sql += " order by albumCount";
        if (!reversedOrder) sql += " desc";
        sql += ", listeners";
        if (!reversedOrder) sql += " desc";
        break;
    case SortByYear:
        sql += " and yearFrom>0 order by yearFrom";
        if (!reversedOrder) sql += " desc";
        break;
    case SortByPopularity:
        sql += " order by listeners";
        if (!reversedOrder) sql += " desc";
        break;
    default:
        sql += " order by name collate nocase";
        if (reversedOrder) sql += " desc";
        break;
    }

#ifdef APP_EXTRA
    if (transition) Extra::fadeInWidget(this, this);
#endif

    if (!sqlModel->query().isValid()) QTimer::singleShot(500, this, SLOT(preloadThumbs()));

    sqlModel->setQuery(sql, Database::instance().getConnection());
    if (sqlModel->lastError().isValid()) qWarning() << sqlModel->lastError().text();

    scrollToTop();
}

void ArtistListView::preloadThumbs() {
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(sqlModel->query().lastQuery(), db);
    bool success = query.exec();
    if (!success)
        qDebug() << query.lastQuery() << query.lastError().text() << query.lastError().number();

    const qreal pixelRatio = IconUtils::pixelRatio();

    while (query.next()) {
        int artistId = query.value(0).toInt();
        Artist *artist = Artist::forId(artistId);
        artist->getPhotoForSize(FinderItemDelegate::ITEM_WIDTH, FinderItemDelegate::ITEM_HEIGHT,
                                pixelRatio);
        qApp->processEvents();
    }
}

void ArtistListView::clearThumbs() {
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(sqlModel->query().lastQuery(), db);
    bool success = query.exec();
    if (!success)
        qDebug() << query.lastQuery() << query.lastError().text() << query.lastError().number();

    while (query.next()) {
        int artistId = query.value(0).toInt();
        Artist *artist = Artist::forId(artistId);
        artist->clearPixmapCache();
    }
}

void ArtistListView::setSortBy(SortBy sortBy) {
    this->sortBy = sortBy;
    updateQuery(true);
    QSettings settings;
    settings.setValue(sortByKey, (int)sortBy);
}

void ArtistListView::setReversedOrder(bool reversedOrder) {
    this->reversedOrder = reversedOrder;
    updateQuery(true);
    QSettings settings;
    settings.setValue(reverseOrderKey, reversedOrder);
}
