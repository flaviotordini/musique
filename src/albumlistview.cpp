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

#include "albumlistview.h"
#include "mainwindow.h"
#include "iconutils.h"
#include "artistsqlmodel.h"
#include "database.h"
#ifdef APP_EXTRA
#include "extra.h"
#endif

namespace {
const char *sortByKey = "albumSortBy";
const char *reverseOrderKey = "albumReverseOrder";
}

AlbumListView::AlbumListView(QWidget *parent) : BaseFinderView(parent),
    showToolBar(false) {
    setupToolbar();
}

void AlbumListView::appear() {
    BaseFinderView::appear();
    if (showToolBar) {
        QStatusBar *statusBar = MainWindow::instance()->statusBar();
#ifdef APP_EXTRA
        Extra::fadeInWidget(statusBar, statusBar);
#endif
        statusBar->insertPermanentWidget(0, toolBar);
        toolBar->show();
    }
}

void AlbumListView::disappear() {
    BaseFinderView::disappear();
    if (showToolBar) {
        QStatusBar *statusBar = MainWindow::instance()->statusBar();
#ifdef APP_EXTRA
        Extra::fadeInWidget(statusBar, statusBar);
#endif
        statusBar->removeWidget(toolBar);
    }
}

void AlbumListView::setupToolbar() {
    toolBar = new QToolBar();
    toolBar->hide();
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolBar->setIconSize(QSize(16, 16));

    QSettings settings;
    sortBy = static_cast<SortBy>(settings.value(sortByKey, SortByArtist).toInt());

    QMenu *sortMenu = new QMenu(this);
    QActionGroup *sortGroup = new QActionGroup(this);

    QAction *sortByArtistAction = new QAction(tr("Artist"), this);
    sortByArtistAction->setActionGroup(sortGroup);
    sortByArtistAction->setCheckable(true);
    if (sortBy == SortByArtist) sortByArtistAction->setChecked(true);
    connect(sortByArtistAction, SIGNAL(triggered()), SLOT(setSortByArtist()));
    sortMenu->addAction(sortByArtistAction);

    QAction *sortByNameAction = new QAction(tr("Title"), this);
    sortByNameAction->setActionGroup(sortGroup);
    sortByNameAction->setCheckable(true);
    if (sortBy == SortByTitle) sortByNameAction->setChecked(true);
    connect(sortByNameAction, SIGNAL(triggered()), SLOT(setSortByTitle()));
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

void AlbumListView::updateQuery(bool transition) {
    QString sql = "select id from albums where trackCount>0";

    switch (sortBy) {
    case SortByArtist:
        sql = "select b.id from albums b, artists a"
                " where b.artist=a.id and b.trackCount>0"
                " order by a.name collate nocase";
        if (reversedOrder) sql += " desc";
        sql += ", b.year";
        if (!reversedOrder) sql += " desc";
        sql += ", b.trackCount";
        if (!reversedOrder) sql += " desc";
        break;
    case SortByYear:
        sql += " and year>0 order by year";
        if (!reversedOrder) sql += " desc";
        break;
    case SortByPopularity:
        sql += " order by listeners";
        if (!reversedOrder) sql += " desc";
        break;
    default:
        sql += " order by title collate nocase";
        if (reversedOrder) sql += " desc";
        break;
    }

#ifdef APP_EXTRA
    if (transition)
        Extra::fadeInWidget(this, this);
#endif

    if (!sqlModel->query().isValid())
        QTimer::singleShot(1000, this, SLOT(preloadThumbs()));

    sqlModel->setQuery(sql, Database::instance().getConnection());
    if (sqlModel->lastError().isValid())
        qWarning() << sqlModel->lastError().text();

    scrollToTop();
    showToolBar = true;
}

void AlbumListView::preloadThumbs() {
    qApp->processEvents();
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(sqlModel->query().lastQuery(), db);
    bool success = query.exec();
    if (!success)
        qDebug() << query.lastQuery() << query.lastError().text() << query.lastError().number();
    while (query.next()) {
        int albumId = query.value(0).toInt();
        Album* album = Album::forId(albumId);
        album->getThumb();
        qApp->processEvents();
    }
}

void AlbumListView::setSortBy(SortBy sortBy) {
    this->sortBy = sortBy;
    updateQuery(true);
    QSettings settings;
    settings.setValue(sortByKey, (int)sortBy);
}

void AlbumListView::setReversedOrder(bool reversedOrder) {
    this->reversedOrder = reversedOrder;
    updateQuery(true);
    QSettings settings;
    settings.setValue(reverseOrderKey, reversedOrder);
}
