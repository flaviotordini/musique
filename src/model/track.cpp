#include "track.h"
#include "album.h"
#include "artist.h"

#include "../database.h"
#include "../datautils.h"
#include <QtSql>

#include "../networkaccess.h"
#include <QtNetwork>
#include "../mbnetworkaccess.h"

#include <mpegfile.h>
#include <id3v2tag.h>
#include <unsynchronizedlyricsframe.h>

namespace The {
    NetworkAccess* http();
}

Track::Track() {
    album = 0;
    artist = 0;
    number = 0;
    year = 0;
    length = 0;
    /*
    start = 0;
    end = 0;
    */
    played = false;
    startTime = 0;
}

QHash<int, Track*> Track::cache;
QHash<QString, Track*> Track::pathCache;

Track* Track::forId(int trackId) {

    if (cache.contains(trackId)) {
        // get from cache
        // qDebug() << "Track was cached" << trackId;
        return cache.value(trackId);
    }

    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select path, title, duration, track, artist, album from tracks where id=?");
    query.bindValue(0, trackId);
    bool success = query.exec();
    if (!success) qDebug() << query.lastQuery() << query.lastError().text();
    if (query.next()) {
        Track* track = new Track();
        track->setId(trackId);
        track->setPath(query.value(0).toString());
        track->setTitle(query.value(1).toString());

        // TODO start & end

        track->setLength(query.value(2).toInt());
        track->setNumber(query.value(3).toInt());

        // relations
        // TODO this could be made lazy
        int artistId = query.value(4).toInt();
        track->setArtist(Artist::forId(artistId));
        int albumId = query.value(5).toInt();
        track->setAlbum(Album::forId(albumId));

        // put into cache
        cache.insert(trackId, track);
        if (!pathCache.contains(track->getPath()))
            pathCache.insert(track->getPath(), track);

        return track;
    }

    // id not found
    cache.insert(trackId, 0);
    return 0;
}

Track* Track::forPath(QString path) {
    // qDebug() << "Track::forPath" << path;
    Track *track = 0;
    if (pathCache.contains(path)) {
        track = pathCache.value(path);
    } else {
        int id = Track::idForPath(path);
        if (id != -1) track = Track::forId(id);
    }
    return track;
}

int Track::idForPath(QString path) {
    int id = -1;
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select id from tracks where path=?");
    query.bindValue(0, path);
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
    if (query.next()) {
        id = query.value(0).toInt();
    }
    return id;
}

bool Track::exists(QString path) {
    // qDebug() << "Track::exists";
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select count(*) from tracks where path=?");
    query.bindValue(0, path);
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
    if (query.next()) {
        return query.value(0).toBool();
    }
    return false;
}

bool Track::isModified(QString path, uint lastModified) {
    // qDebug() << "Track::isModified";
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select id from tracks where path=? and tstamp<?");
    query.bindValue(0, path);
    query.bindValue(1, lastModified);
    // qDebug() << query.lastQuery() << query.boundValues().values();
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
    return query.next();
}

void Track::insert() {
    // qDebug() << "Track::insert";
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("insert into tracks (path, title, track, year, album, artist, tstamp, duration) values (?,?,?,?,?,?,?,?)");
    query.bindValue(0, path);
    query.bindValue(1, title);
    query.bindValue(2, number);
    query.bindValue(3, year);
    int albumId = album ? album->getId() : 0;
    query.bindValue(4, albumId);
    int artistId = artist ? artist->getId() : 0;
    query.bindValue(5, artistId);
    query.bindValue(6, QDateTime::currentDateTime().toTime_t());
    query.bindValue(7, length);
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();

    // increment artist's track count
    if (artist && artist->getId()) {
        QSqlQuery query(db);
        query.prepare("update artists set trackCount=trackCount+1 where id=?");
        query.bindValue(0, artist->getId());
        bool success = query.exec();
        if (!success) qDebug() << query.lastError().text();
    }

    // increment album's track count
    if (album && album->getId()) {
        QSqlQuery query(db);
        query.prepare("update albums set trackCount=trackCount+1 where id=?");
        query.bindValue(0, album->getId());
        bool success = query.exec();
        if (!success) qDebug() << query.lastError().text();
    }
}

