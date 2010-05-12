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
    ~PlaylistModel();

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
    void setActiveRow( int row );
    bool rowExists( int row ) const { return (( row >= 0 ) && ( row < tracks.size() ) ); }
    int activeRow() const { return m_activeRow; } // returns -1 if there is no active row
    int previousRow() const;
    int nextRow() const;
    void removeIndexes(QModelIndexList &indexes);
    int rowForTrack(Track* track);
    QModelIndex indexForTrack(Track* track);
    void move(QModelIndexList &indexes, bool up);

    Track* trackAt( int row ) const;
    Track* activeTrack() const;
    int getTotalLength() { return Track::getTotalLength(tracks); }

public slots:
    void addTrack(Track* track);
    void addTracks(QList<Track*> tracks);
    void clear();

signals:
    void activeRowChanged(int);
    void needSelectionFor(QList<Track*>);
    void itemChanged(int total);

private:
    QList<Track*> tracks;

    // the row being played
    int m_activeRow;
    Track *m_activeTrack;

    QString errorMessage;
};

#endif // PLAYLISTMODEL_H
