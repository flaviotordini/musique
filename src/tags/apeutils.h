#ifndef APEUTILS
#define APEUTILS

#include "tags.h"
#include "tagutils.h"

#include <apetag.h>

namespace ApeUtils {

void load(TagLib::APE::Tag *tag, Tags *tags) {
    const TagLib::APE::ItemListMap &items = tag->itemListMap();

    if (items.contains("TRACK")) {
        QString track = TagUtils::qString(items["TRACK"].toString());
        TagUtils::parseTrackString(track, tags);
    }
    if (items.contains("DISC")) {
        QString disk = TagUtils::qString(items["DISC"].toString());
        TagUtils::parseDiskString(disk, tags);
    }

    if (items.contains("COMPOSER")) {
        QString v = TagUtils::qString(items["COMPOSER"].toString());
        tags->setComposer(v);
    }
    if (items.contains("COMPOSERSORT")) {
        QString v = TagUtils::qString(items["COMPOSERSORT"].toString());
        tags->setComposerSort(v);
    }

    if (items.contains("ALBUM ARTIST")) {
        QString v = TagUtils::qString(items["ALBUM ARTIST"].toString());
        tags->setAlbumArtist(v);
    }
    if (items.contains("ALBUMARTISTSORT")) {
        QString v = TagUtils::qString(items["ALBUMARTISTSORT"].toString());
        tags->setAlbumArtistSort(v);
    }

    if (items.contains("ARTISTSORT")) {
        QString v = TagUtils::qString(items["ARTISTSORT"].toString());
        tags->setArtistSort(v);
    }

    if (items.contains("LYRICS")) {
        QString v = TagUtils::qString(items["LYRICS"].toString());
        tags->setLyrics(v);
    }

    if (items.contains("COMPILATION")) {
        bool v = items["COMPILATION"].toString() != "0";
        tags->setCompilation(v);
    }

    // GROUPING
}

}

#endif // APEUTILS
