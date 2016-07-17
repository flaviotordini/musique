#ifndef ASFUTILS
#define ASFUTILS

#include "tags.h"
#include "tagutils.h"

#include <asftag.h>
#include <asfattribute.h>

namespace AsfUtils {

QString string(const TagLib::ASF::AttributeListMap &map, const TagLib::String &key) {
    if (map.contains(key)) {
        const TagLib::ASF::AttributeList &al = map[key];
        if (!al.isEmpty()) {
            return TagUtils::qString(al.front().toString());
        }
    }
    return QString();
}

void load(TagLib::ASF::Tag *tag, Tags *tags) {
    const TagLib::ASF::AttributeListMap &map = tag->attributeListMap();

    if (map.contains("WM/TrackNumber")) {
        const TagLib::ASF::AttributeList &al = map["WM/TrackNumber"];
        if (!al.isEmpty()) {
            const TagLib::ASF::Attribute &a = al.front();
            if (a.type() == TagLib::ASF::Attribute::UnicodeType) {
                QString track = TagUtils::qString(a.toString());
                TagUtils::parseTrackString(track, tags);
            }
        }
    }

    if (map.contains("WM/PartOfSet")) {
        const TagLib::ASF::AttributeList &al = map["WM/PartOfSet"];
        if (!al.isEmpty()) {
            const TagLib::ASF::Attribute &a = al.front();
            if (a.type() == TagLib::ASF::Attribute::UnicodeType) {
                QString disk = TagUtils::qString(a.toString());
                TagUtils::parseDiskString(disk, tags);
            } else {
                int diskNumber = a.toUInt();
                if (diskNumber > 0) tags->setDiskNumber(diskNumber);
            }
        }
    }

    tags->setComposer(string(map, "WM/Composer"));
    tags->setComposerSort(string(map, "WM/ComposerSortOrder"));

    tags->setAlbumArtist(string(map, "WM/AlbumArtist"));
    tags->setAlbumArtistSort(string(map, "WM/AlbumArtistSortOrder"));

    tags->setArtistSort(string(map, "WM/ArtistSortOrder"));

    tags->setLyrics(string(map, "WM/Lyrics"));

    if (map.contains("WM/IsCompilation")) {
        const TagLib::ASF::AttributeList &al = map["WM/IsCompilation"];
        if (!al.isEmpty()) {
            bool v = al.front().toBool();
            tags->setCompilation(v);
        }
    }

    // WM/ContentGroupDescription
}

}

#endif // ASFUTILS
