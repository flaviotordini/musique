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

#ifndef ARTIST_H
#define ARTIST_H

#include <QtCore>
#include <QImage>
#include "item.h"
#include "track.h"
#include <QtNetwork>

class Artist : public Item {

    Q_OBJECT

public:
    Artist(QObject *parent = 0);

    // item
    QList<Track*> getTracks();
    QString getStatusTip();

    // properties
    QString getName()  { return name; }
    void setName(QString name) { this->name = name; }
    const QString &getHash();
    int getTrackCount() { return trackCount; }
    QString getBaseLocation();
    QString getBioLocation();
    QString getBio();

    // relations
    // QList<Album*> getAlbums();

    // data access
    static void clearCache() {
        qDeleteAll(cache);
        cache.clear();
    }
    static Artist* forId(int artistId);
    static int idForName(const QString& name);
    void insert();
    void update();

    // internet

    /**
      * Fix artist data using Last.fm and MusicBrainz web services.
      * Will emit gotInfo() when done.
      * This will also emit gotPhoto() when the photo is ready.
      */
    void fetchInfo();

    QString getImageLocation();
    QPixmap getPhoto();

    // qhash
    /*
    inline bool operator==(const Artist &other) {
        return getId() == other.getId();
    }
    inline uint qHash(const Artist &key) {
        return key.getId();
    }*/

public slots:
    void setPhoto(const QByteArray& bytes);

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
    void parseLastFmInfo(const QByteArray& bytes);
    void parseLastFmRedirectedName(QNetworkReply *reply);

private:
    void parseNameAndMbid(const QByteArray& bytes, const QString& preferredName);
    static QString getHash(const QString&);

    static QHash<int, Artist*> cache;

    int trackCount;

    QString name;
    QString mbid;

    int yearFrom;
    int yearTo;
    uint listeners;
    // QStringList tags;

    QStringList lastFmSearches;

    QString hash;

};

// This is required in order to use QPointer<Artist> as a QVariant
typedef QPointer<Artist> ArtistPointer;
Q_DECLARE_METATYPE(ArtistPointer)

#endif // ARTIST_H
