#ifndef ID3UTILS_H
#define ID3UTILS_H

#include "tags.h"
#include "tagutils.h"

#include <id3v2tag.h>
#include <textidentificationframe.h>
#include <unsynchronizedlyricsframe.h>
#include <attachedpictureframe.h>

namespace Id3Utils {

void SetTextFrame(const char *id, const TagLib::String &value, TagLib::ID3v2::Tag *tag) {
    TagLib::ByteVector id_vector(id);

    // Remove the frame if it already exists
    while (tag->frameListMap().contains(id_vector) &&
           tag->frameListMap()[id_vector].size() != 0) {
        tag->removeFrame(tag->frameListMap()[id_vector].front());
    }

    // Create and add a new frame
    TagLib::ID3v2::TextIdentificationFrame* frame =
            new TagLib::ID3v2::TextIdentificationFrame(id_vector,
                                                       TagLib::String::UTF8);
    frame->setText(value);
    tag->addFrame(frame);
}

void SetTextFrame(const char *id, const QString &value, TagLib::ID3v2::Tag *tag) {
    SetTextFrame(id, TagUtils::tlString(value), tag);
}

QHash<const char*, QString> initId3v2FrameMap() {
    QHash<const char*, QString> map;

    map.insert("TCOM", "composer");
    map.insert("TSOC", "composerSort");
    map.insert("TPE2", "albumArtist");
    map.insert("TSOP", "artistSort");
    map.insert("TSO2", "albumArtistSort");
    // map.insert("TBPM", "bpm");
    // map.insert("TIT1", "grouping");
    // map("TOPE", "performer");
    // map.insert("TCMP", "compilation"); // TODO bool conversion

    return map;
}

void load(TagLib::ID3v2::Tag *tag, Tags *tags) {
    const TagLib::ID3v2::FrameListMap& map = tag->frameListMap();

    if (!map["TRCK"].isEmpty()) {
        const QString track = TagUtils::qString(map["TRCK"].front()->toString()).trimmed();
        TagUtils::parseTrackString(track, tags);
    }

    if (!map["TPOS"].isEmpty()) {
        const QString disk = TagUtils::qString(map["TPOS"].front()->toString()).trimmed();
        TagUtils::parseDiskString(disk, tags);
    }

    // TODO if (!map["TBPM"].isEmpty())

    if (!map["TCOM"].isEmpty())
        tags->setComposer(TagUtils::qString(map["TCOM"].front()->toString()));
    if (!map["TSOC"].isEmpty())
        tags->setComposerSort(TagUtils::qString(map["TSOC"].front()->toString()));

    if (!map["TSOP"].isEmpty())
        tags->setArtistSort(TagUtils::qString(map["TSOP"].front()->toString()));

    if (!map["TPE2"].isEmpty())
        tags->setAlbumArtist(TagUtils::qString(map["TPE2"].front()->toString()));
    if (!map["TSO2"].isEmpty())
        tags->setAlbumArtistSort(TagUtils::qString(map["TSO2"].front()->toString()));

    // if (!map["TIT1"].isEmpty())  // content group

    if (!map["TCMP"].isEmpty())
        tags->setCompilation(TagUtils::qString(map["TCMP"].front()->toString()).toInt());

    TagLib::ID3v2::UnsynchronizedLyricsFrame* frame =
            TagLib::ID3v2::UnsynchronizedLyricsFrame::findByDescription(tag, "LYRICS");
    if (frame) {
        TagLib::String lyrics = frame->text();
        if (!lyrics.isEmpty()) tags->setLyrics(TagUtils::qString(lyrics));
    }
}

}

#endif // ID3UTILS_H
