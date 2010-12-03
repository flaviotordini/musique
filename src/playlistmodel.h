#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QtGui>
#include "model/track.h"

namespace Playlist {

    enum PlaylistDataRoles {
        DataObjectRole = Qt::UserRole,
        ActiveItemRole,
        HoveredItemRole
    };

}

class PlaylistModel : public QAbstractListModel {

    Q_OBJECT

public:

    PlaylistModel(QWidget *parent);

    // inherited from QAbstractListModel
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool removeRows(int position, int rows, const QModelIndex &parent);

    // drag and drop
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QStringList mimeTypes() const;
    Qt::DropActions supportedDropActions() const;
    QMimeData* mimeData( const QModelIndexList &indexes ) const;
    bool dropMimeData(const QMimeData *data,
                      Qt::DropAction action, int row, int column,
                      const QModelIndex &parent);

    // custom methods
    void setActiveRow(int row, bool manual = false);
    bool rowExists( int row ) const { return (( row >= 0 ) && ( row < tracks.size() ) ); }
    void removeIndexes(QModelIndexList &indexes);
    int rowForTrack(Track* track);
    QModelIndex indexForTrack(Track* track);
    void move(QModelIndexList &indexes, bool up);

    Track* trackAt( int row ) const;
    Track* getActiveTrack() const;
    int getTotalLength() { return Track::getTotalLength(tracks); }

    // IO methods
    bool saveTo(QTextStream& stream) const;
    bool loadFrom(QTextStream& stream);

public slots:
    void addTrack(Track* track);
    void addTracks(QList<Track*> tracks);
    void clear();
    void skipBackward();
    void skipForward();
    void trackRemoved();

signals:
    void activeRowChanged(int, bool);
    void needSelectionFor(QList<Track*>);
    void itemChanged(int total);
    void playlistFinished();

private:
    void addShuffledTrack(Track *track);

    QList<Track*> tracks;
    QList<Track*> playedTracks;

    int activeRow;
    Track *activeTrack;

    QString errorMessage;
};

#endif // PLAYLISTMODEL_H
