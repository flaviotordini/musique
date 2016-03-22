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

#ifndef COVERUTILS_H
#define COVERUTILS_H

#include <QtWidgets>

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