void Track::update() {

    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);

    query.prepare("select album, artist from tracks where path=?");
    query.bindValue(0, path);
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
    if (query.next()) {
        int albumId = query.value(0).toInt();
        int artistId = query.value(1).toInt();

        if (!album || album->getId() != albumId) {
            // decrement previous album track count
            query.prepare("update albums set trackCount=trackCount-1 where id=?");
            query.bindValue(0, albumId);
            success = query.exec();
            if (!success) qDebug() << query.lastError().text();
            // and increment the new album track count
            if (album) {
                query.prepare("update albums set trackCount=trackCount+1 where id=?");
                query.bindValue(0, album->getId());
                bool success = query.exec();
                if (!success) qDebug() << query.lastError().text();
            }
        }

        if (!artist || artist->getId() != artistId) {
            query.prepare("update artists set trackCount=trackCount-1 where id=?");
            query.bindValue(0, artistId);
            success = query.exec();
            if (!success) qDebug() << query.lastError().text();
            if (artist) {
                query.prepare("update artists set trackCount=trackCount+1 where id=?");
                query.bindValue(0, artist->getId());
                bool success = query.exec();
                if (!success) qDebug() << query.lastError().text();
            }
        }

    }

    // qDebug() << "Track::update";

    query.prepare("update tracks set title=?, track=?, year=?, album=?, artist=?, tstamp=?, duration=? where path=?");

    query.bindValue(0, title);
    query.bindValue(1, number);
    query.bindValue(2, year);
    int albumId = album ? album->getId() : 0;
    query.bindValue(3, albumId);
    int artistId = artist ? artist->getId() : 0;
    query.bindValue(4, artistId);
    query.bindValue(5, QDateTime().toTime_t());
    query.bindValue(6, length);
    query.bindValue(7, path);
    success = query.exec();
    if (!success) qDebug() << query.lastError().text();
}

void Track::remove(QString path) {
    // qDebug() << "Track::remove";

    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);

    // first update trackCount on artist and album

    query.prepare("select album, artist from tracks where path=?");
    query.bindValue(0, path);
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
    if (query.next()) {

        int albumId = query.value(0).toInt();
        int artistId = query.value(1).toInt();

        query.prepare("update albums set trackCount=trackCount-1 where id=?");
        query.bindValue(0, albumId);
        success = query.exec();
        if (!success) qDebug() << query.lastError().text();

        query.prepare("update artists set trackCount=trackCount-1 where id=?");
        query.bindValue(0, artistId);
        success = query.exec();
        if (!success) qDebug() << query.lastError().text();

    }

    // and then actually delete the track
    query.prepare("delete from tracks where path=?");
    query.bindValue(0, path);
    success = query.exec();
    if (!success) qDebug() << query.lastError().text();

    // update cache and notify everybody using this track
    // that it is gone forever
    int trackId = Track::idForPath(path);
    if (trackId != -1) {
        if (cache.contains(trackId)) {
            Track* track = cache.value(trackId);
            track->emitRemovedSignal();
            cache.remove(trackId);
            track->deleteLater();
        }
    }

}

void Track::emitRemovedSignal() {
    emit removed();
}

QString Track::getHash() {
    return Track::getHash(title);
}

QString Track::getHash(QString name) {
    // return DataUtils::calculateHash(DataUtils::normalizeTag(name));
    return DataUtils::normalizeTag(name);
}

QString Track::getStatusTip() {
    QString tip = QString::fromUtf8("♫ ");
    Artist* artist = getArtist();
    if (artist) tip += artist->getName() + " - ";
    Album* album = getAlbum();
    if (album) tip += album->getName() + " - ";
    tip += getTitle();
    int duration = getLength();
    if (duration) tip += " (" + QString::number(duration) + ")";
    return tip;
}

void Track::fetchInfo() {
    emit gotInfo();
    // fetchMusicBrainzTrack();
}

// *** MusicBrainz ***

void Track::fetchMusicBrainzTrack() {

    QString s = "http://musicbrainz.org/ws/1/track/?type=xml&title=%1&limit=1";
    s = s.arg(title);
    if (artist) {
        s = s.append("&artist=%2").arg(artist->getName());
    };

    QUrl url(s);
    MBNetworkAccess *http = new MBNetworkAccess();
    QObject *reply = http->get(url);
    connect(reply, SIGNAL(data(QByteArray)), SLOT(parseMusicBrainzTrack(QByteArray)));
    connect(reply, SIGNAL(error(QNetworkReply*)), SIGNAL(gotInfo()));
}

void Track::parseMusicBrainzTrack(QByteArray bytes) {
    QString correctTitle = DataUtils::getXMLElementText(bytes, "title");
    qDebug() << title << "-> MusicBrainz ->" << correctTitle;
    if (!correctTitle.isEmpty()) {
        this->title = correctTitle;
    }

    emit gotInfo();
}

QString Track::getAbsolutePath() {
    QString collectionRoot = Database::instance().collectionRoot();
    QString absolutePath = collectionRoot + "/" + path;
    return absolutePath;
}

QString Track::getLyricsLocation() {
    static const QString data = QDesktopServices::storageLocation(QDesktopServices::DataLocation)
            + QLatin1String("/data/");
    QString l = data;
    if (artist) l += artist->getHash();
    l += QLatin1String("/_lyrics/") + getHash();
    return l;
}

