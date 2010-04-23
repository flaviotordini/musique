#include "finderwidget.h"

#include "thlibrary/thblackbar.h"

#include "model/artist.h"
#include "model/album.h"

#include "artistlistview.h"
#include "artistsqlmodel.h"

#include "albumlistview.h"
#include "albumsqlmodel.h"

#include "tracklistview.h"
#include "tracksqlmodel.h"

#include "filesystemfinderview.h"
#include "filesystemmodel.h"

#include "finderhomewidget.h"

#include "database.h"
#include <QtSql>

static const char* FINDER_VIEW_KEY = "finderView";

FinderWidget::FinderWidget(QWidget *parent) : QWidget(parent) {

    folderListView = 0;
    artistListView = 0;
    albumListView = 0;
    trackListView = 0;

    // colors
    QPalette p = palette();
    p.setBrush(QPalette::Window, Qt::black);
    p.setBrush(QPalette::Text, Qt::white);
    p.setBrush(QPalette::WindowText, Qt::white);
    p.setBrush(QPalette::ButtonText, Qt::white);
    setPalette(p);
    setAutoFillBackground(true);

    setMinimumWidth(150);

    QBoxLayout *layout = new QVBoxLayout();
    layout->setMargin(0);
    layout->setSpacing(0);

    // Finder Bar
    finderBar = new THBlackBar(this);

    artistsAction = new QAction(tr("Artists"), this);
    QKeySequence keySequence(Qt::CTRL + Qt::Key_1);
    artistsAction->setShortcut(keySequence);
    artistsAction->setStatusTip(artistsAction->text() + " (" +
                                keySequence.toString(QKeySequence::NativeText) + ")");
    addAction(artistsAction);
    connect(artistsAction, SIGNAL(triggered()), SLOT(showArtists()));
    finderBar->addAction(artistsAction);

    albumsAction = new QAction(tr("Albums"), this);
    keySequence = QKeySequence(Qt::CTRL + Qt::Key_2);
    albumsAction->setShortcut(keySequence);
    albumsAction->setStatusTip(albumsAction->text() + " (" +
                               keySequence.toString(QKeySequence::NativeText) + ")");
    addAction(albumsAction);
    connect(albumsAction, SIGNAL(triggered()), SLOT(showAlbums()));
    finderBar->addAction(albumsAction);

    foldersAction = new QAction(tr("Folders"), this);
    keySequence = QKeySequence(Qt::CTRL + Qt::Key_3);
    foldersAction->setShortcut(keySequence);
    foldersAction->setStatusTip(foldersAction->text() + " (" +
                                keySequence.toString(QKeySequence::NativeText) + ")");
    addAction(foldersAction);
    connect(foldersAction, SIGNAL(triggered()), SLOT(showFolders()));
    finderBar->addAction(foldersAction);

    finderBar->setCheckedAction(0);
    layout->addWidget(finderBar);

    breadcrumb = new BreadcrumbWidget(this);
    breadcrumb->hide();
    layout->addWidget(breadcrumb);

    stackedWidget = new QStackedWidget(this);

    layout->addWidget(stackedWidget);
    setLayout(layout);

    // Restore saved view
    QSettings settings;
    QString currentViewName = settings.value(FINDER_VIEW_KEY).toString();
    if (currentViewName == "folders") showFolders();
    else if (currentViewName == "albums") showAlbums();
    else showArtists();

}

void FinderWidget::setupArtists() {
    artistListModel = new ArtistSqlModel(this);
    artistListView = new ArtistListView(this);
    connect(artistListView, SIGNAL(activated(const QModelIndex &)), SLOT(artistActivated(const QModelIndex &)));
    connect(artistListView, SIGNAL(play(const QModelIndex &)), SLOT(artistPlayed(const QModelIndex &)));
    connect(artistListView, SIGNAL(entered(const QModelIndex &)), SLOT(artistEntered(const QModelIndex &)));
    connect(artistListView, SIGNAL(viewportEntered()), artistListModel, SLOT(clearHover()));
    artistListView->setModel(artistListModel);
    stackedWidget->addWidget(artistListView);
}

