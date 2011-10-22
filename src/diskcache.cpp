#include "diskcache.h"
#include <QtNetwork>

DiskCache::DiskCache(QObject *parent) : QNetworkDiskCache(parent) { }

QIODevice* DiskCache::prepare(const QNetworkCacheMetaData &metaData) {
    QString mime;
    foreach (QNetworkCacheMetaData::RawHeader header, metaData.rawHeaders()) {
        // qDebug() << header.first << header.second;
        if (header.first == "Content-Type") {
            mime = header.second;
            break;
        }
    }

    if (mime.startsWith("text/")
            || mime.startsWith("application/xml")
            || mime.startsWith("application/json")
            || mime.startsWith("application/atom+xml")
            ) {
        // qDebug() << "OK cache";
        return QNetworkDiskCache::prepare(metaData);
    } else {
        // qDebug() << "Don't cache" << metaData.url();
    }

    return 0;
}
