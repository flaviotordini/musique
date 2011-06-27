#include "searchmodel.h"

#include "model/artist.h"
#include "model/album.h"

#include "artistsqlmodel.h"
#include "albumsqlmodel.h"
#include "tracksqlmodel.h"
#include "filesystemmodel.h"
#include "filteringfilesystemmodel.h"

#include "database.h"
#include "trackmimedata.h"

#include "finderwidget.h"

SearchModel::SearchModel(QObject *parent) : QAbstractListModel(parent) {

    finder = dynamic_cast<FinderWidget*>(parent);

    artistListModel = new ArtistSqlModel(this);
    albumListModel = new AlbumSqlModel(this);
    trackListModel = new TrackSqlModel(this);

    fileSystemModel = new FileSystemModel(this);
    fileSystemModel->setResolveSymlinks(true);
    fileSystemModel->setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    FilteringFileSystemModel *proxyModel = new FilteringFileSystemModel(this);
    proxyModel->setSourceModel(fileSystemModel);

    hoveredRow = -1;
    playIconHovered = false;

    timeLine = new QTimeLine(250, this);
    timeLine->setFrameRange(1000, 0);
    connect(timeLine, SIGNAL(frameChanged(int)), SLOT(updatePlayIcon()));

}

int SearchModel::rowCount(const QModelIndex &parent) const {
    return artistListModel->rowCount(parent)
            + albumListModel->rowCount(parent)
            + trackListModel->rowCount(parent);
}

QVariant SearchModel::data(const QModelIndex &index, int role) const {

    const int row = index.row();
    const int artistRowCount = artistListModel->rowCount(index.parent());
    const int albumRowCount = albumListModel->rowCount(index.parent());
    const int trackRowCount = trackListModel->rowCount(index.parent());

    switch (role) {

    case Finder::HoveredItemRole:
        return hoveredRow == row;

    case Finder::PlayIconAnimationItemRole:
        return timeLine->currentFrame() / 1000.;

    case Finder::PlayIconHoveredRole:
        return playIconHovered;

    default:
        if (row >= 0 && row < artistRowCount) {
            return artistListModel->data(index, role);
        } else if (row >= artistRowCount && row < artistRowCount + albumRowCount) {
            return albumListModel->data(createIndex( row - artistRowCount, index.column() ), role);
        } else if (row >= artistRowCount + albumRowCount && row < artistRowCount + albumRowCount + trackRowCount) {
            return trackListModel->data(createIndex( row - artistRowCount - albumRowCount, index.column() ), role);
        }

    }

    return QVariant();
}

void SearchModel::search(QString query) {
    QString likeQuery = "%" + query + "%";


    QSqlQuery q(Database::instance().getConnection());
    q.prepare("select id from artists where name like ? and trackCount>1 order by trackCount desc");
    q.bindValue(0, likeQuery);
    q.exec();
    artistListModel->setQuery(q);
    if (artistListModel->lastError().isValid())
        qDebug() << artistListModel->lastError();

    q.prepare("select id from albums where (title like ? or year=?) and trackCount>0 order by year desc, trackCount desc");
    q.bindValue(0, likeQuery);
    q.bindValue(1, query);
    q.exec();
    albumListModel->setQuery(q);
    if (albumListModel->lastError().isValid())
        qDebug() << albumListModel->lastError();

    q.prepare("select id from tracks where title like ? order by track, path");
    q.bindValue(0, likeQuery);
    q.exec();
    trackListModel->setQuery(q);
    if (trackListModel->lastError().isValid())
        qDebug() << trackListModel->lastError();

    reset();

}

