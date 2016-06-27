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

#ifndef TRACKMIMEDATA_H
#define TRACKMIMEDATA_H

#include <QMimeData>
#include "model/track.h"
#include "constants.h"

static const QString TRACK_MIME = "application/x-" + QLatin1String(Constants::UNIX_NAME) + "-tracks";

class TrackMimeData : public QMimeData {

    Q_OBJECT

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
