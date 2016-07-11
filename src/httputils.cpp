#include "httputils.h"
#include "constants.h"
#include "http.h"
#include "throttledhttp.h"
#include "cachedhttp.h"

Http &HttpUtils::musicBrainz() {
    static Http *h = [] {
        Http *http = new Http;
        http->addRequestHeader("User-Agent", userAgent());

        ThrottledHttp *throttledHttp = new ThrottledHttp(*http);
        throttledHttp->setMilliseconds(1100);

        CachedHttp *cachedHttp = new CachedHttp(*throttledHttp, "mb");
        cachedHttp->setMaxSeconds(86400 * 30);
        cachedHttp->setMaxSize(0);

        return cachedHttp;
    }();
    return *h;
}

Http &HttpUtils::lastFm() {
    static Http *h = [] {
        Http *http = new Http;
        http->addRequestHeader("User-Agent", userAgent());

        ThrottledHttp *throttledHttp = new ThrottledHttp(*http);
        throttledHttp->setMilliseconds(200);

        CachedHttp *cachedHttp = new CachedHttp(*throttledHttp, "lf");
        cachedHttp->setMaxSeconds(86400 * 30);
        cachedHttp->setMaxSize(0);

        return cachedHttp;
    }();
    return *h;
}

Http &HttpUtils::cached() {
    static Http *h = [] {
        Http *http = new Http;
        http->addRequestHeader("User-Agent", userAgent());

        CachedHttp *cachedHttp = new CachedHttp(*http, "http");
        cachedHttp->setMaxSeconds(86400 * 30);
        cachedHttp->setMaxSize(0);

        return cachedHttp;
    }();
    return *h;
}

const QByteArray &HttpUtils::userAgent() {
    static const QByteArray ua = [] {
        return QString(QLatin1String(Constants::NAME)
                       + QLatin1Char('/') + QLatin1String(Constants::VERSION)
                       + QLatin1String(" ( ") + Constants::WEBSITE + QLatin1String(" )")).toUtf8();
    }();
    return ua;
}
