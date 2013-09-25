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

#include "artist.h"
#include "../constants.h"
#include <QtGui>

#include <QtSql>
#include "../database.h"
#include "../datautils.h"

#include "../networkaccess.h"
#include "../mbnetworkaccess.h"

namespace The {
NetworkAccess* http();
}

Artist::Artist(QObject *parent) : Item(parent),
    trackCount(0),
    yearFrom(0),
    yearTo(0),
    listeners(0) { }

QHash<int, Artist*> Artist::cache;

Artist* Artist::forId(int artistId) {

    if (cache.contains(artistId)) {
        // get from cache
        // qDebug() << "Artist was cached" << artistId;
        return cache.value(artistId);
    }

    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select name, trackCount, yearFrom, yearTo, listeners from artists where id=?");
    query.bindValue(0, artistId);
    bool success = query.exec();
    if (!success) qDebug() << query.lastQuery() << query.lastError().text();
    if (query.next()) {
        Artist* artist = new Artist();
        artist->setId(artistId);
        artist->setName(query.value(0).toString());
        artist->trackCount = query.value(1).toInt();
        artist->yearFrom = query.value(2).toInt();
        artist->yearTo = query.value(3).toInt();
        artist->listeners = query.value(4).toUInt();
        // Add other fields here...

        // put into cache
        cache.insert(artistId, artist);
        return artist;
    }
    cache.insert(artistId, 0);
    return 0;
}

int Artist::idForName(QString name) {
    int id = -1;
    const QString hash = Artist::getHash(name);
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select id from artists where hash=?");
    query.bindValue(0, hash);
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
    if (query.next()) {
        id = query.value(0).toInt();
    }
    // qDebug() << "artist hash" <<   hash << "id" << id;
    return id;
}

void Artist::insert() {

    const QString hash = getHash();
    if (hash.isEmpty() || hash.length() < 3) return;

    // qDebug() << "Artist::insert";
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("insert into artists"
                  " (hash, name, yearFrom, yearTo, listeners, albumCount, trackCount)"
                  " values (?,?,?,?,?,0,0)");
    query.bindValue(0, hash);
    query.bindValue(1, name);
    query.bindValue(2, yearFrom);
    query.bindValue(3, yearTo);
    query.bindValue(4, listeners);
    // qDebug() << query.lastQuery();
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
}

void Artist::update() {
    // qDebug() << "Artist::update";
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("update artists set name=? where hash=?");
    query.bindValue(0, name);
    query.bindValue(1, getHash());
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
}

const QString &Artist::getHash() {
    if (hash.isNull())
        hash = getHash(name);
    return hash;
}

QString Artist::getHash(QString name) {
    return DataUtils::normalizeTag(name);
}

QString Artist::getStatusTip() {
    return name + QLatin1String(" - ") + QString("%1 tracks").arg(trackCount);
}

void Artist::fetchInfo() {
    // fetchLastFmSearch();
    fetchLastFmInfo();
}

// *** MusicBrainz ***

void Artist::fetchMusicBrainzArtist() {
    QUrl url = QString("http://musicbrainz.org/ws/1/artist/?type=xml&name=%1&limit=1")
            .arg(name);

    MBNetworkAccess *http = new MBNetworkAccess();
    QObject *reply = http->get(url);
    connect(reply, SIGNAL(data(QByteArray)), SLOT(parseMusicBrainzArtist(QByteArray)));
}

void Artist::parseMusicBrainzArtist(QByteArray bytes) {
    QString correctName = DataUtils::getXMLElementText(bytes, "name");
    qDebug() << name << "-> MusicBrainz ->" << correctName;
    if (!correctName.isEmpty()) {
        this->name = correctName;
        hash.clear();
    }

    // And now gently ask the Last.fm guys for some more info
    fetchLastFmInfo();
}

// *** Last.fm ***

void Artist::fetchLastFmSearch() {
    if (name.isEmpty()) {
        emit gotInfo();
        return;
    }

    // Avoid rare infinite loops with redirected artists
    if (lastFmSearches.contains(name)) {
        emit gotInfo();
        return;
    }

    QUrl url("http://ws.audioscrobbler.com/2.0/");
    url.addQueryItem("method", "artist.search");
    url.addQueryItem("api_key", Constants::LASTFM_API_KEY);
    url.addQueryItem("artist", name);
    url.addQueryItem("limit", "5");

    lastFmSearches << name;

    QObject *reply = The::http()->get(url);
    connect(reply, SIGNAL(data(QByteArray)), SLOT(parseLastFmSearch(QByteArray)));
    connect(reply, SIGNAL(error(QNetworkReply*)), SIGNAL(gotInfo()));
}

