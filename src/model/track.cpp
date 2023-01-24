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

#include "track.h"
#include "album.h"
#include "artist.h"
#include "genre.h"

#include "../database.h"
#include "../datautils.h"
#include <QtSql>

#include "../httputils.h"
#include "http.h"

#include <id3v2tag.h>
#include <mpegfile.h>
#include <unsynchronizedlyricsframe.h>

#include "lyrics.h"

Track::Track()
    : number(0), diskNumber(1), diskCount(1), year(0), length(0), album(nullptr), artist(nullptr),
      played(false), startTime(0) {}

QHash<int, Track *> Track::cache;
QHash<QString, Track *> Track::pathCache;

Track *Track::forId(int trackId) {
    auto i = cache.constFind(trackId);
    if (i != cache.constEnd()) return i.value();

    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare(
            "select path,title,duration,track,disk,diskCount,artist,album from tracks where id=?");
    query.bindValue(0, trackId);
    bool success = query.exec();
    if (!success) qDebug() << query.lastQuery() << query.lastError().text();
    if (query.next()) {
        Track *track = new Track();
        track->setId(trackId);
        track->setPath(query.value(0).toString());
        track->setTitle(query.value(1).toString());
        track->setLength(query.value(2).toInt());
        track->setNumber(query.value(3).toInt());
        track->setDiskNumber(query.value(4).toInt());
        track->setDiskCount(query.value(5).toInt());

        // relations
        // TODO this could be made lazy
        int artistId = query.value(6).toInt();
        track->setArtist(Artist::forId(artistId));
        int albumId = query.value(7).toInt();
        track->setAlbum(Album::forId(albumId));

        // put into cache
        cache.insert(trackId, track);
        pathCache.insert(track->getPath(), track);

        return track;
    }

    // id not found
    cache.insert(trackId, nullptr);
    return nullptr;
}

Track *Track::forPath(const QString &path) {
    // qDebug() << "Track::forPath" << path;
    auto i = pathCache.constFind(path);
    if (i != pathCache.constEnd()) return i.value();
    Track *track = nullptr;
    int id = Track::idForPath(path);
    if (id != -1) track = Track::forId(id);
    return track;
}

int Track::idForPath(const QString &path) {
    int id = -1;
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select id from tracks where path=?");
    query.bindValue(0, path);
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
    if (query.next()) {
        id = query.value(0).toInt();
    }
    return id;
}

bool Track::exists(const QString &path) {
    // qDebug() << "Track::exists";
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select count(*) from tracks where path=?");
    query.bindValue(0, path);
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
    if (query.next()) {
        return query.value(0).toBool();
    }
    return false;
}

bool Track::isModified(const QString &path, uint lastModified) {
    // qDebug() << "Track::isModified";
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select id from tracks where path=? and tstamp<?");
    query.bindValue(0, path);
    query.bindValue(1, lastModified);
    // qDebug() << query.lastQuery() << query.boundValues().values();
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
    return query.next();
}

void Track::insert() {
    // qDebug() << "Track::insert";
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("insert into tracks "
                  "(path,title,track,disk,diskCount,year,album,artist,albumArtist,tstamp,duration) "
                  "values (?,?,?,?,?,?,?,?,?,?,?)");
    query.bindValue(0, path);
    query.bindValue(1, title);
    query.bindValue(2, number);
    query.bindValue(3, diskNumber);
    query.bindValue(4, diskCount);
    query.bindValue(5, year);
    int albumId = album ? album->getId() : 0;
    query.bindValue(6, albumId);
    int artistId = artist ? artist->getId() : 0;
    query.bindValue(7, artistId);
    artistId = album && album->getArtist() ? album->getArtist()->getId() : 0;
    query.bindValue(8, artistId);
    query.bindValue(9, QDateTime::currentSecsSinceEpoch());
    query.bindValue(10, length);
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
    id = query.lastInsertId().toInt();

    // increment artist's track count
    if (artist && artist->getId()) {
        QSqlQuery query(db);
        query.prepare("update artists set trackCount=trackCount+1 where id=?");
        query.bindValue(0, artist->getId());
        bool success = query.exec();
        if (!success) qDebug() << query.lastError().text();
    }

    // increment album's track count
    if (album && album->getId()) {
        QSqlQuery query(db);
        query.prepare("update albums set trackCount=trackCount+1 where id=?");
        query.bindValue(0, album->getId());
        bool success = query.exec();
        if (!success) qDebug() << query.lastError().text();
    }

    // increment genres' track count
    for (Genre *genre : qAsConst(genres)) {
        {
            QSqlQuery query(db);
            query.prepare("insert into genreTracks (genre,track) values(?,?)");
            query.bindValue(0, genre->getId());
            query.bindValue(1, id);
            bool success = query.exec();
            if (!success)
                qDebug() << query.lastError().text() << query.executedQuery() << genre->getId()
                         << id;
        }

        QSqlQuery query(db);
        query.prepare("update genres set trackCount=trackCount+1 where id=?");
        query.bindValue(0, genre->getId());
        bool success = query.exec();
        if (!success) qDebug() << query.lastError().text();
    }
}

