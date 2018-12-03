#include "tagutils.h"

#include <tstring.h>
#include <fileref.h>
#include <tag.h>
#include <tpropertymap.h>
#include <tfilestream.h>

#include <flacfile.h>
#include <mp4file.h>
#include <mpegfile.h>
#include <apefile.h>
#include <mpcfile.h>
#include <wavpackfile.h>
#include <trueaudiofile.h>
#include <asffile.h>

#include "id3utils.h"
#include "vorbisutils.h"
#include "mp4utils.h"
#include "apeutils.h"
#include "asfutils.h"

Tags *TagUtils::load(const QString &filename) {
#ifdef Q_OS_WIN
    const wchar_t * encodedName = reinterpret_cast<const wchar_t*>(filename.utf16());
    TagLib::FileStream readOnlyStream(encodedName, true);
#else
    TagLib::FileStream readOnlyStream((TagLib::FileName)filename.toUtf8(), true);
#endif

    TagLib::FileRef fileref(&readOnlyStream);
    if (fileref.isNull()) {
        qDebug() << "Taglib cannot parse" << filename;
        return nullptr;
    }

    Tags *tags = new Tags();
    tags->setFilename(filename);

    TagLib::Tag *tag = fileref.tag();
    if (tag) {
        tags->setTitle(TagUtils::qString(tag->title()));
        tags->setArtistString(TagUtils::qString(tag->artist()));
        tags->setAlbumString(TagUtils::qString(tag->album()));
        tags->setGenre(TagUtils::qString(tag->genre()));
        tags->setTrackNumber(tag->track());
        tags->setYear(tag->year());
        tags->setComment(TagUtils::qString(tag->comment()));
        TagLib::AudioProperties *audioProperties = fileref.audioProperties();
        if (audioProperties)
            tags->setDuration(audioProperties->length());
    }

    /*
    TagLib::PropertyMap map = tag->properties();
    for (TagLib::PropertyMap::ConstIterator i = map.begin(); i != map.end(); ++i) {
        for (TagLib::StringList::ConstIterator j = i->second.begin(); j != i->second.end(); ++j) {
            const QString tagName = TagLibUtils::toQString(i->first);
            qDebug() << "PropertyMap" << tagName << TagLibUtils::toQString(*j);
            if (tagName == QLatin1String("ALBUMARTIST"))
                tags->setAlbumArtist(TagLibUtils::toQString(*j));
            else if (tagName == QLatin1String("DISCNUMBER"))
                tags->setDiskNumber(TagLibUtils::toQString(*j).toInt());
            // else if (tagName == QLatin1String("COMPOSER"))
            // tags->setComposer(toQString(*j));
            else if (tagName == QLatin1String("LYRICS"))
                tags->setLyrics(TagLibUtils::toQString(*j));
            //else if (tagName == QLatin1String("BPM"))
            //    tags->bpm = toQString(*j).toInt();
            // else qDebug() << "Unused tag" << tagName << toQString(*j);
        }
    }
    */

    // Handle file types where TagLibf:::File::tag() returns a "TagUnion"
    TagLib::File *file = fileref.file();

    if (TagLib::MPEG::File *f = dynamic_cast<TagLib::MPEG::File*>(file)) {
        if (TagLib::ID3v2::Tag *t = f->ID3v2Tag()) Id3Utils::load(t, tags);

    } else if (TagLib::TrueAudio::File *f = dynamic_cast<TagLib::TrueAudio::File*>(file)) {
        if (TagLib::ID3v2::Tag *t = f->ID3v2Tag()) Id3Utils::load(t, tags);

    } else if (TagLib::FLAC::File *f = dynamic_cast<TagLib::FLAC::File*>(file)) {
        if (TagLib::Ogg::XiphComment *t = f->xiphComment()) VorbisUtils::load(t, tags);
        else if (TagLib::ID3v2::Tag *t = f->ID3v2Tag()) Id3Utils::load(t, tags);

    } else if (TagLib::APE::File *f = dynamic_cast<TagLib::APE::File*>(file)) {
        if (TagLib::APE::Tag *t = f->APETag()) ApeUtils::load(t, tags);

    } else if (TagLib::MPC::File *f = dynamic_cast<TagLib::MPC::File*>(file)) {
        if (TagLib::APE::Tag *t = f->APETag()) ApeUtils::load(t, tags);

    } else if (TagLib::WavPack::File *f = dynamic_cast<TagLib::WavPack::File*>(file)) {
        if (TagLib::APE::Tag *t = f->APETag()) ApeUtils::load(t, tags);

    } else {

        // Fallback to casting tag() for any other file type
        TagLib::Tag *tag = file->tag();

        if (TagLib::ID3v2::Tag *t = dynamic_cast<TagLib::ID3v2::Tag*>(tag))
            Id3Utils::load(t, tags);

        else if (TagLib::Ogg::XiphComment *t = dynamic_cast<TagLib::Ogg::XiphComment*>(tag))
            VorbisUtils::load(t, tags);

        else if (TagLib::APE::Tag *t = dynamic_cast<TagLib::APE::Tag*>(tag))
            ApeUtils::load(t, tags);

        else if (TagLib::MP4::Tag *t = dynamic_cast<TagLib::MP4::Tag*>(tag))
            Mp4Utils::load(t, tags);

        else if (TagLib::ASF::Tag *t = dynamic_cast<TagLib::ASF::Tag*>(tag))
            AsfUtils::load(t, tags);
    }

    return tags;
}

QString TagUtils::qString(const TagLib::String &tstring) {
    if (tstring.isEmpty()) return QString();
    return QString::fromWCharArray(tstring.toCWString(), tstring.size());;
    // return QString::fromUtf8(tstring.toCString(true));
}

TagLib::String TagUtils::tlString(const QString &s) {
    if (s.isEmpty()) return TagLib::String();
    return TagLib::String(s.toUtf8().data(), TagLib::String::UTF8);
}

void TagUtils::parseDiskString(const QString &disk, Tags *tags) {
    const int i = disk.indexOf('/');
    int diskNumber = 1;
    int diskCount = 1;
    if (i != -1) {
        diskNumber = disk.left(i).toInt();
        diskCount = disk.mid(i+1).toInt();
    } else {
        diskNumber = disk.toInt();
    }
    if (diskNumber) tags->setDiskNumber(diskNumber);
    if (diskCount) tags->setDiskCount(diskCount);
}

void TagUtils::parseTrackString(const QString &track, Tags *tags) {
    const int i = track.indexOf('/');
    int trackNumber = 0;
    int trackCount = 0;
    if (i != -1) {
        trackNumber = track.left(i).toInt();
        trackCount = track.mid(i+1).toInt();
    } else {
        trackNumber = track.toInt();
    }
    if (trackNumber) tags->setTrackNumber(trackNumber);
    if (trackCount) tags->setTrackCount(trackCount);
}
