#include "constants.h"

#define STR(x) #x
#define STRINGIFY(x) STR(x)

const char *Constants::VERSION = STRINGIFY(APP_VERSION);
const int Constants::DATABASE_VERSION = 1;
const char *Constants::APP_NAME = "Minitunes";
const char *Constants::ORG_NAME = "Flavio Tordini";
const char *Constants::ORG_DOMAIN = "flavio.tordini.org";
const char *Constants::WEBSITE = "http://flavio.tordini.org/minitunes";
const char *Constants::LASTFM_API_KEY = "e1db9fda381dea473df994bc26dfa1f1";
