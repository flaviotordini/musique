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
    get();
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
