#ifndef TAGUTILS
#define TAGUTILS

#include <tstring.h>
#include <QtCore>
#include "tags.h"

namespace TagUtils {

Tags* load(const QString &filename);
QString qString(const TagLib::String &tstring);
TagLib::String tlString(const QString &s);
void parseDiskString(const QString &disk, Tags *tags);
void parseTrackString(const QString &track, Tags *tags);

}

#endif // TAGUTILS
