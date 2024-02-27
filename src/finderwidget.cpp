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

#include "mainwindow.h"
#include "messagebar.h"
#ifdef UPDATER
#include "updater.h"
#endif

namespace {
const char *finderViewKey = "finderView";
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

    QBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    maybeShowMessage();

    setupBar();
    layout->addWidget(bar);

    breadcrumb = new Breadcrumb(this);
    breadcrumb->hide();
    connect(breadcrumb, &Breadcrumb::goneBack, this, &FinderWidget::goBack);
    layout->addWidget(breadcrumb);

    folderBreadcrumb = new Breadcrumb(this);
    folderBreadcrumb->hide();
    connect(folderBreadcrumb, &Breadcrumb::goneBack, this, &FinderWidget::folderGoBack);
    layout->addWidget(folderBreadcrumb);

    stackedWidget = new QStackedWidget(this);

    layout->addWidget(stackedWidget);

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
}

void FinderWidget::setupBar() {
    bar = new SegmentedControl(this);

    artistsAction = new QAction(tr("Artists"), this);
    artistsAction->setShortcut(Qt::CTRL | Qt::Key_1);
    connect(artistsAction, &QAction::triggered, this, &FinderWidget::showArtists);
    bar->addAction(artistsAction);

    albumsAction = new QAction(tr("Albums"), this);
    albumsAction->setShortcut(Qt::CTRL | Qt::Key_2);
    connect(albumsAction, &QAction::triggered, this, &FinderWidget::showAlbums);
    bar->addAction(albumsAction);

    genresAction = new QAction(tr("Genres"), this);
    genresAction->setShortcut(Qt::CTRL | Qt::Key_3);
    connect(genresAction, &QAction::triggered, this, &FinderWidget::showGenres);
    bar->addAction(genresAction);

    foldersAction = new QAction(tr("Folders"), this);
    foldersAction->setShortcut(Qt::CTRL | Qt::Key_4);
    connect(foldersAction, SIGNAL(triggered()), SLOT(showFolders()));
    bar->addAction(foldersAction);

    for (auto a : bar->actions()) {
        addAction(a);
        a->setAutoRepeat(false);
        if (!a->shortcut().isEmpty())
            a->setStatusTip(a->text() + " (" + a->shortcut().toString(QKeySequence::NativeText) +
                            ")");
    }

    bar->setCheckedAction(0);
}

void FinderWidget::setupArtists() {
    artistListModel = new ArtistSqlModel(this);
    artistListView = new ArtistListView(stackedWidget);
    artistListView->setEnabled(false);
    connect(artistListView, &QAbstractItemView::activated, this,
            [this](auto i) { artistActivated(i); });
    connect(artistListView, &FinderListView::play, this, &FinderWidget::artistPlayed);
    artistListView->setModel(artistListModel);
    stackedWidget->addWidget(artistListView);
}

void FinderWidget::setupAlbums() {
    albumListModel = new AlbumSqlModel(this);
    albumListView = new AlbumListView(stackedWidget);
    albumListView->setEnabled(false);
    connect(albumListView, &QAbstractItemView::activated, this,
            [this](auto i) { albumActivated(i); });
    connect(albumListView, &FinderListView::play, this, &FinderWidget::albumPlayed);
    albumListView->setModel(albumListModel);
    stackedWidget->addWidget(albumListView);
}

void FinderWidget::setupGenres() {
    genresModel = new GenresModel(this);
    genresListView = new FinderListView(stackedWidget);
    genresListView->setEnabled(false);
    connect(genresListView, &QAbstractItemView::activated, this, &FinderWidget::genreActivated);
    connect(genresListView, &FinderListView::play, this, &FinderWidget::genrePlayed);
    genresListView->setModel(genresModel);
    stackedWidget->addWidget(genresListView);
}

void FinderWidget::setupTracks() {
    trackListModel = new TrackSqlModel(this);
    trackListView = new TrackListView(stackedWidget);
    connect(trackListView, &QAbstractItemView::activated, this,
            [this](auto i) { trackActivated(i); });
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
    connect(fileSystemView, &QAbstractItemView::activated, this, &FinderWidget::folderActivated);
    connect(fileSystemView, &FinderListView::play, this, &FinderWidget::folderPlayed);
    fileSystemView->setModel(filteringFileSystemModel);
    fileSystemView->setFileSystemModel(fileSystemModel);
    stackedWidget->addWidget(fileSystemView);
}

