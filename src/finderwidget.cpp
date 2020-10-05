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

#include "finderwidget.h"
#include "finderitemdelegate.h"

#include "segmentedcontrol.h"

#include "breadcrumb.h"

#include "playlistmodel.h"
#include "playlistview.h"

#include "model/album.h"
#include "model/artist.h"
#include "model/genre.h"

#include "artistlistview.h"
#include "artistsqlmodel.h"

#include "albumlistview.h"
#include "albumsqlmodel.h"

#include "genresmodel.h"

#include "tracklistview.h"
#include "tracksqlmodel.h"

#include "filesystemfinderview.h"
#include "filesystemmodel.h"
#include "filteringfilesystemmodel.h"

#include "searchmodel.h"
#include "searchview.h"

#include "database.h"
#include <QtSql>

namespace {
const QString finderViewKey = QStringLiteral("finderView");
}

FinderWidget::FinderWidget(QWidget *parent) : QWidget(parent) {
    fileSystemView = nullptr;
    artistListView = nullptr;
    albumListView = nullptr;
    trackListView = nullptr;
    genresListView = nullptr;

    fileSystemModel = nullptr;
    artistListModel = nullptr;
    albumListModel = nullptr;
    trackListModel = nullptr;
    genresModel = nullptr;

    searchView = nullptr;

    // colors
    QPalette p = palette();
    QColor backgroundColor(0x20, 0x20, 0x20);
    p.setBrush(QPalette::Window, backgroundColor);
    p.setBrush(QPalette::Base, backgroundColor);
    p.setBrush(QPalette::Text, Qt::white);
    p.setBrush(QPalette::WindowText, Qt::white);

    QBoxLayout *layout = new QVBoxLayout();
    layout->setMargin(0);
    layout->setSpacing(0);

    setupBar();
    layout->addWidget(finderBar);

    breadcrumb = new Breadcrumb(this);
    breadcrumb->setPalette(p);
    breadcrumb->hide();
    connect(breadcrumb, SIGNAL(goneBack()), SLOT(goBack()));
    layout->addWidget(breadcrumb);

    folderBreadcrumb = new Breadcrumb(this);
    folderBreadcrumb->setPalette(p);
    folderBreadcrumb->hide();
    connect(folderBreadcrumb, SIGNAL(goneBack()), SLOT(folderGoBack()));
    layout->addWidget(folderBreadcrumb);

    stackedWidget = new QStackedWidget(this);
    stackedWidget->setPalette(p);

    layout->addWidget(stackedWidget);
    setLayout(layout);

    setMinimumWidth(FinderItemDelegate::ITEM_WIDTH * 3 + 4 +
                    style()->pixelMetric(QStyle::PM_ScrollBarExtent));
    setMinimumHeight(FinderItemDelegate::ITEM_HEIGHT + finderBar->minimumHeight());

    restoreSavedView();
}

void FinderWidget::restoreSavedView() {
    QSettings settings;
    QString currentViewName = settings.value(finderViewKey).toString();

    if (currentViewName == "folders")
        QTimer::singleShot(0, this, SLOT(showFolders()));
    else if (currentViewName == "albums")
        QTimer::singleShot(0, this, SLOT(showAlbums()));
    else if (currentViewName == "genres")
        QTimer::singleShot(0, this, SLOT(showGenres()));
    else
        QTimer::singleShot(0, this, SLOT(showArtists()));

    /*
    if (currentViewName == "folders") QTimer::singleShot(0, foldersAction, SLOT(trigger()));
    else if (currentViewName == "albums") QTimer::singleShot(0, albumsAction, SLOT(trigger()));
    else QTimer::singleShot(0, artistsAction, SLOT(trigger()));
    */
}

void FinderWidget::setupBar() {
    finderBar = new SegmentedControl(this);

    artistsAction = new QAction(tr("Artists"), this);
    artistsAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_1));
    connect(artistsAction, SIGNAL(triggered()), SLOT(showArtists()));
    finderBar->addAction(artistsAction);

    albumsAction = new QAction(tr("Albums"), this);
    albumsAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_2));
    connect(albumsAction, SIGNAL(triggered()), SLOT(showAlbums()));
    finderBar->addAction(albumsAction);

    genresAction = new QAction(tr("Genres"), this);
    genresAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_3));
    connect(genresAction, SIGNAL(triggered()), SLOT(showGenres()));
    finderBar->addAction(genresAction);

    foldersAction = new QAction(tr("Folders"), this);
    foldersAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_4));
    connect(foldersAction, SIGNAL(triggered()), SLOT(showFolders()));
    finderBar->addAction(foldersAction);

    for (QAction *action : finderBar->actions()) {
        addAction(action);
        action->setAutoRepeat(false);
        if (!action->shortcut().isEmpty())
            action->setStatusTip(action->text() + " (" +
                                 action->shortcut().toString(QKeySequence::NativeText) + ")");
    }

    finderBar->setCheckedAction(0);
}

