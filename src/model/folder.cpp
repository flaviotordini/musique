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

#include <QtSql>
#include "../database.h"

static QHash<QString, Folder*> folderCache;

Folder::Folder(QString path, QObject *parent)
    : Item(parent),
    path(path),
    trackCount(-1),
    totalLength(-1) {
    dir.setPath(path);
}

Folder* Folder::forPath(QString path) {

    // qDebug() << "Folder::forPath" << path;

    if (folderCache.contains(path)) {
        // get from cache
        // qDebug() << "Folder was cached" << path;
        return folderCache.value(path);
    }

    Folder* folder = new Folder(path);
    folderCache.insert(path, folder);

    return folder;
}

Album* Folder::getAlbum() {
    QSqlDatabase db = Database::instance().getConnection();

    QSqlQuery query(db);
    query.prepare("select id from albums where title like ? limit 1");
    query.bindValue(0, QString("%" + dir.dirName() + "%"));
    bool success = query.exec();
    if (!success) qDebug() << query.lastQuery() << query.lastError().text();
    if (query.next()) {
        return Album::forId(query.value(0).toInt());
    }
    return 0;
}

Artist* Folder::getArtist() {
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select artist from tracks where path like ? limit 1");
    query.bindValue(0, QString(path + "/%"));
    bool success = query.exec();
    if (!success) qDebug() << query.lastQuery() << query.lastError().text();
    if (query.next()) {
        return Artist::forId(query.value(0).toInt());
    }
    return 0;
}

QImage Folder::getPhoto() {

    QImage photo;

    // Try to get a relevant Album
    Album *album = getAlbum();

    // If there's no album, try to get a relevant Artist
    Artist *artist = 0;
    if (!album) {
        artist = getArtist();
    }

    QString imageLocation;
    QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    if (album) {
        imageLocation = dataLocation + "/albums/" + album->getHash();
    } else if (artist) {
        imageLocation = dataLocation + "/artists/" + artist->getHash();
    }

    if (!imageLocation.isEmpty())
        photo = QImage(imageLocation);

    return photo;
}

QList<Track*> Folder::getTracks() {
    QList<Track*> tracks;
    QString collectionRoot = Database::instance().collectionRoot()  + "/";
    if (path.length() < collectionRoot.length()) path = collectionRoot;
    // qDebug() << path << collectionRoot;
    QString relativePath = QString(path).replace(collectionRoot, "");
    // qDebug() << relativePath << path;

    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select id from tracks where path like ? order by artist, album, disk, track, path");
    query.bindValue(0, QString(relativePath + "/%"));
    bool success = query.exec();
    if (!success)
        qDebug() << query.lastQuery() << query.lastError().text() << query.lastError().number();

    while (query.next()) {
        int trackId = query.value(0).toInt();
        Track* track = Track::forId(trackId);
        tracks << track;
    }
    return tracks;
}

int Folder::getTrackCount() {
    if (trackCount == -1) {
        QList<Track*> tracks = getTracks();
        trackCount = tracks.size();
        qDebug() << trackCount;
        if (trackCount > 0) totalLength = Track::getTotalLength(tracks);
        else totalLength = 0;
    }
    return trackCount;
}

int Folder::getTotalLength() {
    if (totalLength == -1) {
        QList<Track*> tracks = getTracks();
        trackCount = tracks.size();
        if (trackCount > 0) totalLength = Track::getTotalLength(tracks);
        else totalLength = 0;
    }
    return totalLength;
}
