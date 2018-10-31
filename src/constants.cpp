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

#include "constants.h"
#include <QtCore>

#define STR(x) #x
#define STRINGIFY(x) STR(x)

const char *Constants::VERSION = STRINGIFY(APP_VERSION);
const int Constants::DATABASE_VERSION = 3;
const char *Constants::NAME = STRINGIFY(APP_NAME);
const char *Constants::UNIX_NAME = STRINGIFY(APP_UNIX_NAME);
const char *Constants::ORG_NAME = "Flavio Tordini";
const char *Constants::ORG_DOMAIN = "flavio.tordini.org";
const char *Constants::WEBSITE = "https://flavio.tordini.org/musique";
const char *Constants::EMAIL = "flavio.tordini@gmail.com";
const char *Constants::LASTFM_API_KEY = "e1db9fda381dea473df994bc26dfa1f1";
const char *Constants::LASTFM_SHARED_SECRET = "c768156553c34c00b75934396f11405d";
