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

#include "album.h"
#include "../constants.h"

#include "../database.h"
#include "../datautils.h"
#include <QtSql>
#include <utility>

#include "../httputils.h"
#include "http.h"

#include "../finderitemdelegate.h"

Album::Album() : year(0), artist(nullptr), listeners(0) {}

QHash<int, Album *> Album::cache;

Album *Album::forId(int albumId) {
    auto i = cache.constFind(albumId);
    if (i != cache.constEnd()) return i.value();

    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select title, year, artist from albums where id=?");
    query.bindValue(0, albumId);
    bool success = query.exec();
    if (!success) qDebug() << query.lastQuery() << query.lastError().text();
    if (query.next()) {
        Album *album = new Album();
        album->setId(albumId);
        album->setTitle(query.value(0).toString());
        album->setYear(query.value(1).toInt());

        // relations
        int artistId = query.value(2).toInt();
        album->setArtist(Artist::forId(artistId));
        // if (!album->getArtist()) qWarning() << "no artist for" << album->getName();

        // put into cache
        cache.insert(albumId, album);
        return album;
    }
    cache.insert(albumId, nullptr);
    return nullptr;
}

int Album::idForHash(const QString &hash) {
    int id = -1;
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select id from albums where hash=?");
    query.bindValue(0, hash);
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
    query.prepare("insert into albums (hash,title,year,artist,trackCount,listeners)"
                  " values (?,?,?,?,0,?)");
    query.bindValue(0, getHash());
    query.bindValue(1, name);
    query.bindValue(2, year);
    int artistId = artist ? artist->getId() : 0;
    query.bindValue(3, artistId);
    query.bindValue(4, listeners);
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
    id = query.lastInsertId().toInt();

    // increment artist's album count
    if (artist && artist->getId()) {
        QSqlQuery query(db);
        query.prepare("update artists set albumCount=albumCount+1 where id=?");
        query.bindValue(0, artist->getId());
        bool success = query.exec();
        if (!success) qDebug() << query.lastError().text();

        // for artists that have no yearFrom, use the earliest album year
        if (year > 0) {
            query = QSqlQuery(db);
            query.prepare(
                    "update artists set yearFrom=? where id=? and (yearFrom=0 or yearFrom>?)");
            query.bindValue(0, year);
            query.bindValue(1, artist->getId());
            query.bindValue(2, year);
            bool success = query.exec();
            if (!success) qDebug() << query.lastError().text();
        }
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

QString Album::getHash(const QString &name, Artist *artist) {
    QString h;
    if (artist)
        h = artist->getHash() + QLatin1Char('/');
    else
        h = "_unknown/";
    h += DataUtils::normalizeTag(name);
    return h;
}

const QString &Album::getHash() {
    if (hash.isNull()) hash = getHash(name, artist);
    return hash;
}

QString Album::getStatusTip() {
    QString tip;
    Artist *artist = getArtist();
    if (artist) tip += artist->getName() + " - ";
    tip += getTitle();
    if (year) tip += " (" + QString::number(year) + ")";
    tip += " - " + formattedDuration();
    return tip;
}

QString Album::formattedDuration() {
    int totalLength = Track::getTotalLength(getTracks());
    QString duration;
    if (totalLength > 3600)
        duration = QTime(0, 0).addSecs(totalLength).toString("h:mm:ss");
    else
        duration = QTime(0, 0).addSecs(totalLength).toString("m:ss");
    return duration;
}

void Album::fetchInfo() {
    // an artist name is needed in order to fix the album title
    // also workaround last.fm bug with selftitled albums
    /*
    if (artist && artist->getName() != name) {
        fetchLastFmSearch();
    } else */
    fetchLastFmInfo();
}

// *** Last.fm Photo ***

bool Album::hasPhoto() {
    return QFile::exists(getImageLocation());
}

QPixmap Album::getPhoto() {
    QPixmap p;
    QFile file(getImageLocation());
    if (file.open(QFile::ReadOnly)) {
        p.loadFromData(file.readAll());
        file.close();
    }
    return p;
}

QPixmap Album::getThumb(int width, int height, qreal pixelRatio) {
    if (pixmap.isNull() || pixmap.devicePixelRatio() != pixelRatio ||
        pixmap.width() != width * pixelRatio) {
        pixmap = getPhoto();
        if (pixmap.isNull()) return pixmap;
        const int pixelWidth = width * pixelRatio;
        const int pixelHeight = height * pixelRatio;
        const int wDiff = pixmap.width() - pixelWidth;
        const int hDiff = pixmap.height() - pixelHeight;
        if (wDiff || hDiff) {
            pixmap = pixmap.scaled(pixelWidth, pixelHeight, Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);
        }
        pixmap.setDevicePixelRatio(pixelRatio);
    }
    return pixmap;
}

void Album::fetchLastFmSearch() {
    QUrl url("http://ws.audioscrobbler.com/2.0/");
    QUrlQuery q;
    q.addQueryItem("method", "album.search");
    q.addQueryItem("api_key", Constants::LASTFM_API_KEY);
    q.addQueryItem("artist", artist->getName());
    q.addQueryItem("album", name);
    q.addQueryItem("limit", "5");
    url.setQuery(q);

    QObject *reply = HttpUtils::lastFm().get(url);
    connect(reply, SIGNAL(data(QByteArray)), SLOT(parseLastFmSearch(QByteArray)));
    connect(reply, SIGNAL(error(QString)), SIGNAL(gotInfo()));
}

void Album::parseLastFmSearch(const QByteArray &bytes) {
    QXmlStreamReader xml(bytes);

    QString artistName;
    QString albumName;

    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == QLatin1String("artist")) {
                artistName = xml.readElementText();
            } else if (xml.name() == QLatin1String("name")) {
                albumName = xml.readElementText();
            }

        } else if (xml.isEndElement()) {
            if (xml.name() == QLatin1String("album")) {
                // qDebug() << "Comparing artist name" << artist->getName() << artistName;
                if (artist->getName() == artistName) {
                    if (name != albumName) {
                        qDebug() << "Fixed album name" << name << "=>" << albumName;
                        name = albumName;
                        hash.clear();
                    }
                    break;
                }
            }
        }
    }

    /* Error handling. */
    if (xml.hasError()) {
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
            hash.clear();
            // qDebug() << "*** Redirected name is" << name;
            fetchLastFmSearch();
            return;
        }
    }
    emit gotInfo();
}