void FinderWidget::setupArtists() {
    artistListModel = new ArtistSqlModel(this);
    artistListView = new ArtistListView(stackedWidget);
    artistListView->setEnabled(false);
    connect(artistListView, SIGNAL(activated(const QModelIndex &)),
            SLOT(artistActivated(const QModelIndex &)));
    connect(artistListView, SIGNAL(play(const QModelIndex &)),
            SLOT(artistPlayed(const QModelIndex &)));
    artistListView->setModel(artistListModel);
    stackedWidget->addWidget(artistListView);
}

void FinderWidget::setupAlbums() {
    albumListModel = new AlbumSqlModel(this);
    albumListView = new AlbumListView(stackedWidget);
    albumListView->setEnabled(false);
    connect(albumListView, SIGNAL(activated(const QModelIndex &)),
            SLOT(albumActivated(const QModelIndex &)));
    connect(albumListView, SIGNAL(play(const QModelIndex &)),
            SLOT(albumPlayed(const QModelIndex &)));
    albumListView->setModel(albumListModel);
    stackedWidget->addWidget(albumListView);
}

void FinderWidget::setupGenres() {
    genresModel = new GenresModel(this);
    genresListView = new FinderListView(stackedWidget);
    genresListView->setEnabled(false);
    connect(genresListView, SIGNAL(activated(const QModelIndex &)),
            SLOT(genreActivated(const QModelIndex &)));
    connect(genresListView, SIGNAL(play(const QModelIndex &)),
            SLOT(genrePlayed(const QModelIndex &)));
    genresListView->setModel(genresModel);
    stackedWidget->addWidget(genresListView);
}

void FinderWidget::setupTracks() {
    trackListModel = new TrackSqlModel(this);
    trackListView = new TrackListView(stackedWidget);
    connect(trackListView, SIGNAL(activated(const QModelIndex &)),
            SLOT(trackActivated(const QModelIndex &)));
    trackListView->setModel(trackListModel);
    stackedWidget->addWidget(trackListView);
    trackListView->setPalette(stackedWidget->palette());
}

void FinderWidget::setupFolders() {
    fileSystemModel = new FileSystemModel(this);
    fileSystemModel->setResolveSymlinks(true);
    fileSystemModel->setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    filteringFileSystemModel = new FilteringFileSystemModel(this);
    filteringFileSystemModel->setSourceModel(fileSystemModel);

    fileSystemView = new FileSystemFinderView(stackedWidget);
    connect(fileSystemView, SIGNAL(activated(const QModelIndex &)),
            SLOT(folderActivated(const QModelIndex &)));
    connect(fileSystemView, SIGNAL(play(const QModelIndex &)),
            SLOT(folderPlayed(const QModelIndex &)));
    fileSystemView->setModel(filteringFileSystemModel);
    fileSystemView->setFileSystemModel(fileSystemModel);
    stackedWidget->addWidget(fileSystemView);
}

void FinderWidget::setupSearch() {
    searchModel = new SearchModel(this);
    searchView = new SearchView(stackedWidget);

    connect(searchView, SIGNAL(activated(const QModelIndex &)), searchModel,
            SLOT(itemActivated(const QModelIndex &)));
    connect(searchView, SIGNAL(play(const QModelIndex &)), searchModel,
            SLOT(itemPlayed(const QModelIndex &)));

    searchView->setEnabled(false);
    searchView->setModel(searchModel);
    stackedWidget->addWidget(searchView);
}

void FinderWidget::showArtists() {
    if (!artistListView) setupArtists();
    artistListView->updateQuery();
    showWidget(artistListView, true);
    finderBar->setCheckedAction(artistsAction);
    QSettings settings;
    settings.setValue(finderViewKey, "artists");
}

void FinderWidget::showAlbums() {
    if (!albumListView) setupAlbums();
    albumListView->updateQuery();
    albumListView->setShowToolBar(true);
    showWidget(albumListView, true);
    finderBar->setCheckedAction(albumsAction);
    QSettings settings;
    settings.setValue(finderViewKey, "albums");
}

void FinderWidget::showGenres() {
    if (!genresListView) setupGenres();
    if (genresListView->rootIndex().isValid()) genresListView->setRootIndex(QModelIndex());
    showWidget(genresListView, true);
    finderBar->setCheckedAction(genresAction);
    QSettings settings;
    settings.setValue(finderViewKey, "genres");
}

