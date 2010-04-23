#include "filesystemmodel.h"
#include "trackmimedata.h"

FileSystemModel::FileSystemModel(QObject *parent) : QFileSystemModel(parent) {
    hoveredRow = -1;
    playIconHovered = false;

    timeLine = new QTimeLine(250, this);
    timeLine->setFrameRange(1000, 0);
    connect(timeLine, SIGNAL(frameChanged(int)), SLOT(updatePlayIcon()));
}

QVariant FileSystemModel::data(const QModelIndex &index, int role) const {

    Folder *folder = 0;
    Track *track = 0;
    QString path;

    switch (role) {

    case Finder::ItemTypeRole:
        return Finder::ItemTypeFolder;

    case Finder::DataObjectRole:
        path = QFileSystemModel::data(index, QFileSystemModel::FilePathRole).toString();
        qDebug() << "model path" << path << path.isEmpty();
        if (isDir(index)) {
            folder = Folder::forPath(path);
            return QVariant::fromValue(QPointer<Folder>(folder));
        } else {
            track = Track::forPath(path);
            return QVariant::fromValue(QPointer<Track>(track));
        }

    case Finder::HoveredItemRole:
        return hoveredRow == index.row();

    case Finder::PlayIconAnimationItemRole:
        return timeLine->currentFrame() / 1000.;

    case Finder::PlayIconHoveredRole:
        return playIconHovered;

    default:
        return QFileSystemModel::data(index, role);

    }

    return QVariant();
}

void FileSystemModel::setHoveredRow(int row) {
    int oldRow = hoveredRow;
    hoveredRow = row;
    emit dataChanged( createIndex( oldRow, 0 ), createIndex( oldRow, columnCount() - 1 ) );
    emit dataChanged( createIndex( hoveredRow, 0 ), createIndex( hoveredRow, columnCount() - 1 ) );
}

void FileSystemModel::clearHover() {
    emit dataChanged( createIndex( hoveredRow, 0 ), createIndex( hoveredRow, columnCount() - 1 ) );
    hoveredRow = -1;
}

void FileSystemModel::enterPlayIconHover() {
    if (playIconHovered) return;
    playIconHovered = true;
    if (timeLine->state() != QTimeLine::Running) {
        timeLine->setDirection(QTimeLine::Forward);
        timeLine->start();
    }
}

void FileSystemModel::exitPlayIconHover() {
    if (!playIconHovered) return;
    playIconHovered = false;
    if (timeLine->state() == QTimeLine::Running) {
        timeLine->stop();
        timeLine->setDirection(QTimeLine::Backward);
        timeLine->start();
    }
    setHoveredRow(hoveredRow);
}

void FileSystemModel::updatePlayIcon() {
    emit dataChanged( createIndex( hoveredRow, 0 ), createIndex( hoveredRow, columnCount() - 1 ) );
}

// --- Sturm und drang ---

Qt::DropActions FileSystemModel::supportedDropActions() const {
    return Qt::MoveAction;
}

Qt::ItemFlags FileSystemModel::flags(const QModelIndex &index) const {
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
    if (index.isValid()) {
        return ( defaultFlags | Qt::ItemIsDragEnabled );
    } else
        return defaultFlags;
}

QStringList FileSystemModel::mimeTypes() const {
    QStringList types;
    types << TRACK_MIME;
    return types;
}

QMimeData* FileSystemModel::mimeData( const QModelIndexList &indexes ) const {

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