void Album::fetchLastFmInfo() {
    /*
    if (QFile::exists(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/albums/" +
    getHash())) { qDebug() << "Album" << name << "has a photo"; emit gotInfo(); return;
    } */

    if (!artist) {
        qDebug() << "Album" << name << "has no artist";
        emit gotInfo();
        return;
    }

    QUrl url("http://ws.audioscrobbler.com/2.0/");
    QUrlQuery q;
    q.addQueryItem("method", "album.getinfo");
    q.addQueryItem("api_key", Constants::LASTFM_API_KEY);
    q.addQueryItem("autocorrect", "1");
    if (mbid.isEmpty()) {
        q.addQueryItem("artist", artist->getName());
        q.addQueryItem("album", name);
    } else {
        q.addQueryItem("mbid", mbid);
    }
    url.setQuery(q);
    QObject *reply = HttpUtils::lastFm().get(url);
    connect(reply, SIGNAL(data(QByteArray)), SLOT(parseLastFmInfo(QByteArray)));
    connect(reply, SIGNAL(error(QString)), SIGNAL(gotInfo()));
}

void Album::parseLastFmInfo(const QByteArray &bytes) {
    QXmlStreamReader xml(bytes);

    QMap<QString, QVariant> trackNames;

    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("album")) {
            while (xml.readNextStartElement()) {
                const auto n = xml.name();

                if (n == QLatin1String("track")) {
                    QString number = xml.attributes().value("rank").toString();
                    if (trackNames.contains(number))
                        xml.skipCurrentElement();
                    else
                        while (xml.readNextStartElement()) {
                            if (xml.name() == QLatin1String("name")) {
                                QString title = xml.readElementText();
                                trackNames.insert(number, title);
                            } else
                                xml.skipCurrentElement();
                        }
                }

                else if (n == QLatin1String("name")) {
                    QString albumTitle = xml.readElementText();
                    if (name != albumTitle) {
                        QString newHash = getHash(albumTitle, artist);
                        if (getHash() != newHash) {
                            qDebug() << "Fixed album name" << name << "->" << albumTitle;
                            name = albumTitle;
                            hash.clear();
                        }
                    }
                }

                else if (n == QLatin1String("image") &&
                         xml.attributes().value("size") == QLatin1String("extralarge")) {
                    bool imageAlreadyPresent = property("localCover").toBool();
                    if (!imageAlreadyPresent)
                        imageAlreadyPresent = QFile::exists(getImageLocation());
                    if (!imageAlreadyPresent) {
                        QString imageUrl = xml.readElementText();
                        if (!imageUrl.isEmpty()) setProperty("imageUrl", imageUrl);
                    } else
                        xml.skipCurrentElement();
                }

                else if (n == QLatin1String("listeners")) {
                    listeners = xml.readElementText().toUInt();
                }

                else if (n == QLatin1String("releasedate") && year < 1600) {
                    QString releasedateString = xml.readElementText().simplified();
                    if (!releasedateString.isEmpty()) {
                        // Something like "6 Apr 1999, 00:00"
                        QDateTime releaseDate =
                                QDateTime::fromString(releasedateString, "d MMM yyyy, hh:mm");
                        int releaseYear = releaseDate.date().year();
                        if (releaseYear > 0) year = releaseDate.date().year();
                    }
                }

                // wiki
                else if (n == QLatin1String("wiki")) {
                    while (xml.readNextStartElement()) {
                        if (xml.name() == QLatin1String("content")) {
                            QString wiki = xml.readElementText();
                            static const QRegularExpression re("User-contributed text.*");
                            wiki.remove(re);
                            wiki = wiki.trimmed();
                            if (!wiki.isEmpty()) {
                                QString wikiLocation = getWikiLocation();
                                QDir().mkpath(QFileInfo(wikiLocation).absolutePath());
                                QFile file(wikiLocation);
                                if (!file.open(QIODevice::WriteOnly))
                                    qWarning()
                                            << "Error opening file for writing" << file.fileName();
                                QTextStream stream(&file);
                                stream << wiki;
                            }
                        } else
                            xml.skipCurrentElement();
                    }
                }

                else
                    xml.skipCurrentElement();
            }
        }
    }

    setProperty("trackNames", trackNames);

    if (xml.hasError()) qWarning() << xml.errorString();

    emit gotInfo();
}

