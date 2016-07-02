#include "httputils.h"
#include "constants.h"
#include "http.h"
#include "throttledhttp.h"
#include "cachedhttp.h"

Http &HttpUtils::musicBrainz() {
    static Http *mbHttp = [] {
        Http *http = new Http;
        http->addRequestHeader("User-Agent", userAgent());

        ThrottledHttp *throttledHttp = new ThrottledHttp(*http);
        throttledHttp->setMilliseconds(1100);

        CachedHttp *cachedHttp = new CachedHttp(*throttledHttp, "mb");
        cachedHttp->setMaxSeconds(86400 * 30);
        cachedHttp->setMaxSize(0);

        return cachedHttp;
    }();
    return *mbHttp;
}

const QByteArray &HttpUtils::userAgent() {
    static const QByteArray ua = [] {
        return QString(QLatin1String(Constants::NAME)
                       + QLatin1Char('/') + QLatin1String(Constants::VERSION)
                       + QLatin1String(" ( ") + Constants::WEBSITE + QLatin1String(" )")).toUtf8();
    }();
    return ua;
}

const QByteArray &HttpUtils::stealthUserAgent() {
    static const QByteArray ua = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_11_4) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/49.0.2623.112 Safari/537.36";
    return ua;
}
