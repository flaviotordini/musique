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

#include "mbnetworkaccess.h"

namespace The {
    NetworkAccess* http();
}

MBNetworkAccess::MBNetworkAccess() {
    rateTimer = new QTimer(this);
    rateTimer->setSingleShot(true);
    rateTimer->setInterval(500);
    connect(rateTimer, SIGNAL(timeout()), SLOT(get()));
}

QObject* MBNetworkAccess::get(const QUrl url) {
    rateTimer->setProperty("url", url);
    return get();
}

QObject* MBNetworkAccess::get() {
    QUrl url = rateTimer->property("url").toUrl();
    // qDebug() << "get" << url;

    // MusicBrainz request rate control
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    static uint unixTime = 0;
    // static uint requests = 0;
    uint now = QDateTime::currentDateTime().toTime_t();
    // qDebug() << now;
    if (now == unixTime) {
        //requests++;
        //if (requests > 0) {
        // qDebug() << "Too many requests";
        rateTimer->start();
        return this;
        //}
    }
    unixTime = now;
    // requests = 0;

    return reallyGet();

}

QObject* MBNetworkAccess::reallyGet() {
    // qDebug() << "reallyGet";
    QUrl url = rateTimer->property("url").toUrl();
    QObject* reply = The::http()->get(url);
    connect(reply, SIGNAL(data(QByteArray)), this, SIGNAL(data(QByteArray)));
    connect(reply, SIGNAL(error(QNetworkReply*)), this, SIGNAL(error(QNetworkReply*)));

    // this will cause the deletion of this object once the request is finished
    setParent(reply);

    return reply;
}
