#ifndef MP4UTILS
#define MP4UTILS

#include "tags.h"
#include "tagutils.h"

#include <mp4tag.h>

namespace Mp4Utils {

void load(TagLib::MP4::Tag *tag, Tags *tags) {
    const TagLib::MP4::ItemListMap &map = tag->itemListMap();

    if (map.contains("trkn")) {
        TagLib::MP4::Item::IntPair intPair = map["trkn"].toIntPair();
        tags->setTrackNumber(intPair.first);
        tags->setTrackCount(intPair.second);
    }
    if (map.contains("disk")) {
        TagLib::MP4::Item::IntPair intPair = map["disk"].toIntPair();
        tags->setDiskNumber(intPair.first);
        tags->setDiskCount(intPair.second);
    }

    if (map.contains("\251wrt")) {
        QString v = TagUtils::qString(map["\251wrt"].toStringList().toString(", "));
        tags->setComposer(v);
    }
    if (map.contains("soco")) {
        QString v = TagUtils::qString(map["soco"].toStringList().toString(", "));
        tags->setComposerSort(v);
    }

    TagLib::MP4::ItemListMap::ConstIterator it = map.find("aART");
    if (it != map.end()) {
        TagLib::StringList sl = it->second.toStringList();
        if (!sl.isEmpty())
            tags->setAlbumArtist(TagUtils::qString(sl.front()));
    }
    if (map.contains("soaa")) {
        TagLib::StringList sl = map["soaa"].toStringList();
        if (!sl.isEmpty())
            tags->setAlbumArtistSort(TagUtils::qString(sl.front()));
    }

    if (map.contains("soar")) {
        TagLib::StringList sl = map["soar"].toStringList();
        if (!sl.isEmpty())
            tags->setArtistSort(TagUtils::qString(sl.front()));
    }

    if (map.contains("©lyr")) {
        TagLib::StringList sl = map["©lyr"].toStringList();
        if (!sl.isEmpty())
            tags->setLyrics(TagUtils::qString(sl.front()));
    }

    if (map.contains("cpil")) {
        bool v = map["cpil"].toBool();
        tags->setCompilation(v);
    }

    /*
    if (items.contains("\251grp")) {
        Decode(items["\251grp"].toStringList().toString(" "), nullptr,
                song->mutable_grouping());
    }
    */
}

}

#endif // MP4UTILS
