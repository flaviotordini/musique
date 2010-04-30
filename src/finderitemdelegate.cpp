#include "finderitemdelegate.h"
#include "basesqlmodel.h"
#include "finderwidget.h"
#include "model/artist.h"
#include "model/album.h"
#include "model/folder.h"
#include "fontutils.h"

const int FinderItemDelegate::ITEM_HEIGHT = 150;
const int FinderItemDelegate::ITEM_WIDTH = 150;
const int FinderItemDelegate::PADDING = 10;

FinderItemDelegate::FinderItemDelegate(QObject* parent) : QStyledItemDelegate(parent) { }

QPixmap FinderItemDelegate::createPlayIcon(bool hovered) const {
    static const int iconHeight = 24;
    static const int iconWidth = 24;
    static const int PADDING = 4;
    QPixmap playIcon = QPixmap(iconWidth*2, iconHeight*2);
    playIcon.fill(Qt::transparent);
    QPainter painter(&playIcon);
    painter.setRenderHints(QPainter::Antialiasing, true);

    QColor black = Qt::black;
    QColor white = Qt::white;

    if (hovered) {
        black = Qt::white;
        white = Qt::black;
    }
    painter.setBrush(black);
    QPen pen(white);
    pen.setWidth(2);
    painter.setPen(pen);
    painter.drawEllipse(QPoint(iconWidth, iconHeight), iconWidth-2, iconHeight-2);

    painter.translate(iconWidth/2, iconHeight/2);
    QPolygon polygon;
    polygon << QPoint(PADDING*2, PADDING)
            << QPoint(iconWidth - PADDING, iconHeight/2)
            << QPoint(PADDING*2, iconHeight - PADDING);
    painter.setBrush(white);
    pen.setColor(white);
    pen.setWidth(7);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.drawPolygon(polygon);

    return playIcon;
}

QPixmap FinderItemDelegate::createMissingItemBackground() const {

    QPixmap missingItemBackground = QPixmap(ITEM_WIDTH, ITEM_HEIGHT);
    QPainter painter(&missingItemBackground);

    QRadialGradient radialGrad(QPoint(ITEM_WIDTH/2, ITEM_HEIGHT/3), ITEM_WIDTH);
    radialGrad.setColorAt(0, QColor(48, 48, 48));
    radialGrad.setColorAt(1, Qt::black);
    painter.setBrush(radialGrad);
    painter.setPen(Qt::NoPen);
    painter.drawRect(QRect(0, 0, ITEM_WIDTH, ITEM_HEIGHT));

    return missingItemBackground;
}

QPixmap FinderItemDelegate::createMissingItemPixmap() const {

    QPixmap missingItemPixmap = QPixmap(getMissingItemBackground());
    QPainter painter(&missingItemPixmap);

    QPixmap missingIcon = QPixmap::fromImage(QImage(":/images/app.png"));
    painter.setOpacity(.25);
    painter.drawPixmap(0, 0, missingIcon);

    return missingItemPixmap;

}

QPixmap FinderItemDelegate::getPlayIcon(bool hovered) const {
    static QPixmap playIcon;
    static QPixmap hoveredPlayIcon;
    if (hovered) {
        if (hoveredPlayIcon.isNull()) {
            hoveredPlayIcon = createPlayIcon(true);
        }
        return hoveredPlayIcon;
    } else {
        if (playIcon.isNull()) {
            playIcon = createPlayIcon(false);
        }
        return playIcon;
    }
}

QPixmap FinderItemDelegate::getMissingItemBackground() const {
    static QPixmap missingItemBackground;
    if (missingItemBackground.isNull()) {
        missingItemBackground = createMissingItemBackground();
    }
    return missingItemBackground;
}

QPixmap FinderItemDelegate::getMissingItemPixmap() const {
    static QPixmap missingItemPixmap;
    if (missingItemPixmap.isNull()) {
        missingItemPixmap = createMissingItemPixmap();
    }
    return missingItemPixmap;
}

QSize FinderItemDelegate::sizeHint( const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/ ) const {
    return QSize(ITEM_WIDTH, ITEM_HEIGHT);
}

void FinderItemDelegate::paint( QPainter* painter,
                                const QStyleOptionViewItem& option,
                                const QModelIndex& index ) const {

    int itemType = index.data(Finder::ItemTypeRole).toInt();

    if (itemType == Finder::ItemTypeArtist) {
        paintArtist(painter, option, index);
    } else if (itemType == Finder::ItemTypeAlbum) {
        paintAlbum(painter, option, index);
    } else if (itemType == Finder::ItemTypeFolder) {
        paintFolder(painter, option, index);
        // } else if (itemType == Finder::ItemTypeTrack) {
        // paintTrack(painter, option, index);
    } else {
        QStyledItemDelegate::paint(painter, option, index);
    }

}

