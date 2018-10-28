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

#include "diskcache.h"
#include <QtNetwork>

DiskCache::DiskCache(QObject *parent) : QNetworkDiskCache(parent) { }

QIODevice* DiskCache::prepare(const QNetworkCacheMetaData &metaData) {
    QString mime;
    foreach (const QNetworkCacheMetaData::RawHeader &header, metaData.rawHeaders()) {
        // qDebug() << header.first << header.second;
        if (header.first.constData() == "Content-Type") {
            mime = header.second;
            break;
        }
    }

    if (mime.startsWith(QLatin1String("text/"))
            || mime.startsWith(QLatin1String("application/xml"))
            || mime.startsWith(QLatin1String("application/json)"))
            || mime.startsWith(QLatin1String("application/atom+xml"))
            ) {
        return QNetworkDiskCache::prepare(metaData);
    }

    return nullptr;
}
