#ifndef HTTPUTILS_H
#define HTTPUTILS_H

#include <QtCore>

class Http;

class HttpUtils {

public:
    static Http &musicBrainz();
    static const QByteArray &userAgent();
    static const QByteArray &stealthUserAgent();

private:
    HttpUtils() { }

};

#endif // HTTPUTILS_H
