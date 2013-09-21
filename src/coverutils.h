#ifndef COVERUTILS_H
#define COVERUTILS_H

#include <QtGui>

// TagLib
#include <id3v2tag.h>
#include <mpegfile.h>
#include <attachedpictureframe.h>
#include <oggfile.h>
#include <vorbisfile.h>
#include <flacfile.h>
#include <oggflacfile.h>
#include <mp4file.h>

class Album;

class CoverUtils {

public:
    static bool coverFromFile(QString dir, Album *album);
    static bool coverFromTags(QString filename, Album *album);

private:
    CoverUtils() {}
    static bool isAcceptableImage(const QImage &image);
    static QImage maybeScaleImage(const QImage &image);
    static bool saveImage(const QImage &image, Album *album);
    static bool coverFromMPEGTags(TagLib::ID3v2::Tag *tag, Album *album);
    static bool coverFromXiphComment(TagLib::Ogg::XiphComment *xiphComment, Album *album);
    static bool coverFromMP4(QString filename, Album *album);

};

#endif // COVERUTILS_H