void FinderWidget::setupSearch() {
    searchModel = new SearchModel(this);
    searchView = new SearchView(stackedWidget);
    connect(searchView, &QAbstractItemView::activated, searchModel, &SearchModel::itemActivated);
    connect(searchView, &FinderListView::play, searchModel, &SearchModel::itemPlayed);
    searchView->setEnabled(false);
    searchView->setModel(searchModel);
    stackedWidget->addWidget(searchView);
}

void FinderWidget::showArtists() {
    if (!artistListView) setupArtists();
    artistListView->updateQuery();
    showWidget(artistListView, true);
    bar->setCheckedAction(artistsAction);
    QSettings settings;
    settings.setValue(finderViewKey, "artists");
}

void FinderWidget::showAlbums() {
    if (!albumListView) setupAlbums();
    albumListView->updateQuery();
    albumListView->setShowToolBar(true);
    showWidget(albumListView, true);
    bar->setCheckedAction(albumsAction);
    QSettings settings;
    settings.setValue(finderViewKey, "albums");
}

void FinderWidget::showGenres() {
    if (!genresListView) setupGenres();
    if (genresListView->rootIndex().isValid()) genresListView->setRootIndex(QModelIndex());
    showWidget(genresListView, true);
    bar->setCheckedAction(genresAction);
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
    bar->setCheckedAction(foldersAction);
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
    bar->setCheckedAction(-1);
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
        QList<Track *> tracks = artist->getTracks();
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
        QList<Track *> tracks = album->getTracks();
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
        QList<Track *> tracks = folder->getTracks();
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

void FinderWidget::addTracksAndPlay(const QList<Track *> &tracks) {
    if (tracks.isEmpty()) return;
    playlistModel->addTracks(tracks);

    Track *trackToPlay = nullptr;

    QSettings settings;
    const bool shuffle = settings.value("shuffle").toBool();
    if (shuffle) {
        int nextRow = (int)((float)rand() / (float)RAND_MAX * tracks.size());
        trackToPlay = tracks.at(nextRow);
    } else {
        trackToPlay = tracks.first();
    }
    playlistModel->setActiveRow(playlistModel->rowForTrack(trackToPlay));
    playlistView->scrollTo(playlistModel->indexForTrack(trackToPlay),
                           QAbstractItemView::PositionAtCenter);
}

void FinderWidget::maybeShowMessage() {
    auto createMessageBar = [this](QString msg) {
        auto mb = new MessageBar(this);
        int margin = 8;
        mb->setContentsMargins(margin, margin, margin, margin);
        mb->setMessage(msg);
        layout()->addWidget(mb);
        connect(mb, &MessageBar::closed, mb, &MessageBar::deleteLater);
        return mb;
    };

    QSettings settings;
    QString key;

    bool showMessages = settings.contains("geometry");

#ifndef APP_WIN
    if (showMessages && !settings.contains(key = "sofa")) {
        QString msg = tr("Need a remote control for %1? Try %2!")
                              .arg(QGuiApplication::applicationDisplayName(), "Sofa");
        auto messageBar = createMessageBar(msg);
        connect(messageBar, &MessageBar::clicked, this, [key] {
            QString url = "https://" + QCoreApplication::organizationDomain() + '/' + key;
            QDesktopServices::openUrl(url);
        });
        connect(messageBar, &MessageBar::closed, this, [key] {
            QSettings settings;
            settings.setValue(key, true);
        });
        showMessages = false;
    }
#endif

    if (showMessages) {
        key = "donate" + QCoreApplication::applicationVersion();
        if (!settings.contains(key)) {
            QString msg = tr("I keep improving %1 to make it the best I can. Support this work!")
                                  .arg(QGuiApplication::applicationDisplayName());
            auto messageBar = createMessageBar(msg);
            connect(messageBar, &MessageBar::clicked, this,
                    [] { MainWindow::instance()->donate(); });
            connect(messageBar, &MessageBar::closed, this, [key] {
                QSettings settings;
                settings.setValue(key, true);
            });
        }
    }

#ifdef UPDATER
    auto onUpdateStatusChange = [this, createMessageBar](auto status) {
        qDebug() << status;
        if (status == Updater::Status::UpdateDownloaded) {
            QString msg = tr("An update is ready to be installed. Quit and install update.");
            auto messageBar = createMessageBar(msg);
            connect(messageBar, &MessageBar::clicked, this, [] { qApp->quit(); });
        }
    };
    connect(&Updater::instance(), &Updater::statusChanged, this, onUpdateStatusChange);
    onUpdateStatusChange(Updater::instance().getStatus());
#endif
}