void Track::update() {
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);

    query.prepare("select album, artist from tracks where path=?");
    query.bindValue(0, path);
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
    if (query.next()) {
        int albumId = query.value(0).toInt();
        int artistId = query.value(1).toInt();

        if (!album || album->getId() != albumId) {
            // decrement previous album track count
            query.prepare("update albums set trackCount=trackCount-1 where id=?");
            query.bindValue(0, albumId);
            success = query.exec();
            if (!success) qDebug() << query.lastError().text();
            // and increment the new album track count
            if (album) {
                query.prepare("update albums set trackCount=trackCount+1 where id=?");
                query.bindValue(0, album->getId());
                bool success = query.exec();
                if (!success) qDebug() << query.lastError().text();
            }
        }

        if (!artist || artist->getId() != artistId) {
            query.prepare("update artists set trackCount=trackCount-1 where id=?");
            query.bindValue(0, artistId);
            success = query.exec();
            if (!success) qDebug() << query.lastError().text();
            if (artist) {
                query.prepare("update artists set trackCount=trackCount+1 where id=?");
                query.bindValue(0, artist->getId());
                bool success = query.exec();
                if (!success) qDebug() << query.lastError().text();
            }
        }
    }

    // qDebug() << "Track::update";

    query.prepare("update tracks set title=?, track=?, disk=?, year=?, album=?, artist=?, "
                  "albumArtist=?, tstamp=?, duration=? where path=?");

    query.bindValue(0, title);
    query.bindValue(1, number);
    query.bindValue(2, diskNumber);
    query.bindValue(3, year);
    int albumId = album ? album->getId() : 0;
    query.bindValue(4, albumId);
    int artistId = artist ? artist->getId() : 0;
    query.bindValue(5, artistId);
    artistId = album && album->getArtist() ? album->getArtist()->getId() : 0;
    query.bindValue(6, artistId);
    query.bindValue(7, QDateTime().toSecsSinceEpoch());
    query.bindValue(8, length);
    query.bindValue(9, path);
    success = query.exec();
    if (!success) qDebug() << query.lastError().text();
}

void Track::remove(const QString &path) {
    // qDebug() << "Track::remove";

    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);

    // first update trackCount on artist and album

    query.prepare("select album, artist, genre from tracks where path=?");
    query.bindValue(0, path);
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
    if (query.next()) {
        int albumId = query.value(0).toInt();
        int artistId = query.value(1).toInt();
        int genreId = query.value(2).toInt();

        query.prepare("update albums set trackCount=trackCount-1 where id=?");
        query.bindValue(0, albumId);
        success = query.exec();
        if (!success) qDebug() << query.lastError().text();

        query.prepare("update artists set trackCount=trackCount-1 where id=?");
        query.bindValue(0, artistId);
        success = query.exec();
        if (!success) qDebug() << query.lastError().text();

        query.prepare("update genres set trackCount=trackCount-1 where id=?");
        query.bindValue(0, genreId);
        success = query.exec();
        if (!success) qDebug() << query.lastError().text();
    }

    // update cache and notify everybody using this track
    // that it is gone forever
    int trackId = Track::idForPath(path);
    if (trackId != -1) {
        if (cache.contains(trackId)) {
            Track *track = cache.value(trackId);
            track->emitRemovedSignal();
            cache.remove(trackId);
            track->deleteLater();
        }
    }

    // and then actually delete the track
    query.prepare("delete from tracks where path=?");
    query.bindValue(0, path);
    success = query.exec();
    if (!success) qDebug() << query.lastError().text();
}

void Track::emitRemovedSignal() {
    emit removed();
}

QString Track::getHash() {
    return Track::getHash(title);
}

QString Track::getHash(const QString &name) {
    // return DataUtils::calculateHash(DataUtils::normalizeTag(name));
    return DataUtils::normalizeTag(name);
}

QString Track::getStatusTip() {
    QString tip = QString::fromUtf8("♫ ");
    Artist *artist = getArtist();
    if (artist) tip += artist->getName() + " - ";
    Album *album = getAlbum();
    if (album) tip += album->getName() + " - ";
    tip += getTitle();
    int duration = getLength();
    if (duration) tip += " (" + QString::number(duration) + ")";
    return tip;
}

void Track::fetchInfo() {
    emit gotInfo();
}

QString Track::getAbsolutePath() {
    QString collectionRoot = Database::instance().collectionRoot();
    QString absolutePath = collectionRoot + "/" + path;
    return absolutePath;
}

QString Track::getLyricsLocation() {
    QString l = Database::getFilesLocation();
    if (artist) l += artist->getHash();
    l += QLatin1String("/_lyrics/") + getHash();
    return l;
}

void Track::setLyrics(const QString &value) {
    QString filePath = getLyricsLocation();
    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Error opening file for writing" << file.fileName();
    } else {
        QTextStream stream(&file);
        stream << value;
    }
}

void Track::getLyrics() {
    QString artistName;
    if (artist) artistName = artist->getName();

    QFile file(getLyricsLocation());
    if (file.exists()) {
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Cannot open file" << file.fileName();
        } else {
            QByteArray bytes = file.readAll();
            emit gotLyrics(QString::fromUtf8(bytes.data()));
            return;
        }
    }

    Lyrics::get(artistName, title)
            .onData([this](auto lyrics) { emit gotLyrics(lyrics); })
            .onError([this, artistName](auto msg) { qDebug() << msg << artistName << title; });
}

int Track::getTotalLength(const QVector<Track *> &tracks) {
    int length = 0;
    for (Track *track : tracks) {
        length += track->getLength();
    }
    return length;
}