QString Album::getBaseLocation() {
    return Database::getFilesLocation() + getHash();
}

QString Album::getImageLocation() {
    return getBaseLocation() + QLatin1String("/_cover");
}

QString Album::getWikiLocation() {
    return getBaseLocation() + QLatin1String("/_wiki");
}

void Album::setPhoto(const QByteArray &bytes) {
    qDebug() << "Storing photo for" << name;

    // store photo
    QString storageLocation = getImageLocation();

    QFileInfo info(storageLocation);
    QDir().mkpath(info.absolutePath());

    QFile file(storageLocation);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Error opening file for writing" << file.fileName();
    }
    QDataStream stream(&file);
    stream.writeRawData(bytes.constData(), bytes.size());

    emit gotPhoto();
}

QList<Track *> Album::getTracks() {
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select id from tracks where album=? order by disk, track, path");
    query.bindValue(0, id);
    bool success = query.exec();
    if (!success) qDebug() << query.lastQuery() << query.lastError();
    QList<Track *> tracks;
    tracks.reserve(query.size());
    while (query.next()) {
        int trackId = query.value(0).toInt();
        Track *track = Track::forId(trackId);
        tracks << track;
    }
    return tracks;
}

QString Album::getWiki() {
    QFile file(getWikiLocation());
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
    // QRegularExpression re("\\b(the|a|of|and|n|or|s|is)\\b");
    s2 = s;
    static const QRegularExpression shortWordsRE("\\b([a-z]{1,1}|the|of|and|or|is)\\b");
    s2.remove(shortWordsRE);
    if (s2.simplified().length() > 4) s = s2;
    s2.clear();

    // keep only letters
    int stringSize = s.size();
    for (int i = 0; i < stringSize; ++i) {
        const QChar c = s.at(i);
        if (c.isLetter()) s2.append(c);
    }
    s = s2;
    s2.clear();

    // simplify accented chars èé=>e etc
    static QList<QPair<QChar, QString>> charVariants;
    static int charVariantsSize;
    if (charVariants.isEmpty()) {
        charVariants << QPair<QChar, QString>('a', "áàâäãåāăą")
                     << QPair<QChar, QString>('e', "éèêëēĕėęě")
                     << QPair<QChar, QString>('i', "íìıîïĩīĭį")
                     << QPair<QChar, QString>('o', "óòôöõōŏőơ")
                     << QPair<QChar, QString>('u', "úùûüũūŭůűųư")
                     << QPair<QChar, QString>('c', "çćčĉċ") << QPair<QChar, QString>('d', "đ")
                     << QPair<QChar, QString>('g', "ğĝġģǵ") << QPair<QChar, QString>('h', "ĥħ")
                     << QPair<QChar, QString>('j', "ĵ") << QPair<QChar, QString>('k', "ķĸ")
                     << QPair<QChar, QString>('l', "ĺļľŀ") << QPair<QChar, QString>('n', "ñńņňŉŋ")
                     << QPair<QChar, QString>('r', "ŕŗř") << QPair<QChar, QString>('s', "śŝſș")
                     << QPair<QChar, QString>('t', "ţťŧț") << QPair<QChar, QString>('r', "ŕŗř")
                     << QPair<QChar, QString>('w', "ŵ") << QPair<QChar, QString>('y', "ýÿŷ")
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
        newTitle.remove(QRegularExpression(" *\\(.*\\)"));
    else if (newTitle.count('(') > 1) {
        int i = newTitle.indexOf('(');
        if (i != -1) {
            i = newTitle.indexOf('(', i + 1);
            if (i != -1) newTitle = newTitle.left(i).simplified();
        }
    }

    if (trackTitle == newTitle) return newTitle;

    QString normalizedNewTitle = normalizeString(newTitle);
    if (normalizedNewTitle.isEmpty()) return QString();

    QString normalizedTrackTitle = normalizeString(trackTitle);
    if (normalizedTrackTitle.isEmpty()) return QString();

    if (normalizedNewTitle == normalizedTrackTitle ||
        (normalizedTrackTitle.size() > 3 && normalizedNewTitle.contains(normalizedTrackTitle)) ||
        (normalizedNewTitle.size() > 3 && normalizedTrackTitle.contains(normalizedNewTitle)) ||
        (track->getNumber() && normalizedTrackTitle == "track")) {
#ifndef QT_NO_DEBUG_OUTPUT
        if (trackTitle.toLower() != newTitle.toLower())
            qDebug() << "✓" << artist->getName() << name << trackTitle << "->" << newTitle;
#endif
        return newTitle;
    } else {
        // qDebug() << artist->getName() << name << normalizedTrackTitle << "!=" <<
        // normalizedNewTitle;
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
            qDebug() << "✓" << artist->getName() << name << "[empty title]"
                     << "->" << newTitle;
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