void FinderItemDelegate::paintArtist(QPainter* painter,
                                     const QStyleOptionViewItem& option,
                                     const QModelIndex& index) const {

    painter->save();

    painter->translate(option.rect.topLeft());
    const QRect line(0, 0, option.rect.width(), option.rect.height());

    // const bool isActive = index.data( ActiveItemRole ).toBool();
    const bool isHovered = index.data(Finder::HoveredItemRole ).toBool();
    const bool isSelected = option.state & QStyle::State_Selected;

    // get the data object
    const ArtistPointer artistPointer = index.data(Finder::DataObjectRole).value<ArtistPointer>();
    Artist *artist = artistPointer.data();

    // thumb
    QPixmap pixmap = getArtistPixmap(artist);
    if (pixmap.isNull()) pixmap = getMissingItemPixmap();
    painter->drawPixmap(0, 0, pixmap);

    // play icon overlayed on the thumb
    if (isHovered) {
        double animation = index.data(Finder::PlayIconAnimationItemRole).toDouble();
        bool playIconHovered = index.data(Finder::PlayIconHoveredRole).toBool();
        paintPlayIcon(painter, line, animation, playIconHovered);
    }

    // name
    drawName(painter, artist->getName(), line, isSelected);

    // if (artist->getTrackCount() > 0) {
    drawBadge(painter, QString::number(artist->getTrackCount()), line);
    // }

    painter->restore();

}

void FinderItemDelegate::paintAlbum(QPainter* painter,
                                    const QStyleOptionViewItem& option,
                                    const QModelIndex& index) const {

    painter->save();

    painter->translate(option.rect.topLeft());
    const QRect line(0, 0, option.rect.width(), option.rect.height());

    // const bool isActive = index.data( ActiveItemRole ).toBool();
    const bool isHovered = index.data(Finder::HoveredItemRole ).toBool();
    const bool isSelected = option.state & QStyle::State_Selected;

    // get the data object
    const AlbumPointer albumPointer = index.data(Finder::DataObjectRole).value<AlbumPointer>();
    Album *album = albumPointer.data();

    // thumb
    QPixmap pixmap = getAlbumPixmap(album);
    if (pixmap.isNull()) pixmap = getMissingItemPixmap();
    painter->drawPixmap(0, 0, pixmap);

    // play icon overlayed on the thumb
    if (isHovered) {
        double animation = index.data(Finder::PlayIconAnimationItemRole).toDouble();
        bool playIconHovered = index.data(Finder::PlayIconHoveredRole).toBool();
        paintPlayIcon(painter, line, animation, playIconHovered);
    }

    // name
    drawName(painter, album->getTitle(), line, isSelected);
    if (album->getYear() > 0) {
        drawBadge(painter, QString::number(album->getYear()), line);
    }

    painter->restore();

}

void FinderItemDelegate::paintFolder(QPainter* painter,
                                     const QStyleOptionViewItem& option,
                                     const QModelIndex& index) const {

    painter->save();

    painter->translate(option.rect.topLeft());
    const QRect line(0, 0, option.rect.width(), option.rect.height());

    // const bool isActive = index.data( ActiveItemRole ).toBool();
    const bool isHovered = index.data(Finder::HoveredItemRole ).toBool();
    const bool isSelected = option.state & QStyle::State_Selected;

    // thumb
    painter->drawPixmap(0, 0, getMissingItemBackground());
    QIcon fileIcon = index.data(QFileSystemModel::FileIconRole).value<QIcon>();
    if (!fileIcon.isNull())
        painter->drawPixmap(ITEM_WIDTH/2-32, ITEM_HEIGHT/3-32, fileIcon.pixmap(QSize(64,64)));

    // play icon overlayed on the thumb
    if (isHovered) {
        double animation = index.data(Finder::PlayIconAnimationItemRole).toDouble();
        bool playIconHovered = index.data(Finder::PlayIconHoveredRole).toBool();
        paintPlayIcon(painter, line, animation, playIconHovered);
    }

    // get the data object
    const QVariant dataObject = index.data(Finder::DataObjectRole);
    const FolderPointer folderPointer = dataObject.value<FolderPointer>();
    Folder *folder = folderPointer.data();
    Track *track = 0;
    QString name;
    if (!folder) {
        const TrackPointer trackPointer = dataObject.value<TrackPointer>();
        track = trackPointer.data();
        if (!track) {
            painter->restore();
            return;
        }
        name = track->getTitle();
    } else {
        name = folder->getName();
    }

    // name
    drawName(painter, name, line, isSelected);

    painter->restore();

}

