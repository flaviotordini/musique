#include <algorithm>
#include "playlistmodel.h"
#include "trackmimedata.h"
#include "model/album.h"
#include "model/artist.h"
#include "mainwindow.h"

namespace The {
QMap<QString, QAction*>* globalActions();
}

#ifdef APP_DEMO
    static const int demoMaxTracks = 10;
    static QString demoMessage;
#endif

PlaylistModel::PlaylistModel(QWidget *parent) : QAbstractListModel(parent) {
    activeTrack = 0;
    activeRow = -1;

#ifdef APP_DEMO
    demoMessage = tr("This demo is limited to only %1 tracks in the playlist.").arg(QString::number(demoMaxTracks));
#endif

}

int PlaylistModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    else return tracks.size();
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const {

    const int row = index.row();
    if (row < 0 || row >= tracks.size())
        return QVariant();

    Track *track = tracks.at(row);
    if (!track) return QVariant();

    switch (role) {

    case Playlist::DataObjectRole:
        return QVariant::fromValue(QPointer<Track>(track));

    case Playlist::ActiveItemRole:
        return track == activeTrack;

    }

    return QVariant();
}

void PlaylistModel::setActiveRow(int row, bool manual) {
    if (!rowExists(row)) return;

    const int oldActiveRow = activeRow;
    activeRow = row;
    activeTrack = trackAt(activeRow);
    activeTrack->setPlayed(true);
    playedTracks << activeTrack;

    if (rowExists(oldActiveRow)) {
        QModelIndex oldIndex = index(oldActiveRow, 0, QModelIndex());
        emit dataChanged(oldIndex, oldIndex);
    }

    QModelIndex newIndex = index(activeRow, 0, QModelIndex());
    emit dataChanged(newIndex, newIndex);

    emit activeRowChanged(row, manual);

}

void PlaylistModel::skipBackward() {
    QSettings settings;
    const bool shuffle = settings.value("shuffle").toBool();

    Track *previousTrack = 0;

    if (shuffle) {

        if (playedTracks.size() > 1)
            previousTrack = playedTracks.at(playedTracks.size() - 2);

    } else {

        int prevRow = activeRow - 1;
        if (rowExists(prevRow)) {
            previousTrack = tracks.at(prevRow);
        }

    }

    if (previousTrack) {
        playedTracks.removeAll(previousTrack);
        previousTrack->setPlayed(false);
        playedTracks.removeAll(activeTrack);
        activeTrack->setPlayed(false);
        int prevRow = tracks.indexOf(previousTrack);
        setActiveRow(prevRow);
    }
}

void PlaylistModel::skipForward() {
    QSettings settings;
    const bool shuffle = settings.value("shuffle").toBool();
    const bool repeat = settings.value("repeat").toBool();

    Track *nextTrack = 0;

    if (shuffle) {

        // get a random non-played non-active track
        if (playedTracks.size() < tracks.size()) {
            while (nextTrack == 0) {
                int nextRow = (int) ((float) qrand() / (float) RAND_MAX * tracks.size());
                Track *candidateTrack = tracks.at(nextRow);
                if (!candidateTrack->isPlayed() && candidateTrack != activeTrack) {
                    nextTrack = candidateTrack;
                }
            }
        }

        // repeat
        if (repeat && nextTrack == 0 && !tracks.empty()) {
            playedTracks.clear();
            foreach (Track *track, tracks) {
                track->setPlayed(false);
            }
            // get a random non-active track
            while (nextTrack == 0) {
                int nextRow = (int) ((float) qrand() / (float) RAND_MAX * (tracks.size() - 1));
                Track *candidateTrack = tracks.at(nextRow);
                if (candidateTrack != activeTrack) {
                    nextTrack = candidateTrack;
                }
            }
        }

    } else {

        // normal non-shuffle mode
        int nextRow = activeRow + 1;
        if (rowExists(nextRow)) {
            nextTrack = tracks.at(nextRow);
        } else if (repeat && !tracks.empty()) {
            nextTrack = tracks.first();
        }

    }

    if (nextTrack) {
        int nextRow = tracks.indexOf(nextTrack);
        setActiveRow(nextRow);
    } else {
        activeRow = -1;
        activeTrack = 0;
        foreach(Track *track, playedTracks)
            track->setPlayed(false);
        playedTracks.clear();
        emit playlistFinished();
    }
}

