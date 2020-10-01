#include "lyrics.h"

#include "js.h"
#include "localcache.h"
#include "sharedcache.h"

namespace {

LocalCache &localCache() {
    static auto i = [] {
        auto i = LocalCache::instance("ly");
        i->setMaxSeconds(0);
        return i;
    }();
    return *i;
}

SharedCache &sharedCache() {
    static auto i = [] {
        auto i = new SharedCache();
        i->setBaseUrl("http://c.tordini.org/");
        i->setGroup("ly");
        return i;
    }();
    return *i;
}

QString simplify(QString s) {
    s.replace(QString::fromUtf8("’"), QLatin1String("'"));
    return s;
}

} // namespace

Lyrics::Lyrics(QObject *parent) : QObject(parent) {}

Lyrics &Lyrics::get(QString artist, QString song) {
    auto l = new Lyrics;
    connect(l, &Lyrics::data, l, &QObject::deleteLater);
    connect(l, &Lyrics::error, l, &QObject::deleteLater);

    artist = simplify(artist);
    song = simplify(song);
    const QString keyString = artist + '|' + song;

    const QByteArray key = LocalCache::hash(keyString.toUtf8());
    QByteArray value = localCache().value(key);
    if (!value.isEmpty()) {
        qDebug() << "Lyrics from cache" << value;
        QString lyrics = QString::fromUtf8(value);
        QTimer::singleShot(0, l, [l, lyrics] { emit l->data(lyrics); });
        return *l;
    }

    const QString cckey = SharedCache::hash(keyString);
    connect(sharedCache().value(cckey), &HttpReply::finished, l,
            [l, cckey, key, artist, song](auto &reply) {
                if (reply.isSuccessful()) {
                    auto lyrics = reply.body();
                    localCache().insert(key, lyrics);
                    emit l->data(lyrics);
                } else {
                    JS::instance()
                            .callFunction(new JSResult(l), "lyrics", {artist, song})
                            .onString([l, cckey, key](auto &lyrics) {
                                auto bytes = lyrics.toUtf8();
                                if (!lyrics.isEmpty())
                                    sharedCache().insert(cckey, bytes, "text/plain");

                                // insert empty values too, so we don't keep getting them in the
                                // short term
                                localCache().insert(key, bytes);
                                emit l->data(lyrics);
                            })
                            .onError([l](auto &msg) { l->emit error(msg); });
                }
            });
    return *l;
}
