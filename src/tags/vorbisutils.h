#ifndef VORBISUTILS
#define VORBISUTILS

#include "tags.h"
#include "tagutils.h"

#include <flacpicture.h>
#include <xiphcomment.h>

namespace VorbisUtils {

QHash<QString, const char*> initReadVorbisMap() {
    QHash<QString, const char*> map;

    // map.insert("CONTENT GROUP", "grouping");
    map.insert("ALBUMARTIST", "albumArtist");
    map.insert("ALBUM ARTIST", "albumArtist");
    map.insert("ARTISTSORT", "artistSort");
    map.insert("ALBUMARTISTSORT", "albumArtistSort");

    return map;
}

void load(TagLib::Ogg::XiphComment *tag, Tags *tags) {
    const TagLib::Ogg::FieldListMap &map = tag->fieldListMap();

    if (!map["TRACKNUMBER"].isEmpty()) {
        const QString track = TagUtils::qString(map["TRACKNUMBER"].front()).trimmed();
        TagUtils::parseTrackString(track, tags);
    }

    if (!map["TRACKTOTAL"].isEmpty()) {
        int total = TagUtils::qString(map["TRACKTOTAL"].front()).trimmed().toInt();
        tags->setTrackCount(total);
    }

    if (!map["DISCNUMBER"].isEmpty()) {
        const QString disk = TagUtils::qString(map["DISCNUMBER"].front()).trimmed();
        TagUtils::parseDiskString(disk, tags);
    }

    if (!map["DISCTOTAL"].isEmpty()) {
        int total = TagUtils::qString(map["DISCTOTAL"].front()).trimmed().toInt();
        tags->setDiskCount(total);
    }

    static const QHash<QString, const char*> oggFieldMap = initReadVorbisMap();
    const QMetaObject *metaObject = tags->metaObject();

    for (TagLib::PropertyMap::ConstIterator i = map.begin(); i != map.end(); ++i) {
        for (TagLib::StringList::ConstIterator j = i->second.begin(); j != i->second.end(); ++j) {
            const QString fieldName = TagUtils::qString(i->first);
            const QString fieldValue = TagUtils::qString(*j);
            const char* propertyName = oggFieldMap.value(fieldName);
            QByteArray byteArray;
            if (!propertyName) {
                byteArray = fieldName.toLower().toUtf8();
                propertyName = byteArray.constData();
            }
            // qDebug() << "OGG" << propertyName << fieldValue;
            // tagMap.insert(propertyName, QVariant(fieldValue));
            if (metaObject->indexOfProperty(propertyName) >= 0) {
                // qDebug() << "Setting property" << fieldName << propertyName << fieldValue;
                tags->setProperty(propertyName, fieldValue);
            } else {
                // qDebug() << "Discarding OGG field" << fieldName << propertyName << fieldValue;
            }
        }
    }
}

}

#endif // VORBISUTILS