void FinderWidget::showFolders() {
    if (!fileSystemView) setupFolders();

    const QString path = Database::instance().collectionRoot();
    fileSystemModel->setRootPath(path);
    QSortFilterProxyModel *proxyModel =
            static_cast<QSortFilterProxyModel *>(fileSystemView->model());
    fileSystemView->setRootIndex(proxyModel->mapFromSource(fileSystemModel->index(path)));

    showWidget(fileSystemView, true);
    finderBar->setCheckedAction(foldersAction);
    QSettings settings;
    settings.setValue(finderViewKey, "folders");
}

void FinderWidget::showSearch(const QString &query) {
    if (query.isEmpty()) {
        if (stackedWidget->currentWidget() == searchView) restoreSavedView();
        return;
    }

    if (!searchView) setupSearch();

    showWidget(searchView, true);
    finderBar->setCheckedAction(-1);
    searchModel->search(query);
}

void FinderWidget::showWidget(QWidget *widget, bool isRoot) {
    // breadcrumb behaviour
    if (isRoot) {
        history.clear();
        breadcrumb->clear();
        folderBreadcrumb->clear();
        folderBreadcrumb->hide();
    } else {
        breadcrumb->addItem(widget->windowTitle());
    }
    breadcrumb->setVisible(!isRoot);

    // call disappear() on previous widget
    QWidget *currentWidget = stackedWidget->currentWidget();
    if (currentWidget && currentWidget != widget) {
        bool ret = QMetaObject::invokeMethod(currentWidget, "disappear", Qt::DirectConnection);
        if (!ret) qDebug() << "FinderWidget::showWidget invokeMethod failed for" << currentWidget;
    }

    // call appear() on new widget
    bool ret = QMetaObject::invokeMethod(widget, "appear", Qt::DirectConnection);
    if (!ret) qDebug() << "FinderWidget::showWidget invokeMethod failed for" << widget;

    stackedWidget->setCurrentWidget(widget);
    history.push(widget);

    widget->setFocus();
}

void FinderWidget::goBack() {
    if (history.size() > 1) {
        breadcrumb->goBack();
        breadcrumb->goBack();
        history.pop();
        QWidget *widget = history.pop();
        bool isRoot = history.isEmpty();
        showWidget(widget, isRoot);
        if (genresListView && widget == genresListView) {
            genresListView->setRootIndex(genresListView->rootIndex().parent());
        }
    }
}

void FinderWidget::folderGoBack() {
    folderBreadcrumb->goBack();
    QModelIndex index = fileSystemView->rootIndex();
    QSortFilterProxyModel *proxyModel =
            static_cast<QSortFilterProxyModel *>(fileSystemView->model());
    if (proxyModel) {
        index = proxyModel->mapToSource(index);
        QString path = fileSystemModel->filePath(index);
        QDir dir(path);
        dir.cdUp();
        // qDebug() << d << folderListModel->rootPath();
        if (dir.absolutePath() == fileSystemModel->rootDirectory().absolutePath()) {
            folderBreadcrumb->clear();
            folderBreadcrumb->hide();
        }
        index = fileSystemModel->index(dir.absolutePath(), 0);
        qDebug() << dir.absolutePath() << index.isValid();
        index = proxyModel->mapFromSource(index);
        fileSystemView->setRootIndex(index);
    }
}

void FinderWidget::appear() {
    QWidget *currentWidget = stackedWidget->currentWidget();
    if (currentWidget) {
        bool success = QMetaObject::invokeMethod(stackedWidget->currentWidget(), "appear",
                                                 Qt::DirectConnection);
        if (!success) qDebug() << "Error invoking appear() on" << stackedWidget->currentWidget();
    }
}

void FinderWidget::disappear() {
    QWidget *currentWidget = stackedWidget->currentWidget();
    if (currentWidget) {
        bool success = QMetaObject::invokeMethod(stackedWidget->currentWidget(), "disappear",
                                                 Qt::DirectConnection);
        if (!success) qDebug() << "Error invoking disappear() on" << stackedWidget->currentWidget();
    }
}

void FinderWidget::artistActivated(const QModelIndex &index) {
    // get the data object
    const ArtistPointer artistPointer = index.data(Finder::DataObjectRole).value<ArtistPointer>();
    Artist *artist = artistPointer.data();
    if (artist) artistActivated(artist);
}

void FinderWidget::artistActivated(Artist *artist) {
    if (!albumListView) setupAlbums();

    QString qry("select id from albums where artist=%1 and trackCount>0 order by year desc, "
                "trackCount desc");
    qry = qry.arg(artist->getId());
    qDebug() << qry;
    albumListModel->setQuery(qry, Database::instance().getConnection());
    if (albumListModel->lastError().isValid()) qDebug() << albumListModel->lastError();

    albumListView->setWindowTitle(artist->getName());
    albumListView->scrollToTop();
    albumListView->setShowToolBar(false);
    showWidget(albumListView, false);
}