Track* PlaylistModel::trackAt(int row) const {
    if (rowExists(row))
        return tracks.at(row);
    return 0;
}

Track* PlaylistModel::getActiveTrack() const {
    return activeTrack;
}

void PlaylistModel::addShuffledTrack(Track* track) {
    int activeTrackIndex = playedTracks.indexOf(activeTrack);
    if (activeTrackIndex == -1) activeTrackIndex = 0;
    int randomRange = playedTracks.size() - activeTrackIndex;
    int randomRow = activeTrackIndex + (int) ((float) qrand() / (float) RAND_MAX * randomRange);
    playedTracks.insert(randomRow, track);
}

void PlaylistModel::addTrack(Track* track) {
    // no duplicates
    if (!tracks.contains(track)) {

#ifdef APP_DEMO
        if (this->tracks.size() >= demoMaxTracks) {
            MainWindow::instance()->showDemoDialog(demoMessage);
            return;
        }
#endif

        track->setPlayed(false);
        connect(track, SIGNAL(removed()), SLOT(trackRemoved()));
        beginInsertRows(QModelIndex(), tracks.size(), tracks.size());
        tracks << track;
        endInsertRows();
    }
}

void PlaylistModel::addTracks(QList<Track*> tracks) {
    if (!tracks.empty()) {
        // remove duplicates
        foreach(Track* track, this->tracks) {
            tracks.removeAll(track);
        }
        if (!tracks.empty()) {

#ifdef APP_DEMO
            if (this->tracks.size() >= demoMaxTracks) {
                MainWindow::instance()->showDemoDialog(demoMessage);
                return;
            }
#endif

            beginInsertRows(QModelIndex(), this->tracks.size(), this->tracks.size() + tracks.size() - 1);
            // this->tracks.append(tracks);
            foreach(Track* track, tracks) {

#ifdef APP_DEMO
                if (this->tracks.size() >= demoMaxTracks) {
                    endInsertRows();
                    MainWindow::instance()->showDemoDialog(demoMessage);
                    return;
                }
#endif

                this->tracks.append(track);
                track->setPlayed(false);
                connect(track, SIGNAL(removed()), SLOT(trackRemoved()));
            }
            endInsertRows();

        }
    }
}

void PlaylistModel::clear() {
    playedTracks.clear();
    tracks.clear();
    activeTrack = 0;
    activeRow = -1;
    emit layoutChanged();
    emit reset();
    emit activeRowChanged(-1, false);
}

// --- item removal

bool PlaylistModel::removeRows(int position, int rows, const QModelIndex &parent) {
    qDebug() << __PRETTY_FUNCTION__ << position << rows << parent;

    // return false;

    if (position < 0 || position >= tracks.size() || position + rows > tracks.size()) {
      return false;
    }

    if (parent.isValid()) return false;

    if (position >= tracks.size() || position + rows <= 0) return false;

    int beginRow = qMax(0, position);
    int endRow = qMin(position + rows - 1, tracks.size() - 1);

    beginRemoveRows(QModelIndex(), beginRow, endRow);
    while (beginRow <= endRow) {
        Track *track = tracks.takeAt(beginRow);
        if (track) {
            track->setPlayed(false);
            playedTracks.removeAll(track);
        }
        ++beginRow;
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
            playedTracks.removeAll(track);
            track->setPlayed(false);
        }
    }
}

// --- Sturm und drang ---

Qt::DropActions PlaylistModel::supportedDropActions() const {
    return Qt::MoveAction | Qt::CopyAction;
}


