#ifndef ALBUM_H
#define ALBUM_H

#include <QtCore>
#include "item.h"
#include "artist.h"
#include "track.h"

class Album : public Item {

    Q_OBJECT

public:
    Album();

    // item
    QList<Track*> getTracks();

    // properties
    QString getName() { return name; }
    QString getTitle() { return name; }
    void setTitle(QString title) { this->name = title; }
    int getYear() { return year; }
    void setYear(int year) { this->year = year; }
    QString getHash();
    QString getWiki();

    // relations
    Artist* getArtist() { return artist; }
    void setArtist(Artist *artist) { this->artist = artist; }

    // data access
    static void clearCache() {
        qDeleteAll(cache);
        cache.clear();
    }
    static Album* forId(int albumId);
    static int idForName(QString name);
    void insert();


    // internet
    /**
      * Fix album data using Last.fm and MusicBrainz web services.
      * Will emit gotInfo() when done.
      * This will also emit gotPhoto() when the photo is ready.
      */
    void fetchInfo();
    QImage getPhoto();

signals:
    void gotInfo();

private slots:
    void fetchMusicBrainzRelease();
    void parseMusicBrainzRelease(QByteArray bytes);
    void fetchMusicBrainzReleaseDetails();
    void parseMusicBrainzReleaseDetails(QByteArray bytes);

    void fetchLastFmSearch();
    void parseLastFmSearch(QByteArray bytes);
    void fetchLastFmInfo();
    void parseLastFmInfo(QByteArray bytes);
    void parseLastFmRedirectedName(QNetworkReply *reply);

    void setPhoto(QByteArray bytes);

private:
    static QString getHash(QString);

    static QHash<int, Album*> cache;

    QString name;
    int year;
    Artist* artist;
    QString mbid;

};

// This is required in order to use QPointer<Album> as a QVariant
typedef QPointer<Album> AlbumPointer;
Q_DECLARE_METATYPE(AlbumPointer);

#endif // ALBUM_H
