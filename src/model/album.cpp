#include "album.h"
#include "../constants.h"
#include <QtGui>

#include <QtSql>
#include "../database.h"
#include "../datautils.h"

#include <QtNetwork>
#include "../networkaccess.h"
#include "../mbnetworkaccess.h"

namespace The {
    NetworkAccess* http();
}

static QHash<QString, QByteArray> artistAlbums;

Album::Album() : year(0), artist(0) {

}

QHash<int, Album*> Album::cache;

Album* Album::forId(int albumId) {

    if (cache.contains(albumId)) {
        // get from cache
        // qDebug() << "Album was cached" << albumId;
        return cache.value(albumId);
    }

    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select title, year, artist from albums where id=?");
    query.bindValue(0, albumId);
    bool success = query.exec();
    if (!success) qDebug() << query.lastQuery() << query.lastError().text();
    if (query.next()) {
        Album* album = new Album();
        album->setId(albumId);
        album->setTitle(query.value(0).toString());
        album->setYear(query.value(1).toInt());

        // relations
        int artistId = query.value(2).toInt();
        // TODO this could be made lazy
        album->setArtist(Artist::forId(artistId));

        // put into cache
        cache.insert(albumId, album);
        return album;
    }
    cache.insert(albumId, 0);
    return 0;
}

int Album::idForName(QString name) {
    int id = -1;
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select id from albums where hash=?");
    query.bindValue(0, Album::getHash(name));
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
    if (query.next()) {
        id = query.value(0).toInt();
    }
    // qDebug() << "album id" << id;
    return id;
}

void Album::insert() {
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("insert into albums (hash,title,year,artist,trackCount) values (?,?,?,?,0)");
    query.bindValue(0, getHash());
    query.bindValue(1, name);
    query.bindValue(2, year);
    int artistId = artist ? artist->getId() : 0;
    query.bindValue(3, artistId);
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();

    // increment artist's album count
    if (artist && artist->getId()) {
        QSqlQuery query(db);
        query.prepare("update artists set albumCount=albumCount+1 where id=?");
        query.bindValue(0, artist->getId());
        bool success = query.exec();
        if (!success) qDebug() << query.lastError().text();
    }

}

void Album::update() {
    // qDebug() << "Album::update";
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("update albums set title=?, year=?, artist=? where hash=?");
    query.bindValue(0, name);
    query.bindValue(1, year);
    int artistId = artist ? artist->getId() : 0;
    query.bindValue(2, artistId);
    query.bindValue(3, getHash());
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
}

QString Album::getHash() {
    return Album::getHash(name);
}

QString Album::getHash(QString name) {
    // return DataUtils::calculateHash(DataUtils::normalizeTag(name));
    return DataUtils::normalizeTag(name);
}

void Album::fetchInfo() {
    // an artist name is needed in order to fix the album title
    // also workaround last.fm bug with selftitled albums
    if (artist && artist->getName() != name) {
        fetchLastFmSearch();
    } else
        fetchLastFmInfo();
}

// *** MusicBrainz ***

void Album::fetchMusicBrainzRelease() {

    QString s = "http://musicbrainz.org/ws/1/release/?type=xml&title=%1&limit=1";
    s = s.arg(name);
    if (artist) {
        s = s.append("&artist=%2").arg(artist->getName());
    };

    QUrl url(s);
    MBNetworkAccess *http = new MBNetworkAccess();
    QObject *reply = http->get(url);
    connect(reply, SIGNAL(data(QByteArray)), SLOT(parseMusicBrainzRelease(QByteArray)));
    connect(reply, SIGNAL(error(QNetworkReply*)), SIGNAL(gotInfo()));
}

void Album::parseMusicBrainzRelease(QByteArray bytes) {
    QString correctTitle = DataUtils::getXMLElementText(bytes, "title");
    mbid = DataUtils::getXMLAttributeText(bytes, "release", "id");
    qDebug() << "Album:" << name << "-> MusicBrainz ->" << correctTitle << mbid;
    if (!correctTitle.isEmpty()) {
        this->name = correctTitle;
    }

    // get a list of tracks for this album
    // fetchMusicBrainzReleaseDetails();

    // And now gently ask the Last.fm guys for some more info
    emit gotInfo();
    // fetchLastFmInfo();
}