void Artist::parseNameAndMbid(QByteArray bytes, QString preferredValue) {
    QXmlStreamReader xml(bytes);

    const QString preferredValueLower = preferredValue.toLower();
    QString firstValue;
    QString firstMbid;

    /* We'll parse the XML until we reach end of it.*/
    while(!xml.atEnd() && !xml.hasError()) {

        /* Read next element.*/
        QXmlStreamReader::TokenType token = xml.readNext();

        /*
        qDebug() << xml.name();
        foreach (QXmlStreamAttribute attribute, xml.attributes())
            qDebug() << attribute.name() << ":" << attribute.value();
            */

        /* If token is StartElement, we'll see if we can read it.*/
        if(token == QXmlStreamReader::StartElement
                && xml.name() == "name") {
            QString text = xml.readElementText();
            // qDebug() << xml.name() << ":" << text;

            QString artistMbid;

            // now read its mbid
            while (!xml.atEnd() && !xml.hasError()) {
                QXmlStreamReader::TokenType token = xml.readNext();

                // stop at current artist to avoid getting the mbid of another one
                if(token == QXmlStreamReader::EndElement
                        && xml.name() == "artist") break;

                if(token == QXmlStreamReader::StartElement
                        && xml.name() == "mbid") {
                    artistMbid = xml.readElementText();
                    break;
                }
            }

            if (text.toLower() == preferredValueLower) {
                name = text;
                hash.clear();
                mbid = artistMbid;
                return;
            }

            if (firstValue.isNull()) {
                firstValue = text;
                firstMbid = artistMbid;
            }

        }

    }

    /* Error handling. */
    if(xml.hasError()) {
        qDebug() << xml.errorString();
    }

    name = firstValue;
    hash.clear();
    mbid = firstMbid;

}

void Artist::parseLastFmSearch(QByteArray bytes) {

    // static const QString redirectToken = "+noredirect/";

    // mbid = DataUtils::getXMLElementText(bytes, "mbid");
    // name = DataUtils::getXMLElementTextWithPreferredValue(bytes, "name", name);

    parseNameAndMbid(bytes, name);

    /*
    if (mbid.isEmpty()) {
        QString urlString = DataUtils::getXMLElementText(bytes, "url");
        if (!urlString.isEmpty() && urlString.contains(redirectToken)) {
            if (!urlString.startsWith("http://")) urlString.prepend("http://");
            urlString.remove(redirectToken);
            QUrl url = QUrl::fromEncoded(urlString.toUtf8());

            // get it and parse the Location header
            qDebug() << "Redirected artist" << name << urlString << url;

            QObject *reply = The::http()->head(url);
            connect(reply, SIGNAL(finished(QNetworkReply*)), SLOT(parseLastFmRedirectedName(QNetworkReply*)));
            connect(reply, SIGNAL(error(QNetworkReply*)), SIGNAL(gotInfo()));
            return;

        }

    }
    */

    fetchLastFmInfo();

}

void Artist::parseLastFmRedirectedName(QNetworkReply *reply) {
    QString location = reply->header(QNetworkRequest::LocationHeader).toString();
    qDebug() << "Location header" << reply->url() << location;
    if (!location.isEmpty()) {
        int slashIndex = location.lastIndexOf('/');
        if (slashIndex > 0 && slashIndex < location.length()) {
            QString redirectedName = location.mid(slashIndex + 1);
            qDebug() << name << "redirected to" << redirectedName;
            name = redirectedName;
            hash.clear();
            fetchLastFmSearch();
            return;
        }
    }
    emit gotInfo();
}

void Artist::fetchLastFmInfo() {

    QUrl url("http://ws.audioscrobbler.com/2.0/");
    url.addQueryItem("method", "artist.getinfo");
    url.addQueryItem("api_key", Constants::LASTFM_API_KEY);

    if (mbid.isEmpty()) {
        url.addQueryItem("autocorrect", "1");
        url.addQueryItem("artist", name);
    } else url.addQueryItem("mbid", mbid);

    QObject *reply = The::http()->get(url);
    connect(reply, SIGNAL(data(QByteArray)), SLOT(parseLastFmInfo(QByteArray)));
    connect(reply, SIGNAL(error(QNetworkReply*)), SIGNAL(gotInfo()));
}