Item* SearchModel::itemAt(const QModelIndex &index) const {
    Item *item = 0;

    int itemType = index.data(Finder::ItemTypeRole).toInt();
    if (itemType == Finder::ItemTypeArtist) {
        const ArtistPointer pointer = index.data(Finder::DataObjectRole).value<ArtistPointer>();
        item = dynamic_cast<Item*>(pointer.data());
    } else if (itemType == Finder::ItemTypeAlbum) {
        const AlbumPointer pointer = index.data(Finder::DataObjectRole).value<AlbumPointer>();
        item = dynamic_cast<Item*>(pointer.data());
    } else if (itemType == Finder::ItemTypeFolder) {

    } else if (itemType == Finder::ItemTypeTrack) {
        const TrackPointer pointer = index.data(Finder::DataObjectRole).value<TrackPointer>();
        item = dynamic_cast<Item*>(pointer.data());
    }

    return item;
}

// --- Events ---

void SearchModel::itemEntered ( const QModelIndex & index ) {
    this->setHoveredRow(index.row());
}

void SearchModel::itemActivated ( const QModelIndex & index ) {
    int itemType = index.data(Finder::ItemTypeRole).toInt();
    if (itemType == Finder::ItemTypeArtist) {
        const ArtistPointer pointer = index.data(Finder::DataObjectRole).value<ArtistPointer>();
        finder->artistActivated(pointer.data());
    } else if (itemType == Finder::ItemTypeAlbum) {
        const AlbumPointer pointer = index.data(Finder::DataObjectRole).value<AlbumPointer>();
        finder->albumActivated(pointer.data());
    } else if (itemType == Finder::ItemTypeFolder) {

    } else if (itemType == Finder::ItemTypeTrack) {
        const TrackPointer pointer = index.data(Finder::DataObjectRole).value<TrackPointer>();
        finder->trackActivated(pointer.data());
    }
}

void SearchModel::itemPlayed ( const QModelIndex & index ) {
    Item *item = itemAt(index);
    if (!item) return;
    QList<Track*> tracks = item->getTracks();
    finder->addTracksAndPlay(tracks);
}

// --- Hover ---

void SearchModel::setHoveredRow(int row) {
    int oldRow = hoveredRow;
    hoveredRow = row;
    emit dataChanged( createIndex( oldRow, 0 ), createIndex( oldRow, columnCount()-1 ) );
    emit dataChanged( createIndex( hoveredRow, 0 ), createIndex( hoveredRow, columnCount()-1 ) );
}

void SearchModel::clearHover() {
    emit dataChanged( createIndex( hoveredRow, 0 ), createIndex( hoveredRow, columnCount()-1 ) );
    hoveredRow = -1;
    // timeLine->stop();
}

void SearchModel::enterPlayIconHover() {
    if (playIconHovered) return;
    playIconHovered = true;
    if (timeLine->state() != QTimeLine::Running) {
        timeLine->setDirection(QTimeLine::Forward);
        timeLine->start();
    }
}

void SearchModel::exitPlayIconHover() {
    if (!playIconHovered) return;
    playIconHovered = false;
    if (timeLine->state() == QTimeLine::Running) {
        timeLine->stop();
        timeLine->setDirection(QTimeLine::Backward);
        timeLine->start();
    }
    setHoveredRow(hoveredRow);
}

void SearchModel::updatePlayIcon() {
    emit dataChanged( createIndex( hoveredRow, 0 ), createIndex( hoveredRow, columnCount()-1 ) );
}

// --- Sturm und drang ---

Qt::DropActions SearchModel::supportedDropActions() const {
    return Qt::MoveAction;
}

Qt::ItemFlags SearchModel::flags(const QModelIndex &index) const {
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
    if (index.isValid()) {
        return ( defaultFlags | Qt::ItemIsDragEnabled );
    } else
        return defaultFlags;
}

QStringList SearchModel::mimeTypes() const {
    QStringList types;
    types << TRACK_MIME;
    return types;
}

QMimeData* SearchModel::mimeData( const QModelIndexList &indexes ) const {

    TrackMimeData* mime = new TrackMimeData();

    foreach( const QModelIndex &index, indexes ) {
        Item *item = itemAt(index);
        if (item) {
            // qDebug() << item->getTracks();
            mime->addTracks(item->getTracks());
        }
    }

    return mime;
}