void Track::getLyrics() {
#ifdef APP_MAC_STORE_NO
    bool haveStyle = (QFile::exists(qApp->applicationDirPath() + "/../Resources/style.css"));
    if (!haveStyle) {
        return;
    }
#endif

    QFile file(getLyricsLocation());
    if (file.exists()) {
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Cannot open file" << file.fileName();
        } else {
            QByteArray bytes = file.readAll();
            emit gotLyrics(QString::fromUtf8(bytes.data()));
            return;
        }
    }

    QString artistName;
    if (artist) artistName = artist->getName();

    // http://lyrics.wikia.com/LyricWiki:REST
    QUrl url = QString(
            "http://lyrics.wikia.com/api.php?func=getSong&artist=%1&song=%2&fmt=xml")
            .arg(QString::fromUtf8(QUrl::toPercentEncoding(artistName)))
            .arg(QString::fromUtf8(QUrl::toPercentEncoding(title)));

    QObject *reply = The::http()->get(url);
    connect(reply, SIGNAL(data(QByteArray)), SLOT(parseLyricsSearchResults(QByteArray)));
    // connect(reply, SIGNAL(error(QNetworkReply*)), SIGNAL(gotLyrics()));
}

void Track::parseLyricsSearchResults(QByteArray bytes) {
    QString lyricsText = DataUtils::getXMLElementText(bytes, "lyrics");
    if (lyricsText == "Instrumental") {
        emit gotLyrics(lyricsText);
        return;
    }

    QString lyricsUrl = DataUtils::getXMLElementText(bytes, "url");
    if (lyricsUrl.contains("action=edit")) {
        // Lyrics not found
        // qDebug() << "Lyrics not available for" << title;
        readLyricsFromTags();

    } else {
        // Lyrics found, get them
        QUrl url = QUrl::fromEncoded(lyricsUrl.toUtf8());
        QObject *reply = The::http()->get(url);
        connect(reply, SIGNAL(data(QByteArray)), SLOT(scrapeLyrics(QByteArray)));
        // connect(reply, SIGNAL(error(QNetworkReply*)), SIGNAL(gotLyrics()));
    }
}

void Track::scrapeLyrics(QByteArray bytes) {
    QString lyrics = QString::fromUtf8(bytes);


    int pos = lyrics.indexOf( "'lyricbox'" );
    int startPos = lyrics.indexOf( ">", pos ) + 1;
    int endPos = lyrics.indexOf( "</div>", startPos );
    int otherDivPos = lyrics.indexOf( "<div", startPos );
    while ( otherDivPos != -1 && otherDivPos < endPos ) {
        endPos = lyrics.indexOf( "</div>", endPos + 1 );
        otherDivPos = lyrics.indexOf( "<div", otherDivPos + 1 );
    }
    lyrics = lyrics.mid(startPos, endPos-startPos);

    // strip adverts
    pos = lyrics.indexOf("<div");
    while( pos != -1 ) {
        startPos = lyrics.lastIndexOf( "<div", pos);
        endPos = lyrics.indexOf( "</div>", pos);
        // qDebug() << "<div" << startPos << endPos << lyrics;
        lyrics = lyrics.left(startPos) + lyrics.mid(endPos + 6);
        pos = lyrics.indexOf("<div");
    }

    // strip comments
    pos = lyrics.indexOf("<!--");
    while( pos != -1 ) {
        startPos = lyrics.lastIndexOf( "<!--", pos);
        endPos = lyrics.indexOf( "-->", pos);
        // qDebug() << "<!--:" << startPos << endPos << lyrics;
        lyrics = lyrics.left(startPos) + lyrics.mid(endPos + 3);
        pos = lyrics.indexOf("<!--");
    }

    // drop partial lyrics
    if (lyrics.indexOf("Special:Random") != -1) {
        qDebug() << "Discarding incomplete lyrics for" << title;
        readLyricsFromTags();
        return;
    }

    lyrics = lyrics.simplified();

    // cache lyrics
    QString filePath = getLyricsLocation();
    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Error opening file for writing" << file.fileName();
    } else {
        QTextStream stream(&file);
        stream << lyrics;
    }

    emit gotLyrics(lyrics);
}

void Track::readLyricsFromTags() {

    const QString absolutePath = getAbsolutePath();

    const QString suffix = QFileInfo(absolutePath).suffix().toLower();
    if (suffix != "mp3") return;

    TagLib::MPEG::File f((TagLib::FileName) absolutePath.toUtf8());
    if (!f.isValid()) return;

    TagLib::ID3v2::Tag *tag = f.ID3v2Tag();
    if (!tag) return;

    TagLib::ID3v2::FrameList list = tag->frameList("USLT");
    if (list.isEmpty()) return;

    TagLib::ID3v2::UnsynchronizedLyricsFrame *frame =
            static_cast<TagLib::ID3v2::UnsynchronizedLyricsFrame *>(list.front());
    if (!frame) return;

    QString lyrics = QString::fromUtf8(frame->text().toCString(true));
    emit gotLyrics(lyrics);

}

int Track::getTotalLength(QList<Track *>tracks) {
    int length = 0;
    foreach (Track* track, tracks) {
        length += track->getLength();
    }
    return length;
}
