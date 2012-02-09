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

class CoverUtils {

public:
    static bool coverFromFile(QString dir, QString imageLocation);
    static bool coverFromTags(QString filename, QString imageLocation);

private:
    CoverUtils() {}
    static bool isAcceptableImage(const QImage &image);
    static QImage maybeScaleImage(const QImage &image);
    static bool saveImageToLocation(const QImage &image, QString imageLocation);
    static bool coverFromMPEGTags(TagLib::ID3v2::Tag *tag, QString imageLocation);
    static bool coverFromXiphComment(TagLib::Ogg::XiphComment *xiphComment, QString imageLocation);
    static bool coverFromMP4(QString filename, QString imageLocation);

};

#endif // COVERUTILS_H
