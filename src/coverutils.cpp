#include "coverutils.h"

bool CoverUtils::isAcceptableImage(const QImage &image) {
    static const int minimumSize = 150;
    const int width = image.size().width();
    const int height = image.size().height();

    if (width < minimumSize || height < minimumSize) {
        qDebug() << "Local cover too small" << image.size();
        return false;
    }

    float aspectRatio = (float) width / (float) height;
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
        return image.scaled(maximumSize, maximumSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    return image;
}

bool CoverUtils::saveImageToLocation(const QImage &image, QString imageLocation) {
    qDebug() << "Saving scaled image to" << imageLocation;
    QFile file(imageLocation);
    QDir dir;
    dir.mkpath(QFileInfo(file).path());
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Error opening file for writing" << file.fileName();
        return false;
    }
    return image.save(&file, "JPG");
}

bool CoverUtils::coverFromFile(QString dir, QString imageLocation) {
    static QList<QRegExp> coverREs;
    if (coverREs.isEmpty()) {
        coverREs << QRegExp(".*cover.jpg", Qt::CaseInsensitive)
                 << QRegExp(".*front.jpg", Qt::CaseInsensitive)
                 << QRegExp(".*folder.jpg", Qt::CaseInsensitive);
    }

    const QFileInfoList flist = QDir(dir).entryInfoList(
                QDir::NoDotAndDotDot | QDir::Files | QDir::Readable
                );

    foreach (QFileInfo fileInfo, flist) {
        const QString filename = fileInfo.fileName();
        foreach (QRegExp re, coverREs) {
            if (filename.contains(re)) {
                qDebug() << "Found local cover" << filename;
                QImage image(fileInfo.absoluteFilePath());
                if (isAcceptableImage(image)) {
                    QImage scaledImage = maybeScaleImage(image);
                    if (image == scaledImage)
                        return QFile::copy(fileInfo.absoluteFilePath(), imageLocation);
                    else
                        return saveImageToLocation(scaledImage, imageLocation);
                }
                break;
            }
        }
    }

    return false;
}

bool CoverUtils::coverFromTags(QString filename, QString imageLocation) {
    const QString suffix = QFileInfo(filename).suffix().toLower();
    if (suffix == "mp3") {
        TagLib::MPEG::File f((TagLib::FileName)filename.toUtf8());
        if (!f.isValid()) return false;
        return coverFromMPEGTags(f.ID3v2Tag(), imageLocation);
    } else if (suffix == "ogg" || suffix == "oga") {
        TagLib::Ogg::Vorbis::File f((TagLib::FileName)filename.toUtf8());
        if (!f.isValid()) return false;
        return coverFromXiphComment(f.tag(), imageLocation);
    } else if (suffix == "flac") {
        TagLib::FLAC::File f((TagLib::FileName)filename.toUtf8());
        bool res = false;
        if (f.isValid()) res = coverFromMPEGTags(f.ID3v2Tag(), imageLocation);
        if (!res) res = coverFromXiphComment(f.xiphComment(), imageLocation);
        return res;
    } else if (suffix == "aac" ||
               suffix == "m4a" ||
               suffix == "m4b" ||
               suffix == "m4p" ||
               suffix == "mp4") {
        return coverFromMP4(filename, imageLocation);
    }
    return false;
}

bool CoverUtils::coverFromMPEGTags(TagLib::ID3v2::Tag *tag, QString imageLocation) {

    if (!tag) return false;

    TagLib::ID3v2::FrameList list = tag->frameList("APIC");
    if (list.isEmpty()) return false;

    TagLib::ID3v2::AttachedPictureFrame *frame =
            static_cast<TagLib::ID3v2::AttachedPictureFrame *>(list.front());
    if (!frame) return false;
    const int frameSize = frame->picture().size();
    if (frameSize <= 0) return false;

    QImage image;
    image.loadFromData((const uchar *) frame->picture().data(), frame->picture().size());
    if (!isAcceptableImage(image)) return false;

    QImage scaledImage = maybeScaleImage(image);
    if (image != scaledImage)
        return saveImageToLocation(scaledImage, imageLocation);

    QFile imagefile(imageLocation);
    QDir dir;
    dir.mkpath(QFileInfo(imagefile).path());
    if (!imagefile.open(QIODevice::WriteOnly)) {
        qWarning() << "Error opening file for writing" << imagefile.fileName();
        return false;
    }
    QDataStream stream(&imagefile);
    stream.writeRawData(frame->picture().data(), frameSize);

    return true;
}

bool CoverUtils::coverFromXiphComment(TagLib::Ogg::XiphComment *xiphComment, QString imageLocation) {

    if (!xiphComment) return false;

    TagLib::ByteVector byteVector = xiphComment->fieldListMap()["COVERART"].front().data(TagLib::String::Latin1);

    QByteArray encodedData;
    encodedData.setRawData(byteVector.data(), byteVector.size());

    QByteArray data = QByteArray::fromBase64(encodedData);

    QImage image;
    image.loadFromData(data);
    if (!isAcceptableImage(image)) return false;

    qDebug() << "Cover from Xiph!";

    QImage scaledImage = maybeScaleImage(image);
    if (image != scaledImage)
        return saveImageToLocation(scaledImage, imageLocation);

    QFile imagefile(imageLocation);
    QDir dir;
    dir.mkpath(QFileInfo(imagefile).path());
    if (!imagefile.open(QIODevice::WriteOnly)) {
        qWarning() << "Error opening file for writing" << imagefile.fileName();
        return false;
    }
    QDataStream stream(&imagefile);
    stream.writeRawData(data.constData(), data.size());

    return true;
}

bool CoverUtils::coverFromMP4(QString filename, QString imageLocation) {

    TagLib::MP4::File f((TagLib::FileName)filename.toUtf8());
    if (!f.isValid()) return false;

    TagLib::MP4::Tag *tag = static_cast<TagLib::MP4::Tag *>(f.tag());
    if (!tag) return false;

    TagLib::MP4::ItemListMap itemsListMap = tag->itemListMap();
    TagLib::MP4::Item coverItem = itemsListMap["covr"];
    TagLib::MP4::CoverArtList coverArtList = coverItem.toCoverArtList();
    TagLib::MP4::CoverArt coverArt = coverArtList.front();

    QImage image;
    image.loadFromData((const uchar *) coverArt.data().data(), coverArt.data().size());
    if (!isAcceptableImage(image)) return false;

    qDebug() << "Cover from MP4!";

    QImage scaledImage = maybeScaleImage(image);
    if (image != scaledImage)
        return saveImageToLocation(scaledImage, imageLocation);

    QFile imagefile(imageLocation);
    QDir dir;
    dir.mkpath(QFileInfo(imagefile).path());
    if (!imagefile.open(QIODevice::WriteOnly)) {
        qWarning() << "Error opening file for writing" << imagefile.fileName();
        return false;
    }
    QDataStream stream(&imagefile);
    stream.writeRawData(coverArt.data().data(), coverArt.data().size());

    return true;
}
