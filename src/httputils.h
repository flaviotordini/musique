#ifndef HTTPUTILS_H
#define HTTPUTILS_H

#include <QtCore>

class Http;

class HttpUtils {
public:
    static Http &lastFm();
    static Http &discogs();
    static Http &cached();
    static Http &notCached();
    static const QByteArray &userAgent();

private:
    HttpUtils() {}
};

#endif // HTTPUTILS_H