void Album::fetchMusicBrainzReleaseDetails() {

    QString s = "http://musicbrainz.org/ws/1/release/%1?type=xml&inc=tracks";
    s = s.arg(mbid);
    if (artist) {
        s = s.append("&artist=%2").arg(artist->getName());
    };

    QUrl url(s);
    MBNetworkAccess *http = new MBNetworkAccess();
    QObject *reply = http->get(url);
    connect(reply, SIGNAL(data(QByteArray)), SLOT(parseMusicBrainzReleaseDetails(QByteArray)));
    connect(reply, SIGNAL(error(QNetworkReply*)), SIGNAL(gotInfo()));
}

void Album::parseMusicBrainzReleaseDetails(QByteArray bytes) {
    QString correctTitle = DataUtils::getXMLElementText(bytes, "title");
    qDebug() << name << "-> MusicBrainz ->" << correctTitle;
    if (!correctTitle.isEmpty()) {
        this->name = correctTitle;
    }
}

// *** Last.fm Photo ***

QImage Album::getPhoto() {
    return QImage(QDesktopServices::storageLocation(QDesktopServices::DataLocation) + "/albums/" + getHash());
}

void Album::fetchLastFmSearch() {

    QUrl url("http://ws.audioscrobbler.com/2.0/");
    url.addQueryItem("method", "album.search");
    url.addQueryItem("api_key", Constants::LASTFM_API_KEY);
    url.addQueryItem("artist", artist->getName());
    url.addQueryItem("album", name);

    QObject *reply = The::http()->get(url);
    connect(reply, SIGNAL(data(QByteArray)), SLOT(parseLastFmSearch(QByteArray)));
    connect(reply, SIGNAL(error(QNetworkReply*)), SIGNAL(gotInfo()));
}

void Album::parseLastFmSearch(QByteArray bytes) {
    QXmlStreamReader xml(bytes);

    QString artistName;
    QString albumName;

    while(!xml.atEnd() && !xml.hasError()) {
        xml.readNext();
        if (xml.isStartElement()) {

            if(xml.name() == "artist") {
                artistName = xml.readElementText();
            } else if (xml.name() == "name") {
                albumName = xml.readElementText();
            }

        } else if (xml.isEndElement()) {

            if(xml.name() == "album") {
                // qDebug() << "Comparing artist name" << artist->getName() << artistName;
                if (artist->getName() == artistName) {
                    if (name != albumName) {
                        qDebug() << "Fixed album name" << name << "=>" << albumName;
                        name = albumName;
                    }
                    break;
                }
            }

        }
    }

    /* Error handling. */
    if(xml.hasError()) {
        qDebug() << xml.errorString();
    }

    fetchLastFmInfo();

}

void Album::parseLastFmRedirectedName(QNetworkReply *reply) {
    QString location = reply->header(QNetworkRequest::LocationHeader).toString();
    if (!location.isEmpty()) {
        int slashIndex = location.lastIndexOf('/');
        if (slashIndex > 0) {
            name = location.mid(slashIndex);
            // qDebug() << "*** Redirected name is" << name;
            fetchLastFmSearch();
            return;
        }
    }
    emit gotInfo();
}

void Album::fetchLastFmInfo() {

    /*
    if (QFile::exists(QDesktopServices::storageLocation(QDesktopServices::DataLocation) + "/albums/" + getHash())) {
        qDebug() << "Album" << name << "has a photo";
        emit gotInfo();
        return;
    } */

    if (!artist) {
        qDebug() << "Album" << name << "has no artist";
        emit gotInfo();
        return;
    }

    QUrl url("http://ws.audioscrobbler.com/2.0/");
    url.addQueryItem("method", "album.getinfo");
    url.addQueryItem("api_key", Constants::LASTFM_API_KEY);
    if (mbid.isEmpty()) {
        url.addQueryItem("artist", artist->getName());
        url.addQueryItem("album", name);
    } else {
        url.addQueryItem("mbid", mbid);
    }
    QObject *reply = The::http()->get(url);
    connect(reply, SIGNAL(data(QByteArray)), SLOT(parseLastFmInfo(QByteArray)));
    connect(reply, SIGNAL(error(QNetworkReply*)), SIGNAL(gotInfo()));

}