void Artist::parseLastFmInfo(QByteArray bytes) {
    QXmlStreamReader xml(bytes);

    while(xml.readNextStartElement()) {

        if (xml.name() == "artist") {

            while (xml.readNextStartElement()) {
                const QStringRef n = xml.name();

                if(n == QLatin1String("name")) {
                    QString artistName = xml.readElementText();
                    if (name != artistName) {
                        qDebug() << "Fixed artist name" << name << "->" << artistName;
                        name = artistName;
                        hash.clear();
                    }
                }

                else if(n == QLatin1String("image") &&
                        xml.attributes().value("size") == QLatin1String("extralarge")) {
                    if (!QFile::exists(getImageLocation())) {
                        QString imageUrl = xml.readElementText();
                        if (!imageUrl.isEmpty())
                            setProperty("imageUrl", imageUrl);
                    } else xml.skipCurrentElement();
                }

                else if (n == QLatin1String("stats")) {
                    while (xml.readNextStartElement()) {
                        if(xml.name() == "listeners") {
                            listeners = xml.readElementText().toUInt();
                        } else xml.skipCurrentElement();
                    }
                }

                else if(n == QLatin1String("bio")) {
                    while (xml.readNextStartElement()) {
                        if(xml.name() == "content") {
                            QString bio = xml.readElementText();
                            static const QRegExp licenseRE("User-contributed text is available.*");
                            bio.remove(licenseRE);
                            bio = bio.trimmed();
                            if (!bio.isEmpty()) {
                                const QString bioLocation = getBioLocation();
                                QDir().mkpath(QFileInfo(bioLocation).absolutePath());
                                QFile file(bioLocation);
                                if (!file.open(QIODevice::WriteOnly))
                                    qWarning() << "Error opening file for writing" << file.fileName();
                                QTextStream stream(&file);
                                stream << bio;
                            }

                        } else if (xml.name() == "formationlist") {
                            while (xml.readNextStartElement()) {
                                if(xml.name() == "formation") {
                                    while (xml.readNextStartElement()) {
                                        if(yearFrom == 0 && xml.name() == "yearfrom") {
                                            yearFrom = xml.readElementText().toInt();
                                        } else if(xml.name() == "yearto") {
                                            yearTo = xml.readElementText().toInt();
                                        } else xml.skipCurrentElement();
                                    }
                                } else xml.skipCurrentElement();
                            }
                        }

                        else xml.skipCurrentElement();
                    }
                }

                else xml.skipCurrentElement();

            }

        }

    }

    if(xml.hasError())
        qWarning() << xml.errorString();

    emit gotInfo();
}

QString Artist::getBaseLocation() {
    return Database::getFilesLocation() + getHash();
}

QString Artist::getImageLocation() {
    return getBaseLocation() + QLatin1String("/_photo");
}

QString Artist::getBioLocation() {
    return getBaseLocation() + QLatin1String("/_bio");
}

QPixmap Artist::getPhoto() {
    return QPixmap(getImageLocation());
}

void Artist::setPhoto(QByteArray bytes) {
    qDebug() << "Storing photo for" << name;

    // store photo
    QString storageLocation = getImageLocation();
    QDir().mkpath(QFileInfo(storageLocation).absolutePath());
    QFile file(storageLocation);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Error opening file for writing" << file.fileName();
    }
    QDataStream stream( &file ); // we will serialize the data into the file
    stream.writeRawData(bytes.constData(), bytes.size());

    emit gotPhoto();
}

QList<Track*> Artist::getTracks() {
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select t.id from tracks t, albums a"
                  " where t.album=a.id and t.artist=?"
                  " order by a.year desc, a.title collate nocase, t.track, t.path");
    query.bindValue(0, id);
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

QString Artist::getBio() {
    QFile file(getBioLocation());
    if (!file.exists()) return QString();
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Cannot open file" << file.fileName();
        return QString();
    }
    QByteArray bytes = file.readAll();
    return QString::fromUtf8(bytes.data());
}
