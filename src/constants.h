#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>
#include <QDesktopServices>

namespace Constants {
    static const char *VERSION = "0.0";
    static const int DATABASE_VERSION = 1;
    static const char *APP_NAME = "Minitunes";
    static const char *ORG_NAME = "Flavio Tordini";
    static const char *ORG_DOMAIN = "flavio.tordini.org";
    static const char *WEBSITE = "http://flavio.tordini.org/minitunes";
    static const char *EMAIL = "flavio.tordini@gmail.com";
    static const QString USER_AGENT = QString(APP_NAME) + " " + VERSION + " (" + WEBSITE + ")";
    static const char* LASTFM_API_KEY = "e1db9fda381dea473df994bc26dfa1f1";
}

#endif
