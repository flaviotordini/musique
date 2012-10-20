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

QString Album::getStatusTip() {
    QString tip = "◯ ";
    Artist* artist = getArtist();
    if (artist) tip += artist->getName() + " - ";
    tip += getTitle();
    if (year) tip += " (" + QString::number(year) + ")";
    return tip;
}

void Album::fetchInfo() {
    // an artist name is needed in order to fix the album title
    // also workaround last.fm bug with selftitled albums
    if (false && artist && artist->getName() != name) {
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
    url.addQueryItem("limit", "5");

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
    url.addQueryItem("autocorrect", "1");
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

    QMap<QString, QVariant> trackNames;

    while(xml.readNextStartElement()) {

        if (xml.name() == "album") {

            while (xml.readNextStartElement()) {

                if(xml.name() == "track") {
                    QString number = xml.attributes().value("rank").toString();
                    if (trackNames.contains(number)) xml.skipCurrentElement();
                    else
                        while (xml.readNextStartElement()) {
                            if (xml.name() == "name") {
                                QString title = xml.readElementText();
                                trackNames.insert(number, title);
                            }
                            else xml.skipCurrentElement();
                        }
                }

                else if(xml.name() == "name") {
                    QString albumTitle = xml.readElementText();
                    if (name != albumTitle) {
                        qDebug() << "Fixed album name" << name << "->" << albumTitle;
                        name = albumTitle;
                    }
                }

                else if(xml.name() == "image" && xml.attributes().value("size") == "extralarge") {
                    bool imageAlreadyPresent = property("localCover").toBool();
                    if (!imageAlreadyPresent)
                        imageAlreadyPresent = QFile::exists(getImageLocation());
                    if (!imageAlreadyPresent) {
                        QString imageUrl = xml.readElementText();
                        if (!imageUrl.isEmpty())
                            setProperty("imageUrl", imageUrl);
                    }
                }

                else if(xml.name() == "releasedate" && year < 1600) {
                    QString releasedateString = xml.readElementText().simplified();
                    if (!releasedateString.isEmpty()) {
                        // Something like "6 Apr 1999, 00:00"
                        QDateTime releaseDate = QDateTime::fromString(releasedateString, "d MMM yyyy, hh:mm");
                        int releaseYear = releaseDate.date().year();
                        if (releaseYear > 0)
                            year = releaseDate.date().year();
                    }
                }

                // wiki
                else if(xml.name() == "wiki") {
                    while (xml.readNextStartElement()) {
                        if(xml.name() == "content") {
                            QString bio = xml.readElementText();
                            static const QRegExp re("User-contributed text.*");
                            bio.remove(re);
                            if (!bio.isEmpty()) {
                                const QString storageLocation =
                                        QDesktopServices::storageLocation(QDesktopServices::DataLocation)
                                        + "/albums/wikis/";
                                QDir dir;
                                dir.mkpath(storageLocation);
                                QFile file(storageLocation + getHash());
                                if (!file.open(QIODevice::WriteOnly))
                                    qWarning() << "Error opening file for writing" << file.fileName();
                                QTextStream stream(&file); // we will serialize the data into the file
                                stream << bio;
                            }
                        } else xml.skipCurrentElement();
                    }
                }

                else xml.skipCurrentElement();

            }
        }
    }

    setProperty("trackNames", trackNames);

    if(xml.hasError())
        qWarning() << xml.errorString();

    emit gotInfo();
}

QString Album::getImageLocation() {
    QString l = QDesktopServices::storageLocation(QDesktopServices::DataLocation) + "/albums/";
    // if (artist) l += artist->getHash() + "/";
    l += getHash(); // + ".jpg";
    return l;
}

void Album::setPhoto(QByteArray bytes) {
    qDebug() << "Storing photo for" << name;

    // store photo
    QString storageLocation = getImageLocation();
    QFile file(storageLocation);
    QDir dir;
    dir.mkpath(QFileInfo(file).path());
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Error opening file for writing" << file.fileName();
    }
    QDataStream stream( &file ); // we will serialize the data into the file
    stream.writeRawData(bytes.constData(), bytes.size());

    emit gotPhoto();
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

QString normalizeString(QString s) {

    s = s.toLower();
    QString s2;

    // remove short words
    // QRegExp re("\\b(the|a|of|and|n|or|s|is)\\b");
    s2 = s;
    static const QRegExp shortWordsRE("\\b([a-z]{1,1}|the|of|and|or|is)\\b");
    s2.remove(shortWordsRE);
    if (s2.simplified().length() > 4) s = s2;
    s2.clear();

    // keep only letters
    int stringSize = s.size();
    for (int i = 0; i < stringSize; ++i) {
        const QChar c = s.at(i);
        if (c.isLetter())
            s2.append(c);
    }
    s = s2;
    s2.clear();

    // simplify accented chars èé=>e etc
    static QList<QPair<QChar, QString> > charVariants;
    static int charVariantsSize;
    if (charVariants.isEmpty()) {
        charVariants
                << QPair<QChar, QString>('a', "áàâäãåāăą")
                << QPair<QChar, QString>('e', "éèêëēĕėęě")
                << QPair<QChar, QString>('i', "íìıîïĩīĭį")
                << QPair<QChar, QString>('o', "óòôöõōŏőơ")
                << QPair<QChar, QString>('u', "úùûüũūŭůűųư")
                << QPair<QChar, QString>('c', "çćčĉċ")
                << QPair<QChar, QString>('d', "đ")
                << QPair<QChar, QString>('g', "ğĝġģǵ")
                << QPair<QChar, QString>('h', "ĥħ")
                << QPair<QChar, QString>('j', "ĵ")
                << QPair<QChar, QString>('k', "ķĸ")
                << QPair<QChar, QString>('l', "ĺļľŀ")
                << QPair<QChar, QString>('n', "ñńņňŉŋ")
                << QPair<QChar, QString>('r', "ŕŗř")
                << QPair<QChar, QString>('s', "śŝſș")
                << QPair<QChar, QString>('t', "ţťŧț")
                << QPair<QChar, QString>('r', "ŕŗř")
                << QPair<QChar, QString>('w', "ŵ")
                << QPair<QChar, QString>('y', "ýÿŷ")
                << QPair<QChar, QString>('z', "źż");
        charVariantsSize = charVariants.size();
    }

    stringSize = s.size();
    for (int i = 0; i < stringSize; ++i) {
        // qDebug() << s.at(i) << s.at(i).decomposition();
        const QChar currentChar = s.at(i);
        bool replaced = false;

        for (int y = 0; y < charVariantsSize; ++y) {
            QPair<QChar, QString> pair = charVariants.at(y);
            QChar c = pair.first;
            QString variants = pair.second;
            if (variants.contains(currentChar)) {
                s2.append(c);
                replaced = true;
                break;
            }
        }

        if (!replaced) s2.append(currentChar);
    }

    return s2;
}

QString Album::fixTrackTitleUsingTitle(Track *track, QString newTitle) {

    const QString trackTitle = track->getTitle();

    // handle Last.fm parenthesis stuff like "Song name (Remastered)"
    if (!trackTitle.contains('('))
        newTitle.remove(QRegExp(" *\\(.*\\)"));
    else if (newTitle.count('(') > 1) {
        int i = newTitle.indexOf('(');
        if (i != -1) {
            i = newTitle.indexOf('(', i+1);
            if (i != -1)
                newTitle = newTitle.left(i).simplified();
        }
    }

    if (trackTitle == newTitle) return newTitle;

    QString normalizedNewTitle = normalizeString(newTitle);
    if (normalizedNewTitle.isEmpty()) return QString();

    QString normalizedTrackTitle = normalizeString(trackTitle);
    if (normalizedTrackTitle.isEmpty()) return QString();

    if (normalizedNewTitle == normalizedTrackTitle
            || (normalizedTrackTitle.size() > 3 && normalizedNewTitle.contains(normalizedTrackTitle))
            || (normalizedNewTitle.size() > 3 && normalizedTrackTitle.contains(normalizedNewTitle))
            || (track->getNumber() && normalizedTrackTitle == "track")
            ) {
#ifndef QT_NO_DEBUG_OUTPUT
        if (trackTitle.toLower() != newTitle.toLower())
            qDebug() << "✓" << artist->getName() << name << trackTitle << "->" << newTitle;
#endif
        return newTitle;
    } else {
        // qDebug() << artist->getName() << name << normalizedTrackTitle << "!=" << normalizedNewTitle;
    }

    return QString();
}

void Album::fixTrackTitle(Track *track) {
    QMap<QString, QVariant> trackNames = property("trackNames").toMap();
    if (trackNames.isEmpty()) return;

    const QString trackTitle = track->getTitle();
    const int trackNumber = track->getNumber();

    // first, try the corresponding track number
    if (trackNumber && trackNames.contains(QString::number(trackNumber))) {
        QString newTitle = trackNames.value(QString::number(trackNumber)).toString();
        if (trackTitle.isEmpty()) {
            qDebug() << "✓" << artist->getName() << name << "[empty title]" << "->" << newTitle;
            track->setTitle(newTitle);
            trackNames.remove(QString::number(trackNumber));
            setProperty("trackNames", trackNames);
            return;
        }
        newTitle = fixTrackTitleUsingTitle(track, newTitle);
        if (!newTitle.isEmpty()) {
            track->setTitle(newTitle);
            trackNames.remove(QString::number(trackNumber));
            setProperty("trackNames", trackNames);
            return;
        }
    }

    // iterate on all tracks
    QMutableMapIterator<QString, QVariant> i(trackNames);
    while (i.hasNext()) {
        i.next();
        QString newTitle = i.value().toString();
        newTitle = fixTrackTitleUsingTitle(track, newTitle);
        if (!newTitle.isEmpty()) {
            track->setTitle(newTitle);
            int newNumber = i.key().toInt();
            if (newNumber && track->getNumber() == 0 && track->getNumber() != newNumber) {
                qDebug() << "Track number" << track->getNumber() << "->" << newNumber;
                track->setNumber(newNumber);
            }
            i.remove();
            setProperty("trackNames", trackNames);
            return;
        }
    }

}
