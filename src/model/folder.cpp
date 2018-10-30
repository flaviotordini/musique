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

#include "folder.h"

#include "album.h"
#include "artist.h"

#include "../database.h"
#include <QtSql>

namespace {
QHash<QString, Folder *> cache;
}

Folder::Folder(const QString &path, QObject *parent)
    : Item(parent), path(path), trackCount(-1), totalLength(-1) {
    dir.setPath(path);
}

Folder *Folder::forPath(const QString &path) {
    // qDebug() << "Folder::forPath" << path;

    auto i = cache.constFind(path);
    if (i != cache.constEnd()) return i.value();

    Folder *folder = new Folder(path);
    cache.insert(path, folder);

    return folder;
}

QList<Track *> Folder::getTracks() {
    QList<Track *> tracks;
    QString collectionRoot = Database::instance().collectionRoot() + "/";
    if (path.length() < collectionRoot.length()) path = collectionRoot;
    // qDebug() << path << collectionRoot;
    QString relativePath = QString(path).replace(collectionRoot, "");
    // qDebug() << relativePath << path;

    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare(
            "select id from tracks where path like ? order by artist, album, disk, track, path");
    query.bindValue(0, QString(relativePath + "/%"));
    bool success = query.exec();
    if (!success)
        qDebug() << query.lastQuery() << query.lastError().text() << query.lastError().number();

    tracks.reserve(query.size());
    while (query.next()) {
        int trackId = query.value(0).toInt();
        Track *track = Track::forId(trackId);
        tracks << track;
    }
    return tracks;
}

int Folder::getTrackCount() {
    if (trackCount == -1) {
        QList<Track *> tracks = getTracks();
        trackCount = tracks.size();
        qDebug() << trackCount;
        if (trackCount > 0)
            totalLength = Track::getTotalLength(tracks);
        else
            totalLength = 0;
    }
    return trackCount;
}

int Folder::getTotalLength() {
    if (totalLength == -1) {
        QList<Track *> tracks = getTracks();
        trackCount = tracks.size();
        if (trackCount > 0)
            totalLength = Track::getTotalLength(tracks);
        else
            totalLength = 0;
    }
    return totalLength;
}
