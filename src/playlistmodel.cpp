#include "playlistmodel.h"
#include "trackmimedata.h"
#include "model/album.h"
#include "model/artist.h"

namespace The {
    QMap<QString, QAction*>* globalActions();
}

PlaylistModel::PlaylistModel(QWidget *parent) : QAbstractTableModel(parent) {
    m_activeTrack = 0;
    m_activeRow = -1;
}

PlaylistModel::~PlaylistModel() {

}

QVariant PlaylistModel::headerData(int section, Qt::Orientation orientation, int role) const {

    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();

    switch(section) {
    case Playlist::TrackColumn: return tr("Track");
    case Playlist::TitleColumn: return tr("Title");
    case Playlist::AlbumColumn: return tr("Album");
    case Playlist::ArtistColumn: return tr("Artist");
        // case 4: return tr("Year");
    case Playlist::LengthColumn: return tr("Length");
    default: return "";
    }

}

int PlaylistModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return tracks.size();
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const {

    QVariant nullVariant;

    const int row = index.row();
    if (row < 0 || row >= tracks.size())
        return nullVariant;

    Track *track = tracks.at(row);

    switch (role) {
    case Playlist::DataObjectRole:
        return QVariant::fromValue(QPointer<Track>(track));
    case Playlist::ActiveItemRole:
        return track == m_activeTrack;
    case Qt::DisplayRole:
        switch(index.column()) {
        case  Playlist::TrackColumn: return track->getNumber() ? track->getNumber() : nullVariant;
        case  Playlist::TitleColumn: return track->getTitle();
        case  Playlist::AlbumColumn:
            return track->getAlbum() ? track->getAlbum()->getTitle() : nullVariant;
        case  Playlist::ArtistColumn:
            return track->getArtist() ? track->getArtist()->getName() : nullVariant;
            // case  Playlist::YearColumn: return track->getYear() ? track->getYear() : nullVariant;
        case  Playlist::LengthColumn: return track->getLength() ? track->getLength() : nullVariant;
        }
    case Qt::TextAlignmentRole:
        if (index.column() == 0) return Qt::AlignCenter;
        else return nullVariant;
    }

    return nullVariant;
}

void PlaylistModel::setActiveRow( int row) {
    if ( rowExists( row ) ) {

        m_activeRow = row;
        m_activeTrack = trackAt(row);

        // setStateOfRow( row, Item::Played );

        int oldactiverow = m_activeRow;

        if ( rowExists( oldactiverow ) )
            emit dataChanged( createIndex( oldactiverow, 0 ), createIndex( oldactiverow, columnCount() - 1 ) );

        emit dataChanged( createIndex( m_activeRow, 0 ), createIndex( m_activeRow, columnCount() - 1 ) );
        emit activeRowChanged(row);

    } else {
        m_activeRow = -1;
        m_activeTrack = 0;
    }

}

int PlaylistModel::previousRow() const {
    int nextRow = m_activeRow - 1;
    if (rowExists(nextRow))
        return nextRow;
    return -1;
}

int PlaylistModel::nextRow() const {
    int nextRow = m_activeRow + 1;
    if (rowExists(nextRow))
        return nextRow;
    return -1;
}

Track* PlaylistModel::trackAt( int row ) const {
    if ( rowExists( row ) )
        return tracks.at( row );
    return 0;
}

Track* PlaylistModel::activeTrack() const {
    return m_activeTrack;
}

void PlaylistModel::addTrack(Track* track) {
    if (!tracks.contains(track)) {
        beginInsertRows(QModelIndex(), tracks.size(), tracks.size());
        tracks << track;
        endInsertRows();
    }
    The::globalActions()->value("clearPlaylist")->setEnabled(true);
}

void PlaylistModel::addTracks(QList<Track*> tracks) {
    if (!tracks.empty()) {
        beginInsertRows(QModelIndex(), this->tracks.size(), this->tracks.size() + tracks.size() - 1);
        this->tracks.append(tracks);
        endInsertRows();
        The::globalActions()->value("clearPlaylist")->setEnabled(true);
    }
}

