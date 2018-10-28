#include "tagchecker.h"
#include "tags.h"

namespace {

QRegularExpression re(const QString &s) {
    return QRegularExpression(QLatin1Char('^') + s + QLatin1Char('$'));
}

bool isBlacklisted(const QVector<QRegularExpression> &blacklist, const QString &tag) {
    for (const QRegularExpression &re : blacklist) {
        if (re.match(tag).hasMatch()) {
            qDebug() << "Blacklisted tag" << tag;
            return true;
        }
    }
    return false;
}

bool isInvalid(const QString &tag) {
    if (tag.isEmpty()) return true;

    static const QVector<QRegularExpression> blacklist =
            QVector<QRegularExpression>() << re("unknown") << re("no title") << re("missing");

    if (isBlacklisted(blacklist, tag)) return true;

    // number only. May be valid, but high probability of being invalid
    static const QRegularExpression numberRE("^[0-9]+$");
    if (numberRE.match(tag).hasMatch()) {
        // qDebug() << "Tag is number only" << tag;
        return true;
    }

    return false;
}

bool isInvalidArtist(const QString &tag) {
    QString t = tag.simplified().toLower();

    if (isInvalid(t)) return true;
    static const QVector<QRegularExpression> blacklist = QVector<QRegularExpression>()
                                                         << re("various artists") << re("various")
                                                         << re("artist") << re("artist name")
                                                         << re("no artist") << re("insert artist");
    return isBlacklisted(blacklist, t);
}

bool isInvalidAlbum(const QString &tag) {
    QString t = tag.simplified().toLower();

    if (isInvalid(t)) return true;
    static const QVector<QRegularExpression> blacklist = QVector<QRegularExpression>()
                                                         << re("album title") << re("album")
                                                         << re("no album") << re("insert album");
    return isBlacklisted(blacklist, t);
}

bool isInvalidTrack(const QString &tag) {
    QString t = tag.simplified().toLower();

    if (isInvalid(t)) return true;
    static const QVector<QRegularExpression> blacklist =
            QVector<QRegularExpression>()
            << re("untitled") << re("(audio)?track [0-9]+") << re("track (no?\\.?|#) ?[0-9]+")
            << re("track title") << re("no track") << re("insert track");
    return isBlacklisted(blacklist, t);
}

} // namespace

namespace TagChecker {

bool checkTags(Tags *tags) {
    if (tags->getTrackNumber() <= 0) {
        // qDebug() << "no track number";
        return true;
    }
    if (tags->getYear() <= 0) {
        // qDebug() << "no year";
        return true;
    }

    return isInvalidArtist(tags->getArtistString()) || isInvalidAlbum(tags->getAlbumString()) ||
           isInvalidTrack(tags->getTitle());
}

} // namespace TagChecker