void Album::parseLastFmInfo(QByteArray bytes) {
    QXmlStreamReader xml(bytes);

    while(!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();
        if(token == QXmlStreamReader::StartElement) {

            if(xml.name() == "image" && xml.attributes().value("size") == "extralarge") {

                // qDebug() << title << " photo:" << imageUrl;

                bool imageAlreadyPresent = false;
                QString imageLocation = QDesktopServices::storageLocation(QDesktopServices::DataLocation) + "/albums/" + getHash();
                if (QFile::exists(imageLocation)) {
                    QFileInfo imageFileInfo(imageLocation);
                    const uint imagelastModified = imageFileInfo.lastModified().toTime_t();
                    if (imagelastModified > QDateTime::currentDateTime().toTime_t() - 86400*30) {
                        imageAlreadyPresent = true;
                    }
                }

                if (!imageAlreadyPresent) {
                    QString imageUrl = xml.readElementText();
                    // qDebug() << name << " photo:" << imageUrl;
                    if (!imageUrl.isEmpty()) {
                        QUrl url = QUrl::fromEncoded(imageUrl.toUtf8());
                        QObject *reply = The::http()->get(url);
                        connect(reply, SIGNAL(data(QByteArray)), SLOT(setPhoto(QByteArray)));
                    }
                }

            }

            else if(xml.name() == "releasedate") {
                QString releasedateString = xml.readElementText().simplified();
                if (!releasedateString.isEmpty()) {
                    // Something like "6 Apr 1999, 00:00"
                    QDateTime releaseDate = QDateTime::fromString(releasedateString, "d MMM yyyy, hh:mm");
                    int releaseYear = releaseDate.date().year();
                    if (releaseYear > 0) {
                        year = releaseDate.date().year();
                    }
                    // qDebug() << name << releasedateString << releaseDate.toString();
                }
            }

            // wiki
            // TODO check at least parent element name
            else if(xml.name() == "content") {
                QString bio = xml.readElementText();
                bio.remove(QRegExp("User-contributed text.*"));
                // qDebug() << name << " got wiki";
                if (!bio.isEmpty()) {
                    // store bio
                    const QString storageLocation =
                            QDesktopServices::storageLocation(QDesktopServices::DataLocation)
                            + "/albums/wikis/";
                    QDir dir;
                    dir.mkpath(storageLocation);
                    QFile file(storageLocation + getHash());
                    if (!file.open(QIODevice::WriteOnly)) {
                        qDebug() << "Error opening file for writing" << file.fileName();
                    }
                    QTextStream stream( &file ); // we will serialize the data into the file
                    stream << bio;
                }
            }

        }

    }

    /* Error handling. */
    if(xml.hasError()) {
        qDebug() << xml.errorString();
    }

    emit gotInfo();
}

void Album::setPhoto(QByteArray bytes) {
    qDebug() << "Storing photo for" << name;

    // store photo
    QString storageLocation = QDesktopServices::storageLocation(QDesktopServices::DataLocation) + "/albums/";
    QDir dir;
    dir.mkpath(storageLocation);
    QFile file(storageLocation + getHash());
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Error opening file for writing" << file.fileName();
    }
    QDataStream stream( &file ); // we will serialize the data into the file
    stream.writeRawData(bytes.constData(), bytes.size());
}

QList<Track*> Album::getTracks() {
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    if (artist) {
        query.prepare("select id from tracks where album=? and artist=? order by track, path");
    } else {
        query.prepare("select id from tracks where album=? order by track, path");
    }
    query.bindValue(0, id);
    if (artist)
        query.bindValue(1, artist->getId());
    bool success = query.exec();
    if (!success) qDebug() << query.lastQuery() << query.lastError().text() << query.lastError().number();
    QList<Track*> tracks;
    while (query.next()) {
        int trackId = query.value(0).toInt();
        Track* track = Track::forId(trackId);
        tracks << track;
    }
    return tracks;
}

QString Album::getWiki() {

    const QString storageLocation =
            QDesktopServices::storageLocation(QDesktopServices::DataLocation)
            + "/albums/wikis/";
    QFile file(storageLocation + getHash());

    if (!file.exists()) return QString();

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Cannot open file" << file.fileName();
        return QString();
    }

    QByteArray bytes = file.readAll();
    return QString::fromUtf8(bytes.data());

}
