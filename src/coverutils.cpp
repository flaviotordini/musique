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

#include "coverutils.h"
#include "finderitemdelegate.h"
#include "model/album.h"

bool CoverUtils::isAcceptableImage(const QImage &image) {
    const int minimumSize = FinderItemDelegate::ITEM_WIDTH;
    const int width = image.size().width();
    const int height = image.size().height();

    if (width < minimumSize || height < minimumSize) {
        qDebug() << "Local cover too small" << image.size();
        return false;
    }

    float aspectRatio = (float)width / (float)height;
    if (aspectRatio > 1.2 || aspectRatio < 0.8) {
        qDebug() << "Local cover not square enough" << image.size();
        return false;
    }

    return true;
}

QImage CoverUtils::maybeScaleImage(const QImage &image) {
    static const int maximumSize = 300;
    const int width = image.size().width();
    const int height = image.size().height();
    if (width > maximumSize || height > maximumSize) {
        qDebug() << "Scaling local cover" << image.size();
        return image.scaled(maximumSize, maximumSize, Qt::KeepAspectRatio,
                            Qt::SmoothTransformation);
    }
    return image;
}

bool CoverUtils::saveImage(const QImage &image, Album *album) {
    QImage scaledImage = maybeScaleImage(image);
    QBuffer buffer;
    scaledImage.save(&buffer, "JPG");
    album->setPhoto(buffer.data());
    return true;
}

bool CoverUtils::coverFromFile(const QString &dir, Album *album) {
    static const QVector<QRegExp> coverREs = [] {
        QLatin1String ext(".(jpe?g|gif|png|bmp)");
        QVector<QRegExp> res;
        res << QRegExp(".*cover.*" + ext, Qt::CaseInsensitive)
            << QRegExp(".*front.*" + ext, Qt::CaseInsensitive)
            << QRegExp(".*folder.*" + ext, Qt::CaseInsensitive);
        return res;
    }();

    const QFileInfoList flist =
            QDir(dir).entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Readable);

    for (const QFileInfo &fileInfo : flist) {
        const QString filename = fileInfo.fileName();
        for (const QRegExp &re : coverREs) {
            if (filename.contains(re)) {
                qDebug() << "Found local cover" << filename;
                QImage image(fileInfo.absoluteFilePath());
                if (isAcceptableImage(image)) return saveImage(image, album);
                break;
            }
        }
    }

    return false;
}

bool CoverUtils::coverFromTags(const QString &filename, Album *album) {
    const QString suffix = QFileInfo(filename).suffix().toLower();
    if (suffix == "mp3") {
        TagLib::MPEG::File f((TagLib::FileName)filename.toUtf8());
        if (!f.isValid()) return false;
        return coverFromMPEGTags(f.ID3v2Tag(), album);
    } else if (suffix == "ogg" || suffix == "oga") {
        TagLib::Ogg::Vorbis::File f((TagLib::FileName)filename.toUtf8());
        if (!f.isValid()) return false;
        return coverFromXiphComment(f.tag(), album);
    } else if (suffix == "flac") {
        TagLib::FLAC::File f((TagLib::FileName)filename.toUtf8());
        bool res = false;
        if (f.isValid()) res = coverFromMPEGTags(f.ID3v2Tag(), album);
        if (!res) res = coverFromXiphComment(f.xiphComment(), album);
        return res;
    } else if (suffix == "aac" || suffix == "m4a" || suffix == "m4b" || suffix == "m4p" ||
               suffix == "mp4") {
        return coverFromMP4(filename, album);
    }
    return false;
}

bool CoverUtils::coverFromMPEGTags(TagLib::ID3v2::Tag *tag, Album *album) {
    if (!tag) return false;

    TagLib::ID3v2::FrameList list = tag->frameList("APIC");
    if (list.isEmpty()) return false;

    TagLib::ID3v2::AttachedPictureFrame *frame =
            static_cast<TagLib::ID3v2::AttachedPictureFrame *>(list.front());
    if (!frame) return false;
    const int frameSize = frame->picture().size();
    if (frameSize <= 0) return false;

    QImage image;
    image.loadFromData((const uchar *)frame->picture().data(), frame->picture().size());
    if (!isAcceptableImage(image)) return false;

    return saveImage(image, album);
}

bool CoverUtils::coverFromXiphComment(TagLib::Ogg::XiphComment *xiphComment, Album *album) {
    if (!xiphComment) return false;

    const TagLib::StringList &stringList = xiphComment->fieldListMap()["COVERART"];
    if (stringList.isEmpty()) return false;
    TagLib::ByteVector byteVector = stringList.front().data(TagLib::String::Latin1);

    QByteArray encodedData;
    encodedData.setRawData(byteVector.data(), byteVector.size());

    QByteArray data = QByteArray::fromBase64(encodedData);

    QImage image;
    image.loadFromData(data);
    if (!isAcceptableImage(image)) return false;

    qDebug() << "Cover from Xiph!";

    return saveImage(image, album);
}

bool CoverUtils::coverFromMP4(const QString &filename, Album *album) {
    TagLib::MP4::File f((TagLib::FileName)filename.toUtf8());
    if (!f.isValid()) return false;

    TagLib::MP4::Tag *tag = static_cast<TagLib::MP4::Tag *>(f.tag());
    if (!tag) return false;

    TagLib::MP4::ItemListMap itemsListMap = tag->itemListMap();
    TagLib::MP4::Item coverItem = itemsListMap["covr"];
    TagLib::MP4::CoverArtList coverArtList = coverItem.toCoverArtList();
    TagLib::MP4::CoverArt coverArt = coverArtList.front();

    QImage image;
    image.loadFromData((const uchar *)coverArt.data().data(), coverArt.data().size());
    if (!isAcceptableImage(image)) return false;

    qDebug() << "Cover from MP4!";

    return saveImage(image, album);
}