void PlaylistModel::clear() {
    tracks.clear();
    emit reset();
}

// --- item removal

bool PlaylistModel::removeRows(int position, int rows, const QModelIndex & /*parent*/) {
    beginRemoveRows(QModelIndex(), position, position+rows-1);
    for (int row = 0; row < rows; ++row) {
        tracks.removeAt(position);
    }
    endRemoveRows();
    return true;
}

void PlaylistModel::removeIndexes(QModelIndexList &indexes) {
    QList<Track*> originalList(tracks);
    QList<Track*> delitems;
    foreach (QModelIndex index, indexes) {
        Track* track = originalList.at(index.row());
        int idx = tracks.indexOf(track);
        if (idx != -1) {
            beginRemoveRows(QModelIndex(), idx, idx);
            delitems.append(track);
            tracks.removeAll(track);
            endRemoveRows();
        }
    }
}

// --- Sturm und drang ---

Qt::DropActions PlaylistModel::supportedDropActions() const {
    return Qt::MoveAction;
}

Qt::ItemFlags PlaylistModel::flags(const QModelIndex &index) const {
    Qt::ItemFlags defaultFlags = QAbstractTableModel::flags(index);
    if (index.isValid()) {
        return ( defaultFlags | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled );
    } else
        return Qt::ItemIsDropEnabled | defaultFlags;
}

QStringList PlaylistModel::mimeTypes() const {
    QStringList types;
    types << TRACK_MIME;
    return types;
}

QMimeData* PlaylistModel::mimeData( const QModelIndexList &indexes ) const {
    TrackMimeData* mime = new TrackMimeData();

    foreach( const QModelIndex &it, indexes ) {
        int row = it.row();
        if (row >= 0 && row < tracks.size())
            mime->addTrack( tracks.at( it.row() ) );
    }

    return mime;
}

bool PlaylistModel::dropMimeData(const QMimeData *data,
                                 Qt::DropAction action, int row, int column,
                                 const QModelIndex &parent) {

    if (action == Qt::IgnoreAction)
        return true;

    if (!data->hasFormat(TRACK_MIME))
        return false;

    if (column > 0)
        return false;

    int beginRow;
    if (row != -1)
        beginRow = row;
    else if (parent.isValid())
        beginRow = parent.row();
    else
        beginRow = rowCount();

    const TrackMimeData* trackMimeData = dynamic_cast<const TrackMimeData*>( data );
    if(!trackMimeData ) return false;

    int counter = 0;
    QList<Track*> droppedTracks = trackMimeData->tracks();
    foreach(Track *track, droppedTracks) {

        // remove track
        const int trackRow = tracks.indexOf(track);
        removeRows(trackRow, 1, QModelIndex());

        // and then add it at the new position
        const int targetRow = beginRow + counter;
        beginInsertRows(QModelIndex(), targetRow, targetRow);
        tracks.insert(targetRow, track);
        endInsertRows();
        counter++;

    }

    // fix m_activeRow after all this
    m_activeRow = tracks.indexOf(m_activeTrack);

    The::globalActions()->value("clearPlaylist")->setEnabled(true);

    return true;

}

int PlaylistModel::rowForTrack(Track* track) {
    return tracks.indexOf(track);
}

QModelIndex PlaylistModel::indexForTrack(Track* track) {
    return createIndex(tracks.indexOf(track), 0);
}

void PlaylistModel::move(QModelIndexList &indexes, bool up) {

    QList<Track*> movedTracks;

    foreach (QModelIndex index, indexes) {
        int row = index.row();
        qDebug() << "index row" << row;
        Track *track = trackAt(row);
        movedTracks << track;
    }

    int counter = 1;
    foreach (Track *track, movedTracks) {

        int row = rowForTrack(track);
        qDebug() << "track row" << row;
        removeRows(row, 1, QModelIndex());

        if (up) row--;
        else row++;

        beginInsertRows(QModelIndex(), row, row);
        tracks.insert(row, track);
        endInsertRows();

        counter++;
    }

    emit needSelectionFor(movedTracks);

}

void PlaylistModel::sort(int column, Qt::SortOrder order) {

}
