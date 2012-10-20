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

Artist::Artist(QObject *parent) : Item(parent) {

}

QHash<int, Artist*> Artist::cache;

Artist* Artist::forId(int artistId) {

    if (cache.contains(artistId)) {
        // get from cache
        // qDebug() << "Artist was cached" << artistId;
        return cache.value(artistId);
    }

    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select name, trackCount from artists where id=?");
    query.bindValue(0, artistId);
    bool success = query.exec();
    if (!success) qDebug() << query.lastQuery() << query.lastError().text();
    if (query.next()) {
        Artist* artist = new Artist();
        artist->setId(artistId);
        artist->setName(query.value(0).toString());
        artist->trackCount = query.value(1).toInt();
        // artist->lifeBegin = query.value(2).toInt();
        // artist->lifeEnd = query.value(3).toInt();
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
    query.prepare("insert into artists (hash, name, albumCount, trackCount) values (?,?,0,0)");
    query.bindValue(0, hash);
    query.bindValue(1, name);
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

QString Artist::getHash() {
    return Artist::getHash(name);
}

QString Artist::getHash(QString name) {
    // return DataUtils::calculateHash(DataUtils::normalizeTag(name));
    return DataUtils::normalizeTag(name);
}

QString Artist::getStatusTip() {
    return name;
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

                if(xml.name() == "name") {
                    QString artistName = xml.readElementText();
                    if (name != artistName) {
                        qDebug() << "Fixed artist name" << name << "->" << artistName;
                        name = artistName;
                    }
                }

                else if(xml.name() == "image" && xml.attributes().value("size") == "extralarge") {
                    if (!QFile::exists(getImageLocation())) {
                        QString imageUrl = xml.readElementText();
                        if (!imageUrl.isEmpty())
                            setProperty("imageUrl", imageUrl);
                    }
                }

                else if(xml.name() == "bio") {
                    while (xml.readNextStartElement()) {
                        if(xml.name() == "content") {
                            bio = xml.readElementText();
                            static const QRegExp licenseRE("User-contributed text is available.*");
                            bio.remove(licenseRE);
                            if (!bio.isEmpty()) {
                                const QString storageLocation =
                                        QDesktopServices::storageLocation(QDesktopServices::DataLocation)
                                        + "/artists/biographies/";
                                QDir dir;
                                dir.mkpath(storageLocation);
                                QFile file(storageLocation + getHash());
                                if (!file.open(QIODevice::WriteOnly))
                                    qWarning() << "Error opening file for writing" << file.fileName();
                                QTextStream stream(&file);
                                stream << bio;
                            }
                        } else xml.skipCurrentElement();
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

QString Artist::getImageLocation() {
    return QDesktopServices::storageLocation(QDesktopServices::DataLocation) + "/artists/" + getHash();
}

QImage Artist::getPhoto() {
    return QImage(getImageLocation());
}

void Artist::setPhoto(QByteArray bytes) {
    qDebug() << "Storing photo for" << name;

    // store photo
    QString storageLocation = QDesktopServices::storageLocation(QDesktopServices::DataLocation) + "/artists/";
    QDir dir;
    dir.mkpath(storageLocation);
    QFile file(storageLocation + getHash());
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
    query.prepare("select id from tracks where artist=? order by album, track, path");
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

    const QString storageLocation =
            QDesktopServices::storageLocation(QDesktopServices::DataLocation)
            + "/artists/biographies/";
    QFile file(storageLocation + getHash());

    if (!file.exists()) return QString();

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Cannot open file" << file.fileName();
        return QString();
    }

    QByteArray bytes = file.readAll();
    return QString::fromUtf8(bytes.data());

}