void FinderItemDelegate::paintPlayIcon(QPainter *painter, const QRect& rect, double opacity, bool isHovered) const {
    // qDebug() << opacity << isHovered;
    static const int PADDING = 10;
    painter->save();


    QPixmap playIcon = getPlayIcon(isHovered);
    painter->translate(rect.width() - playIcon.width() - PADDING, PADDING);

    if (isHovered)
        painter->setOpacity(opacity * .75);
    else
        painter->setOpacity(.75);

    painter->drawPixmap(playIcon.rect(), playIcon);

    if (isHovered) {
        painter->setOpacity(.9 - opacity * .9);
        painter->drawPixmap(playIcon.rect(), playIcon);
    }

    painter->restore();
}

void FinderItemDelegate::drawName(QPainter *painter, QString name, const QRect& rect, bool isSelected) const {

    const QPalette palette;
    QRect textBox = rect;
    // add padding
    textBox.adjust(0, 0, 0, -ITEM_HEIGHT*2/3);
    // move to the bottom
    textBox.translate(0, ITEM_HEIGHT - textBox.height());

    painter->save();
    painter->setPen(Qt::NoPen);
    if (isSelected) {
        painter->setOpacity(.9);
        painter->setBrush(palette.highlight());
    } else {
        QColor translucentBlack(Qt::black);
        translucentBlack.setAlphaF(.5);
        painter->setBrush(translucentBlack);
    }

    painter->drawRect(textBox);
    painter->restore();

    painter->save();
    if (isSelected)
        painter->setPen(QPen(palette.highlightedText(), 0));

    painter->drawText(textBox, Qt::AlignCenter | Qt::TextWordWrap, name);
    painter->restore();
}

void FinderItemDelegate::drawBadge(QPainter *painter, QString text,  const QRect& rect) const {
    static const int PADDING = 4;

    painter->save();
    painter->setFont(FontUtils::small());
    QRectF textBox = painter->boundingRect(rect, Qt::AlignLeft | Qt::AlignTop, text);
    textBox.adjust(0, 0, PADDING, 0);

    painter->setPen(Qt::NoPen);
    painter->setBrush(Qt::black);
    painter->setOpacity(.5);
    painter->drawRect(textBox);

    painter->setOpacity(1);
    painter->setPen(Qt::white);
    painter->drawText(textBox, Qt::AlignCenter, text);
    painter->restore();
}

QPixmap FinderItemDelegate::getArtistPixmap(Artist *artist) const {
    QPixmap pixmap = artist->property("pixmap").value<QPixmap>();
    if (pixmap.isNull()) {
        // qDebug() << "Creating pixmap for" << artist;
        QImage p = artist->getPhoto();
        int xOffset = 0;
        int wDiff = p.width() - ITEM_WIDTH;
        if (wDiff > 0) xOffset = wDiff / 2;
        int yOffset = 0;
        int hDiff = p.height() - ITEM_HEIGHT;
        if (hDiff > 0) yOffset = hDiff / 4;
        pixmap = QPixmap::fromImage(p.copy(xOffset, yOffset, ITEM_WIDTH, ITEM_HEIGHT));
        artist->setProperty("pixmap", pixmap);
    }
    return pixmap;
}

QPixmap FinderItemDelegate::getFolderPixmap(Folder *folder) const {
    QPixmap pixmap = folder->property("pixmap").value<QPixmap>();
    if (pixmap.isNull()) {
        // qDebug() << "Creating pixmap for" << folder;
        QImage p = folder->getPhoto();
        int xOffset = 0;
        int wDiff = p.width() - ITEM_WIDTH;
        if (wDiff > 0) xOffset = wDiff / 2;
        int yOffset = 0;
        int hDiff = p.height() - ITEM_HEIGHT;
        if (hDiff > 0) yOffset = hDiff / 4;
        pixmap = QPixmap::fromImage(p.copy(xOffset, yOffset, ITEM_WIDTH, ITEM_HEIGHT));
        folder->setProperty("pixmap", pixmap);
    }
    return pixmap;
}

QPixmap FinderItemDelegate::getAlbumPixmap(Album *album) const {
    QPixmap pixmap = album->property("pixmap").value<QPixmap>();
    if (pixmap.isNull()) {
        // qDebug() << "Creating pixmap for" << album;
        QImage p = album->getPhoto();
        if (!p.isNull())
            p = album->getPhoto().scaled(ITEM_WIDTH, ITEM_HEIGHT);
        pixmap = QPixmap::fromImage(p);
        album->setProperty("pixmap", pixmap);
    }
    return pixmap;
}
