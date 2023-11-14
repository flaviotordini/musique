#include "genre.h"

#include <QtSql>

#include "../database.h"
#include "../datautils.h"
#include "iconutils.h"

#include "artist.h"
#include "track.h"

namespace {
QHash<int, Genre *> cache;
QHash<QString, Genre *> hashCache;

QString toCamelCase(const QString &s) {
    QString s2;
    s2.reserve(s.size());
    bool firstChar = true;
    for (const QChar &c : s) {
        if (firstChar)
            s2.append(c.toUpper());
        else
            s2.append(c);
    }
    return s2;
}

} // namespace

void Genre::clearCache() {
    qDeleteAll(cache);
    cache.clear();
    cache.squeeze();
    hashCache.clear();
    hashCache.squeeze();
}

Genre *Genre::forId(int id) {
    auto i = cache.constFind(id);
    if (i != cache.constEnd()) return i.value();

    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select hash,name,trackCount from genres where id=?");
    query.bindValue(0, id);
    bool success = query.exec();
    if (!success) qDebug() << query.lastQuery() << query.lastError().text();

    Genre *genre = nullptr;
    if (query.next()) {
        genre = new Genre();
        genre->setId(id);
        const QString hash = query.value(0).toString();
        genre->setHash(hash);
        genre->setName(query.value(1).toString());
        genre->setTrackCount(query.value(2).toInt());
        hashCache.insert(hash, genre);
    }
    cache.insert(id, genre);
    return genre;
}

Genre *Genre::maybeCreateByName(const QString &name) {
    const QString hash = DataUtils::normalizeTag(name);
    if (hash.isEmpty()) return nullptr;
    auto i = hashCache.constFind(hash);
    if (i != hashCache.constEnd()) return i.value();
    Genre *genre = nullptr;
    int id = Genre::idForHash(hash);
    if (id != -1)
        genre = Genre::forId(id);
    else {
        // Insert it
        QSqlDatabase db = Database::instance().getConnection();
        QSqlQuery query(db);
        query.prepare("insert into genres "
                      "(hash,name,trackCount) "
                      "values (?,?,0)");
        query.bindValue(0, hash);
        query.bindValue(1, name);
        bool success = query.exec();
        if (!success)
            qDebug() << query.lastError().text();
        else {
            int id = query.lastInsertId().toInt();
            genre = Genre::forId(id);
        }
    }
    return genre;
}

Genre *Genre::forHash(const QString &hash) {
    if (hash.isEmpty()) return nullptr;
    auto i = hashCache.constFind(hash);
    if (i != hashCache.constEnd()) return i.value();
    Genre *genre = nullptr;
    int id = Genre::idForHash(hash);
    if (id != -1)
        genre = Genre::forId(id);
    else {
        genre = new Genre();
        genre->setHash(hash);
        genre->setName(toCamelCase(hash));
        genre->setId(-1);
    }
    return genre;
}

int Genre::idForHash(const QString &hash) {
    int id = -1;
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select id from genres where hash=?");

    query.bindValue(0, hash);
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
    if (query.next()) {
        id = query.value(0).toInt();
    }
    return id;
}

QString Genre::cleanGenreName(QString &genreName) {
    static const auto replacements = [] {
        QList<QPair<QString, QString>> map;
        QFile f(":/res/genre-replacements.csv");
        if (f.open(QFile::ReadOnly)) {
            QTextStream stream(&f);
            QString line;
            while (!stream.atEnd()) {
                stream.readLineInto(&line);
                QString badWord;
                QString goodWord;
                const auto fields = line.split(',');
                for (const auto &field : fields) {
                    if (badWord.isNull())
                        badWord = field;
                    else
                        goodWord = field;
                }
                map.append(qMakePair(badWord, goodWord));
            }
        }
        return map;
    }();

    genreName = genreName.trimmed();
    // if (genreName.startsWith(QLatin1String("& "))) genreName = genreName.mid(2);
    if (genreName.isEmpty()) return QString();

    QString s = genreName;
    for (auto &i : replacements) {
        s.replace(i.first, i.second, Qt::CaseInsensitive);
    }

    if (genreName != s) qDebug() << genreName << s;
    return s.simplified();
}

Genre::Genre(QObject *parent)
    : Item(parent), trackCount(0), pixmapArtist(nullptr), parent(nullptr), row(-1) {}

QList<Track *> Genre::getTracks() {
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);

    QStringList ids{QString::number(id)};
    for (Genre *g : qAsConst(children)) {
        ids << QString::number(g->getId());
    }
    query.prepare("select distinct t.id from genretracks g, tracks t where t.id=g.track and "
                  "g.genre in (" +
                  ids.join(',') +
                  ") "
                  "order by t.year desc, t.album, t.disk, t.track, t.path");
    bool success = query.exec();
    if (!success) qDebug() << query.lastQuery() << query.lastError();
    qDebug() << query.lastQuery();
    QList<Track *> tracks;
    tracks.reserve(query.size());
    while (query.next()) {
        int trackId = query.value(0).toInt();
        Track *track = Track::forId(trackId);
        tracks << track;
    }
    return tracks;
}

int Genre::getTotalTrackCount() const {
    int count = trackCount;
    for (Genre *g : qAsConst(children))
        count += g->getTrackCount();
    return count;
}

QPixmap Genre::getThumb(int width, int height, qreal pixelRatio) {
    if (!pixmapArtist) pixmapArtist = randomArtist();
    if (!pixmapArtist) return pixmap;
    if (pixmap.isNull() || pixmap.devicePixelRatio() != pixelRatio ||
        pixmap.width() != width * pixelRatio) {
        pixmap = pixmapArtist->getPhoto();
        if (pixmap.isNull()) return pixmap;

        const int pixelWidth = width * pixelRatio;
        const int pixelHeight = height * pixelRatio;
        const int wDiff = pixmap.width() - pixelWidth;
        const int hDiff = pixmap.height() - pixelHeight;
        if (wDiff || hDiff) {
            pixmap = pixmap.scaled(pixelWidth, pixelHeight, Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);
        }
        /*
        QImage img = pixmap.toImage();
        if (!img.isGrayscale()) img = img.convertToFormat(QImage::Format_Grayscale8);
        pixmap = QPixmap::fromImage(img);
        */
        pixmap.setDevicePixelRatio(pixelRatio);
    }
    return pixmap;
}

Artist *Genre::randomArtist() {
    Artist *artist = nullptr;
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select a.id from genreTracks g, tracks t, artists a "
                  "where g.track=t.id and t.artist=a.id and g.genre=? "
                  "order by random() limit 10");

    query.bindValue(0, id);
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
    while (query.next()) {
        artist = Artist::forId(query.value(0).toInt());
        if (artist->hasPhoto()) break;
    }
    return artist;
}

int Genre::getRow() const {
    return row;
}

void Genre::setRow(int value) {
    row = value;
}