void FinderWidget::setupAlbums() {
    albumListModel = new AlbumSqlModel(this);
    albumListView = new AlbumListView(this);
    connect(albumListView, SIGNAL(activated(const QModelIndex &)), SLOT(albumActivated(const QModelIndex &)));
    connect(albumListView, SIGNAL(play(const QModelIndex &)), SLOT(albumPlayed(const QModelIndex &)));
    connect(albumListView, SIGNAL(entered(const QModelIndex &)), SLOT(albumEntered(const QModelIndex &)));
    connect(albumListView, SIGNAL(viewportEntered()), albumListModel, SLOT(clearHover()));
    albumListView->setModel(albumListModel);
    stackedWidget->addWidget(albumListView);
}

void FinderWidget::setupTracks() {
    trackListModel = new TrackSqlModel(this);
    trackListView = new TrackListView(this);
    connect(trackListView, SIGNAL(activated(const QModelIndex &)), SLOT(trackActivated(const QModelIndex &)));
    connect(trackListView, SIGNAL(play(const QModelIndex &)), SLOT(trackPlayed(const QModelIndex &)));
    connect(trackListView, SIGNAL(entered(const QModelIndex &)), SLOT(trackEntered(const QModelIndex &)));
    connect(trackListView, SIGNAL(viewportEntered()), trackListModel, SLOT(clearHover()));
    trackListView->setModel(trackListModel);
    stackedWidget->addWidget(trackListView);
}

void FinderWidget::setupFolders() {
    folderListModel = new FileSystemModel(this);
    folderListView = new FileSystemFinderView(this);
    connect(folderListView, SIGNAL(activated(const QModelIndex &)), SLOT(folderActivated(const QModelIndex &)));
    connect(folderListView, SIGNAL(play(const QModelIndex &)), SLOT(folderPlayed(const QModelIndex &)));
    connect(folderListView, SIGNAL(entered(const QModelIndex &)), SLOT(folderEntered(const QModelIndex &)));
    connect(folderListView, SIGNAL(viewportEntered()), folderListModel, SLOT(clearHover()));
    folderListView->setModel(folderListModel);
    stackedWidget->addWidget(folderListView);
}

void FinderWidget::showArtists() {
    if (!artistListView) setupArtists();
    showWidget(artistListView, true);
    finderBar->setCheckedAction(artistsAction);
    QSettings settings;
    settings.setValue(FINDER_VIEW_KEY, "artists");
}

void FinderWidget::showAlbums() {
    if (!albumListView) setupAlbums();
    QString qry("select id from albums order by artist, year desc, trackCount desc");
    albumListModel->setQuery(qry, Database::instance().getConnection());
    if (albumListModel->lastError().isValid())
        qDebug() << albumListModel->lastError();
    showWidget(albumListView, true);
    finderBar->setCheckedAction(albumsAction);
    QSettings settings;
    settings.setValue(FINDER_VIEW_KEY, "albums");
}

void FinderWidget::showFolders() {
    if (!folderListView) setupFolders();

    showWidget(folderListView, true);
    finderBar->setCheckedAction(foldersAction);
    QSettings settings;
    settings.setValue(FINDER_VIEW_KEY, "folders");
}

void FinderWidget::showWidget(QWidget *widget, bool isRoot) {
    // breadcrumb behaviour
    if (isRoot) {
        breadcrumb->clear();
    } else {
        breadcrumb->addWidget(widget);
    }
    breadcrumb->setVisible(!isRoot);


    bool ret = QMetaObject::invokeMethod(widget, "appear", Qt::DirectConnection);
    if (!ret) qDebug() << "FinderWidget::showWidget invokeMethod failed for" << widget;
    stackedWidget->setCurrentWidget(widget);
}

void FinderWidget::appear() {
    bool success = QMetaObject::invokeMethod(stackedWidget->currentWidget(), "appear", Qt::DirectConnection);
    if (!success) qDebug() << "Error invoking appear() on" << stackedWidget->currentWidget();
}

