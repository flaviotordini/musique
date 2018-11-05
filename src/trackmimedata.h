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

#include "constants.h"
#include "model/track.h"
#include <QMimeData>

class TrackMimeData : public QMimeData {
    Q_OBJECT

public:
    TrackMimeData();

    static const QString mime;
    static const QStringList &types();

    virtual QStringList formats() const;
    virtual bool hasFormat(const QString &mimeType) const;

    const QVector<Track *> &getTracks() const { return tracks; }
    void addTrack(Track *track) { tracks << track; }
    void setTracks(const QVector<Track *> &value) { tracks = value; }

private:
    QVector<Track *> tracks;
};

#endif // TRACKMIMEDATA_H
