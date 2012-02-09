#include "constants.h"
#include <QtCore>

#define STR(x) #x
#define STRINGIFY(x) STR(x)

const char *Constants::VERSION = STRINGIFY(APP_VERSION);
const int Constants::DATABASE_VERSION = 1;
const char *Constants::NAME = STRINGIFY(APP_NAME);
const char *Constants::UNIX_NAME = STRINGIFY(APP_UNIX_NAME);
const char *Constants::ORG_NAME = "Flavio Tordini";
const char *Constants::ORG_DOMAIN = "flavio.tordini.org";
const char *Constants::WEBSITE = "http://flavio.tordini.org/musique";
const char *Constants::EMAIL = "flavio.tordini@gmail.com";
const char *Constants::LASTFM_API_KEY = "e1db9fda381dea473df994bc26dfa1f1";
const char *Constants::LASTFM_SHARED_SECRET = "c768156553c34c00b75934396f11405d";