void FinderWidget::artistEntered ( const QModelIndex & index ) {
    artistListModel->setHoveredRow(index.row());
}

void FinderWidget::artistActivated ( const QModelIndex & index ) {
    if (!albumListView) setupAlbums();

    // get the data object
    const ArtistPointer artistPointer = index.data(Finder::DataObjectRole).value<ArtistPointer>();
    Artist *artist = artistPointer.data();

    QString qry("select id from albums where artist=%1 order by year desc, trackCount desc");
    qry = qry.arg(artist->getId());
    qDebug() << qry;
    albumListModel->setQuery(qry, Database::instance().getConnection());
    if (albumListModel->lastError().isValid())
        qDebug() << albumListModel->lastError();

    albumListView->setWindowTitle(artist->getName());

    showWidget(albumListView, false);
}

void FinderWidget::artistPlayed ( const QModelIndex & index ) {
    const ArtistPointer artistPointer = index.data(Finder::DataObjectRole).value<ArtistPointer>();
    Artist *artist = artistPointer.data();
    QList<Track*> tracks = artist->getTracks();
    if (tracks.isEmpty()) return;
    playlistModel->addTracks(tracks);
    playlistModel->setActiveRow(playlistModel->rowForTrack(tracks.first()));
}

void FinderWidget::albumEntered ( const QModelIndex & index ) {
    albumListModel->setHoveredRow(index.row());
}

void FinderWidget::albumActivated ( const QModelIndex & index ) {
    if (!trackListView) setupTracks();

    const AlbumPointer albumPointer = index.data(Finder::DataObjectRole).value<AlbumPointer>();
    Album *album = albumPointer.data();

    QString qry("select id from tracks where album=%1 order by track, title");
    qry = qry.arg(album->getId());
    qDebug() << qry;
    trackListModel->setQuery(qry, Database::instance().getConnection());
    if (trackListModel->lastError().isValid())
        qDebug() << trackListModel->lastError();

    trackListView->setWindowTitle(album->getName());
    showWidget(trackListView, false);

}

void FinderWidget::albumPlayed ( const QModelIndex & index ) {
    const AlbumPointer albumPointer = index.data(Finder::DataObjectRole).value<AlbumPointer>();
    Album *album = albumPointer.data();
    QList<Track*> tracks = album->getTracks();
    if (tracks.isEmpty()) return;
    playlistModel->addTracks(tracks);
    playlistModel->setActiveRow(playlistModel->rowForTrack(tracks.first()));
}

void FinderWidget::trackEntered ( const QModelIndex & index ) {
    trackListModel->setHoveredRow(index.row());
}

void FinderWidget::trackActivated ( const QModelIndex & index ) {
    const TrackPointer trackPointer = index.data(Finder::DataObjectRole).value<TrackPointer>();
    Track *track = trackPointer.data();
    playlistModel->addTrack(track);
    playlistModel->setActiveRow(playlistModel->rowForTrack(track));
}

void FinderWidget::folderEntered ( const QModelIndex & index ) {
    folderListModel->setHoveredRow(index.row());
}

void FinderWidget::folderActivated(const QModelIndex & index) {
    // QFileSystemModel *fileSystemModel = static_cast<QFileSystemModel*>(folderListView->model());
    // if (!fileSystemModel) return;
    // const FolderPointer folderPointer = index.data(Finder::DataObjectRole).value<FolderPointer>();
    // Folder *folder = folderPointer.data();
    // QString path = folder->getAbsolutePath();
    // qDebug() << "Changing to" << path;
    folderListView->setRootIndex(index);
}

void FinderWidget::folderPlayed(const QModelIndex & index) {
    const FolderPointer folderPointer = index.data(Finder::DataObjectRole).value<FolderPointer>();
    Folder *folder = folderPointer.data();
    QList<Track*> tracks = folder->getTracks();
    if (tracks.isEmpty()) return;
    playlistModel->addTracks(tracks);
    playlistModel->setActiveRow(playlistModel->rowForTrack(tracks.first()));
}
