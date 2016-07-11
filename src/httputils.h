#ifndef HTTPUTILS_H
#define HTTPUTILS_H

#include <QtCore>

class Http;

class HttpUtils {

public:
    static Http &musicBrainz();
    static Http &lastFm();
    static Http &cached();
    static const QByteArray &userAgent();

private:
    HttpUtils() { }

};

#endif // HTTPUTILS_H
