#ifndef TRACK_H
#define TRACK_H

#include <QtCore>
#include "item.h"

class Album;
class Artist;

class Track : public Item {

    Q_OBJECT

public:
    Track();

    // item
    QList<Track*> getTracks() {
        QList<Track*> tracks;
        tracks << this;
        return tracks;
    }

    // properties
    QString getName() { return title; }
    QString getTitle() { return title; }
    void setTitle(QString title) { this->title = title; }
    QString getPath() { return path; }
    void setPath(QString path) { this->path = path; }
    int getNumber() { return number; }
    void setNumber(int number) { this->number = number; }
    int getLength() { return length; }
    void setLength(int length) { this->length = length; }
    int getYear() { return year; }
    void setYear(int year) { this->year = year; }
    QString getHash();
    QString getAbsolutePath();
    QString getLyrics();

    // relations
    Album* getAlbum() { return album; }
    void setAlbum(Album *album) { this->album = album; }
    Artist* getArtist() { return artist; }
    void setArtist(Artist *artist) { this->artist = artist; }

    // data access
    static Track* forId(int trackId);
    static Track* forPath(QString path);
    static int idForPath(QString path);
    static bool exists(QString path);
    static bool isModified(QString path, QDateTime lastModified);
    static void remove(QString path);
    void insert();
    void update();


    // internet
    void fetchInfo();

    // utils
    static int getTotalLength(QList<Track*> tracks);

signals:
    void gotInfo();

private slots:
    void fetchMusicBrainzTrack();
    void parseMusicBrainzTrack(QByteArray bytes);

private:
    static QString getHash(QString);
    /**
      * Clear all data
      */
    void reset();

    // properties
    QString title;
    QString path;
    int number;
    int year;
    int length;

    // CUE support
    int start;
    int end;

    // relations
    Album *album;
    Artist *artist;

};

// This is required in order to use QPointer<Track> as a QVariant
typedef QPointer<Track> TrackPointer;
Q_DECLARE_METATYPE(TrackPointer);

#endif // TRACK_H
