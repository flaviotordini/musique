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

#ifndef TRACK_H
#define TRACK_H

#include "item.h"
#include <QtCore>

class Album;
class Artist;
class Genre;

class Track : public Item {
    Q_OBJECT

public:
    Track();

    // item
    QVector<Track *> getTracks() {
        QVector<Track *> tracks = {this};
        return tracks;
    }
    QString getName() { return title; }

    QString getStatusTip();

    // properties
    const QString &getTitle() { return title; }
    void setTitle(const QString &title) { this->title = title; }
    const QString &getPath() { return path; }
    void setPath(const QString &path) { this->path = path; }
    int getNumber() { return number; }
    void setNumber(int number) { this->number = number; }
    int getDiskNumber() { return diskNumber; }
    void setDiskNumber(int number) { diskNumber = number; }
    int getDiskCount() { return diskCount; }
    void setDiskCount(int value) { diskCount = value; }
    int getLength() { return length; }
    void setLength(int length);
    const QString &getFormattedLength();
    int getYear() { return year; }
    void setYear(int year) { this->year = year; }
    QString getHash();
    QString getAbsolutePath();
    bool isPlayed() { return played; }
    void setPlayed(bool played) { this->played = played; }
    uint getStartTime() { return startTime; }
    void setStartTime(uint startTime) { this->startTime = startTime; }
    void setLyrics(const QString &value);

    // relations
    Album *getAlbum() const { return album; }
    void setAlbum(Album *album) { this->album = album; }
    Artist *getArtist() const { return artist; }
    void setArtist(Artist *artist) { this->artist = artist; }
    const QVector<Genre *> &getGenres() const { return genres; }
    void addGenre(Genre *genre) { genres << genre; }

    // cache
    static void clearCache() {
        for (Track *track : qAsConst(cache))
            track->emitRemovedSignal();
        qDeleteAll(cache);
        cache.clear();
        cache.squeeze();
        pathCache.clear();
        pathCache.squeeze();
    }
    void emitRemovedSignal();

    // data access
    static Track *forId(int trackId);
    static Track *forPath(const QString &path);
    static int idForPath(const QString &path);
    static bool exists(const QString &path);
    static bool isModified(const QString &path, uint lastModified);
    static void remove(const QString &path);
    void insert();
    void update();

    // internet
    void fetchInfo();
    void getLyrics();

    // utils
    static int getTotalLength(const QVector<Track *> &tracks);

signals:
    void gotInfo();
    void gotLyrics(QString lyrics);
    void removed();

private:
    QString getLyricsLocation();
    static QString getHash(const QString &);

    static QHash<int, Track *> cache;
    static QHash<QString, Track *> pathCache;

    void reset();

    // properties
    QString title;
    QString path;
    int number;
    int diskNumber;
    int diskCount;
    int year;
    int length;
    QString formattedLength;

    /*
    // CUE support
    int start;
    int end;
    */

    // relations
    Album *album;
    Artist *artist;
    QVector<Genre *> genres;

    // playlist
    bool played;

    // scrobbling
    uint startTime;
};

// This is required in order to use QPointer<Track> as a QVariant
typedef QPointer<Track> TrackPointer;
Q_DECLARE_METATYPE(TrackPointer)

#endif // TRACK_H
