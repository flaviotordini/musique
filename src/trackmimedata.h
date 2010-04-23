#ifndef TRACKMIMEDATA_H
#define TRACKMIMEDATA_H

#include <QMimeData>
#include "model/track.h"

const QString TRACK_MIME = "application/x-minitunes-tracks";

class TrackMimeData : public QMimeData {

public:
    TrackMimeData();

    virtual QStringList formats() const;
    virtual bool hasFormat( const QString &mimeType ) const;

    QList<Track*> tracks() const { return m_tracks; }

    void addTrack(Track *track) {
        m_tracks << track;
    }

    void addTracks(QList<Track*> tracks) {
        m_tracks.append(tracks);
    }

private:
    QList<Track*> m_tracks;

};

#endif // TRACKMIMEDATA_H
