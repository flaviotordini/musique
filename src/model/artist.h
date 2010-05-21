#ifndef ARTIST_H
#define ARTIST_H

#include <QtCore>
#include <QImage>
#include <QDesktopServices>
#include "item.h"
#include "track.h"
#include <QtNetwork>

class Artist : public Item {

    Q_OBJECT

public:
    Artist(QObject *parent = 0);

    // item
    QList<Track*> getTracks();

    // properties
    QString getName()  { return name; }
    void setName(QString name) { this->name = name; }
    QString getHash();
    int getTrackCount() { return trackCount; }
    QString getBio();

    // relations
    // QList<Album*> getAlbums();

    // data access
    static void clearCache() {
        qDeleteAll(cache);
        cache.clear();
    }
    static Artist* forId(int artistId);
    static int idForName(QString name);
    void insert();

    // internet

    /**
      * Fix artist data using Last.fm and MusicBrainz web services.
      * Will emit gotInfo() when done.
      * This will also emit gotPhoto() when the photo is ready.
      */
    void fetchInfo();

    QImage getPhoto();

    // qhash
    /*
    inline bool operator==(const Artist &other) {
        return getId() == other.getId();
    }
    inline uint qHash(const Artist &key) {
        return key.getId();
    }*/

signals:
    void gotInfo();
    void gotPhoto();
    void gotCorrectName(QString correctName);

private slots:
    void fetchMusicBrainzArtist();
    void parseMusicBrainzArtist(QByteArray bytes);
    void fetchLastFmSearch();
    void parseLastFmSearch(QByteArray bytes);
    void fetchLastFmInfo();
    void parseLastFmInfo(QByteArray bytes);
    void parseLastFmRedirectedName(QNetworkReply *reply);
    void setPhoto(QByteArray bytes);

private:
    static QString getHash(QString);

    static QHash<int, Artist*> cache;

    int trackCount;

    QString name;
    QString bio;
    QString mbid;
    int lifeBegin;
    int lifeEnd;

    QStringList lastFmSearches;

};

// This is required in order to use QPointer<Artist> as a QVariant
typedef QPointer<Artist> ArtistPointer;
Q_DECLARE_METATYPE(ArtistPointer);

#endif // ARTIST_H