Qt::DropActions PlaylistModel::supportedDragActions() const {
    return Qt::CopyAction;
}

Qt::ItemFlags PlaylistModel::flags(const QModelIndex &index) const {
    if (index.isValid())
        return (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
    return Qt::ItemIsDropEnabled;
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
    if (action == Qt::IgnoreAction) return true;
    if (!data->hasFormat(TRACK_MIME)) return false;
    if (column > 0) return false;

    int beginRow;
    if (row != -1) beginRow = row;
    else if (parent.isValid()) beginRow = parent.row();
    else beginRow = rowCount();

    const TrackMimeData* trackMimeData = dynamic_cast<const TrackMimeData*>(data);
    if (!trackMimeData) return false;

    QList<Track*> droppedTracks = trackMimeData->tracks();

    layoutAboutToBeChanged();

    bool insert = false;
    QList<Track*> movedTracks;
    int counter = 0;
    foreach(Track *track, droppedTracks) {
        // if preset, remove track and maybe fix beginRow
        int originalRow = tracks.indexOf(track);
        if (originalRow != -1) {
            Track *movedTrack = tracks.takeAt(originalRow);
            movedTracks << movedTrack;
            if (originalRow < beginRow) beginRow--;
        } else insert = true;
        const int targetRow = beginRow + counter;
        tracks.insert(targetRow, track);
#ifdef APP_DEMO
        if (tracks.size() >= demoMaxTracks) {
            MainWindow::instance()->showDemoDialog(demoMessage);
            layoutChanged();
            return true;
        }
#endif
        counter++;
    }

    // fix activeRow after all this
    activeRow = tracks.indexOf(activeTrack);

    layoutChanged();

    if (!insert)
        emit needSelectionFor(movedTracks);

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
        // qDebug() << "index row" << row;
        Track *track = trackAt(row);
        if (track)
            movedTracks << track;
    }

    int counter = 1;
    foreach (Track *track, movedTracks) {

        int row = rowForTrack(track);
        // qDebug() << "track row" << row;
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

void PlaylistModel::trackRemoved() {

    // get the Track that sent the signal
    Track *track = static_cast<Track*>(sender());
    if (!track) {
        qDebug() << "Cannot get sender track";
        return;
    }

    int trackRow = rowForTrack(track);
    if (trackRow != -1) {
        qDebug() << "removing track from playlist model" << track;
        removeRows(trackRow, 1, QModelIndex());
    }
}

bool PlaylistModel::saveTo(QTextStream & stream) const
{
    // stream not opened or not writable
    if ( !stream.device()->isOpen() || !stream.device()->isWritable() )
        return false;

    stream.setCodec("UTF-8");
    stream << "[playlist]" << endl;
    int idx = 1;
    foreach(Track* tr, tracks)
    {
        stream << "File" << idx << "=" << tr->getPath() << endl;
        stream << "Title" << idx << "=" << tr->getTitle() << endl;
        stream << "Length" << idx++ << "=" << tr->getLength() << endl;
    }
    stream << "NumberOfEntries=" << tracks.count() << endl;
    stream << "Version=2" << endl;

    return true;
}

bool PlaylistModel::loadFrom(QTextStream & stream)
{
    // stream not opened or not writable
    if ( !stream.device()->isOpen() || !stream.device()->isReadable() )
        return false;

    stream.setCodec("UTF-8");
    QString header;
    QString tag;
    Track* cur = 0;
    stream >> header;
    while ( !stream.atEnd() )
    {
        QString line = stream.readLine(1024);
        if ( line.startsWith("File") )
        {
            QString path = line.section("=", -1);
            cur = Track::forPath(path);
            if (cur) addTrack(cur);
        }
        else if ( line.startsWith("Title") && cur != NULL )
        {
            tag = line.right(line.size()-line.indexOf("=")-1);
            if ( !tag.isNull() && !tag.isEmpty() )
                cur->setTitle(tag);
        }
    }

    return true;
}