void FinderWidget::artistPlayed(const QModelIndex &index) {
    const ArtistPointer artistPointer = index.data(Finder::DataObjectRole).value<ArtistPointer>();
    Artist *artist = artistPointer.data();
    if (artist) {
        QVector<Track *> tracks = artist->getTracks();
        addTracksAndPlay(tracks);
    }
}

void FinderWidget::albumActivated(Album *album) {
    if (!trackListView) setupTracks();

    QString qry("select id from tracks where album=%1 order by disk, track, path");
    qry = qry.arg(album->getId());
    qDebug() << qry;
    trackListModel->setQuery(qry, Database::instance().getConnection());
    if (trackListModel->lastError().isValid()) qDebug() << trackListModel->lastError();

    trackListView->setWindowTitle(album->getName());
    trackListView->scrollToTop();
    showWidget(trackListView, false);
}

void FinderWidget::albumActivated(const QModelIndex &index) {
    const AlbumPointer albumPointer = index.data(Finder::DataObjectRole).value<AlbumPointer>();
    Album *album = albumPointer.data();
    if (album) albumActivated(album);
}

void FinderWidget::albumPlayed(const QModelIndex &index) {
    const AlbumPointer albumPointer = index.data(Finder::DataObjectRole).value<AlbumPointer>();
    Album *album = albumPointer.data();
    if (album) {
        QVector<Track *> tracks = album->getTracks();
        addTracksAndPlay(tracks);
    }
}

void FinderWidget::trackActivated(Track *track) {
    playlistModel->addTrack(track);
    playlistModel->setActiveRow(playlistModel->rowForTrack(track));
}

void FinderWidget::trackActivated(const QModelIndex &index) {
    const TrackPointer trackPointer = index.data(Finder::DataObjectRole).value<TrackPointer>();
    Track *track = trackPointer.data();
    if (track) trackActivated(track);
}

void FinderWidget::folderActivated(const QModelIndex &index) {
    const TrackPointer trackPointer = index.data(Finder::DataObjectRole).value<TrackPointer>();
    Track *track = trackPointer.data();
    if (track) {
        addTracksAndPlay(track->getTracks());
    } else {
        fileSystemView->setRootIndex(index);
        const FolderPointer folderPointer =
                index.data(Finder::DataObjectRole).value<FolderPointer>();
        Folder *folder = folderPointer.data();
        if (folder) {
            folderBreadcrumb->addItem(folder->getName());
            folderBreadcrumb->setVisible(true);
        }
    }
}

void FinderWidget::folderPlayed(const QModelIndex &index) {
    const FolderPointer folderPointer = index.data(Finder::DataObjectRole).value<FolderPointer>();
    Folder *folder = folderPointer.data();
    if (folder) {
        QVector<Track *> tracks = folder->getTracks();
        addTracksAndPlay(tracks);
    } else {
        const TrackPointer trackPointer = index.data(Finder::DataObjectRole).value<TrackPointer>();
        Track *track = trackPointer.data();
        if (track) {
            addTracksAndPlay(track->getTracks());
        }
    }
}

void FinderWidget::genreActivated(const QModelIndex &index) {
    qDebug() << "Activating index" << index;
    Genre *genre = index.data(Finder::DataObjectRole).value<GenrePointer>().data();
    if (genre && !genre->getChildren().isEmpty()) {
        genresListView->setWindowTitle(genre->getName());
        genresListView->setRootIndex(index);
        showWidget(genresListView, false);
    }
}

void FinderWidget::genrePlayed(const QModelIndex &index) {
    Item *item = index.data(Finder::ItemObjectRole).value<ItemPointer>().data();
    if (item) addTracksAndPlay(item->getTracks());
}

void FinderWidget::addTracksAndPlay(const QVector<Track *> &tracks) {
    if (tracks.isEmpty()) return;
    playlistModel->addTracks(tracks);

    Track *trackToPlay = nullptr;

    QSettings settings;
    const bool shuffle = settings.value("shuffle").toBool();
    if (shuffle) {
        int nextRow = (int)((float)qrand() / (float)RAND_MAX * tracks.size());
        trackToPlay = tracks.at(nextRow);
    } else {
        trackToPlay = tracks.first();
    }
    playlistModel->setActiveRow(playlistModel->rowForTrack(trackToPlay));
    playlistView->scrollTo(playlistModel->indexForTrack(trackToPlay),
                           QAbstractItemView::PositionAtCenter);
}
