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
#include <QtWidgets>

#include "../database.h"
#include "../datautils.h"
#include <QtSql>

#include "../httputils.h"
#include "http.h"

Artist::Artist(QObject *parent)
    : Item(parent), trackCount(0), yearFrom(0), yearTo(0), listeners(0) {}

QHash<int, Artist *> Artist::cache;

Artist *Artist::forId(int artistId) {
    auto i = cache.constFind(artistId);
    if (i != cache.constEnd()) return i.value();

    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select name, trackCount, yearFrom, yearTo, listeners from artists where id=?");
    query.bindValue(0, artistId);
    bool success = query.exec();
    if (!success) qDebug() << query.lastQuery() << query.lastError().text();
    if (query.next()) {
        Artist *artist = new Artist();
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
    cache.insert(artistId, nullptr);
    return nullptr;
}

int Artist::idForName(const QString &name) {
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
    id = query.lastInsertId().toInt();
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
    if (hash.isNull()) hash = getHash(name);
    return hash;
}

QString Artist::getHash(const QString &name) {
    return DataUtils::normalizeTag(name);
}

QString Artist::getStatusTip() {
    return name + QLatin1String(" - ") + QString("%1 tracks").arg(trackCount);
}

void Artist::fetchInfo() {
    // fetchLastFmSearch();
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
    QUrlQuery q;
    q.addQueryItem("method", "artist.search");
    q.addQueryItem("api_key", Constants::LASTFM_API_KEY);
    q.addQueryItem("artist", name);
    q.addQueryItem("limit", "5");
    url.setQuery(q);

    lastFmSearches << name;

    QObject *reply = HttpUtils::lastFm().get(url);
    connect(reply, SIGNAL(data(QByteArray)), SLOT(parseLastFmSearch(QByteArray)));
    connect(reply, SIGNAL(error(QString)), SIGNAL(gotInfo()));
}

void Artist::parseNameAndMbid(const QByteArray &bytes, const QString &preferredValue) {
    QXmlStreamReader xml(bytes);

    const QString preferredValueLower = preferredValue.toLower();
    QString firstValue;
    QString firstMbid;

    /* We'll parse the XML until we reach end of it.*/
    while (!xml.atEnd() && !xml.hasError()) {
        /* Read next element.*/
        QXmlStreamReader::TokenType token = xml.readNext();

        /*
        qDebug() << xml.name();
        foreach (QXmlStreamAttribute attribute, xml.attributes())
            qDebug() << attribute.name() << ":" << attribute.value();
            */

        /* If token is StartElement, we'll see if we can read it.*/
        if (token == QXmlStreamReader::StartElement && xml.name() == "name") {
            QString text = xml.readElementText();
            // qDebug() << xml.name() << ":" << text;

            QString artistMbid;

            // now read its mbid
            while (!xml.atEnd() && !xml.hasError()) {
                QXmlStreamReader::TokenType token = xml.readNext();

                // stop at current artist to avoid getting the mbid of another one
                if (token == QXmlStreamReader::EndElement && xml.name() == "artist") break;

                if (token == QXmlStreamReader::StartElement && xml.name() == "mbid") {
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
    if (xml.hasError()) {
        qDebug() << xml.errorString();
    }

    name = firstValue;
    hash.clear();
    mbid = firstMbid;
}

void Artist::parseLastFmSearch(const QByteArray &bytes) {
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

            QObject *reply = HttpUtils::lastFm().head(url);
            connect(reply, SIGNAL(finished(QNetworkReply*)),
    SLOT(parseLastFmRedirectedName(QNetworkReply*))); connect(reply, SIGNAL(error(QNetworkReply*)),
    SIGNAL(gotInfo())); return;

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
    QUrlQuery q;
    q.addQueryItem("method", "artist.getinfo");
    q.addQueryItem("api_key", Constants::LASTFM_API_KEY);
    if (mbid.isEmpty()) {
        if (name.isEmpty()) {
            emit gotInfo();
            return;
        }
        q.addQueryItem("autocorrect", "1");
        q.addQueryItem("artist", name);
    } else
        q.addQueryItem("mbid", mbid);
    url.setQuery(q);

    QObject *reply = HttpUtils::lastFm().get(url);
    connect(reply, SIGNAL(data(QByteArray)), SLOT(parseLastFmInfo(QByteArray)));
    connect(reply, SIGNAL(error(QString)), SIGNAL(gotInfo()));
}

void Artist::parseLastFmInfo(const QByteArray &bytes) {
    QXmlStreamReader xml(bytes);

    while (xml.readNextStartElement()) {
        if (xml.name() == "artist") {
            while (xml.readNextStartElement()) {
                const QStringRef n = xml.name();

                if (n == QLatin1String("name")) {
                    QString artistName = xml.readElementText();
                    if (name != artistName) {
                        QString newHash = getHash(artistName);
                        if (getHash() != newHash) {
                            qDebug() << "Fixed artist name" << name << "->" << artistName << hash
                                     << newHash;
                            name = artistName;
                            hash.clear();
                        }
                    }
                }

                else if (n == QLatin1String("image") &&
                         xml.attributes().value("size") == QLatin1String("extralarge")) {
                    if (!QFile::exists(getImageLocation())) {
                        xml.readElementText();
                        QString imageUrl("http://tse2.mm.bing.net/th?q=");
                        imageUrl += name;
                        imageUrl += "+band&w=300&h=300&c=9&rs=1";
                        if (!imageUrl.isEmpty()) setProperty("imageUrl", imageUrl);
                    } else
                        xml.skipCurrentElement();
                }

                else if (n == QLatin1String("stats")) {
                    while (xml.readNextStartElement()) {
                        if (xml.name() == "listeners") {
                            listeners = xml.readElementText().toUInt();
                        } else
                            xml.skipCurrentElement();
                    }
                }

                else if (n == QLatin1String("bio")) {
                    while (xml.readNextStartElement()) {
                        if (xml.name() == "content") {
                            QString bio = xml.readElementText();
                            static const QRegExp licenseRE("User-contributed text is available.*");
                            bio.remove(licenseRE);
                            bio = bio.trimmed();
                            if (!bio.isEmpty()) {
                                const QString bioLocation = getBioLocation();
                                QDir().mkpath(QFileInfo(bioLocation).absolutePath());
                                QFile file(bioLocation);
                                if (!file.open(QIODevice::WriteOnly))
                                    qWarning()
                                            << "Error opening file for writing" << file.fileName();
                                QTextStream stream(&file);
                                stream << bio;
                            }

                        } else if (xml.name() == "formationlist") {
                            while (xml.readNextStartElement()) {
                                if (xml.name() == "formation") {
                                    while (xml.readNextStartElement()) {
                                        if (yearFrom == 0 && xml.name() == "yearfrom") {
                                            yearFrom = xml.readElementText().toInt();
                                        } else if (xml.name() == "yearto") {
                                            yearTo = xml.readElementText().toInt();
                                        } else
                                            xml.skipCurrentElement();
                                    }
                                } else
                                    xml.skipCurrentElement();
                            }
                        }

                        else
                            xml.skipCurrentElement();
                    }
                }

                else
                    xml.skipCurrentElement();
            }
        }
    }

    if (xml.hasError()) qWarning() << xml.errorString();

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

bool Artist::hasPhoto() {
    return QFile::exists(getImageLocation());
}

QPixmap Artist::getPhoto() {
    QPixmap p;
    QFile file(getImageLocation());
    if (file.open(QFile::ReadOnly)) {
        p.loadFromData(file.readAll());
        file.close();
    }
    return p;
}

QPixmap Artist::getThumb(int width, int height, qreal pixelRatio) {
    if (pixmap.isNull() || pixmap.devicePixelRatio() != pixelRatio ||
        pixmap.width() != width * pixelRatio) {
        pixmap = getPhoto();
        if (pixmap.isNull()) return pixmap;

        const int pixelWidth = width * pixelRatio;
        const int pixelHeight = height * pixelRatio;
        const int wDiff = pixmap.width() - pixelWidth;
        const int hDiff = pixmap.height() - pixelHeight;
        if (wDiff > 0 || hDiff > 0) {
            int xOffset = 0;
            int yOffset = 0;
            if (hDiff > 0) yOffset = hDiff / 4;
            if (wDiff > 0) xOffset = wDiff / 2;
            pixmap = pixmap.copy(xOffset, yOffset, pixelWidth, pixelHeight);
        }
        pixmap.setDevicePixelRatio(pixelRatio);
    }
    return pixmap;
}

void Artist::setPhoto(const QByteArray &bytes) {
    qDebug() << "Storing photo for" << name;

    // store photo
    QString storageLocation = getImageLocation();
    QDir().mkpath(QFileInfo(storageLocation).absolutePath());
    QFile file(storageLocation);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Error opening file for writing" << file.fileName();
    }
    QDataStream stream(&file); // we will serialize the data into the file
    stream.writeRawData(bytes.constData(), bytes.size());

    emit gotPhoto();
}

QVector<Track *> Artist::getTracks() {
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select distinct t.id from tracks t, albums a"
                  " where (t.album=a.id or t.album=0) and t.artist=?"
                  " order by a.year desc, a.title collate nocase, t.disk, t.track, t.path");
    query.bindValue(0, id);
    bool success = query.exec();
    if (!success)
        qDebug() << query.lastQuery() << query.lastError().text() << query.lastError().number();
    QVector<Track *> tracks;
    tracks.reserve(query.size());
    while (query.next()) {
        int trackId = query.value(0).toInt();
        Track *track = Track::forId(trackId);
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
