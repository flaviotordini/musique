#include "finderitemdelegate.h"
#include "finderlistview.h"
#include "finderwidget.h"

#include "datautils.h"
#include "fontutils.h"
#include "iconutils.h"

#include "model/album.h"
#include "model/artist.h"
#include "model/folder.h"
#include "model/track.h"

const int FinderItemDelegate::PADDING = 10;

FinderItemDelegate::FinderItemDelegate(FinderListView *parent)
    : QStyledItemDelegate(parent), view(parent) {}

QSize FinderItemDelegate::sizeHint(const QStyleOptionViewItem & /*option*/,
                                   const QModelIndex & /*index*/) const {
    return gridSize;
}

void FinderItemDelegate::setItemSize(int width, int height) {
    int paddingRatio = 8;
    int padding = itemWidth / paddingRatio;
    itemWidth = width - padding * 2;
    itemHeight = height - padding * 2;
    gridSize = QSize(width, height);
}

QPixmap FinderItemDelegate::createPlayIcon(bool hovered, qreal pixelRatio) {
    const int iconHeight = 24;
    const int iconWidth = 24;
    const int padding = 4;

    QPixmap playIcon = QPixmap(iconWidth * 2 * pixelRatio, iconHeight * 2 * pixelRatio);
    playIcon.setDevicePixelRatio(pixelRatio);
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
    painter.drawEllipse(QPoint(iconWidth, iconHeight), iconWidth - 2, iconHeight - 2);

    painter.translate(iconWidth / 2, iconHeight / 2);
    QPolygon polygon;
    polygon << QPoint(padding * 2, padding) << QPoint(iconWidth - padding, iconHeight / 2)
            << QPoint(padding * 2, iconHeight - padding);
    painter.setBrush(white);
    pen.setColor(white);
    pen.setWidth(3);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.drawPolygon(polygon);

    return playIcon;
}

const QPixmap &FinderItemDelegate::getPlayIcon(bool hovered, qreal pixelRatio) {
    static QMap<QByteArray, QPixmap> cache;
    const QByteArray key = (hovered ? QByteArrayLiteral("1|") : QByteArrayLiteral("0|")) +
                           QByteArray::number(pixelRatio);
    auto i = cache.constFind(key);
    if (i != cache.constEnd()) return i.value();
    QPixmap pixmap = createPlayIcon(hovered, pixelRatio);
    return cache.insert(key, pixmap).value();
}

QRect FinderItemDelegate::paintItem(QPainter *painter,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index,
                                    Item *item) const {
    painter->save();
    QPoint offset((option.rect.width() - itemWidth) / 2, (option.rect.height() - itemHeight) / 2);
    painter->translate(option.rect.topLeft() + offset);
    const QRect line(0, 0, itemWidth, itemHeight);

    const bool isHovered = view->isHovered(index);
    // const bool isActive = index.data( ActiveItemRole ).toBool();
    const bool isSelected = option.state & QStyle::State_Selected;

    // thumb
    QPixmap pixmap = item->getThumb(itemWidth, itemHeight, painter->device()->devicePixelRatioF());

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    if (pixmap.isNull())
        painter->setBrush(option.palette.midlight());
    else
        painter->setBrush(pixmap);
    if (isSelected) {
        painter->setPen(QPen(option.palette.highlight(), 2));
    } else
        painter->setPen(Qt::NoPen);

    bool loading = item->property("loading").isValid();
    if (loading) painter->setOpacity(.5);
    QRect picRect(0, 0, itemWidth, itemWidth);
    auto artist = qobject_cast<Artist *>(item);
    if (artist)
        painter->drawEllipse(picRect.center(), picRect.width() / 2, picRect.width() / 2);
    else
        painter->drawRoundedRect(picRect, 3, 3);

    if (loading) {
        painter->setOpacity(1);
        painter->setPen(Qt::NoPen);
        drawCentralPixmap(painter, option, IconUtils::iconPixmap("content-loading", 32), line);
    }
    painter->restore();

    // play icon overlayed on the thumb
    if (isHovered) {
        double animation = view->animationFrame();
        bool playIconHovered = view->isPlayIconHovered();
        paintPlayIcon(painter, line, animation, playIconHovered);
    }

    // name
    drawName(painter, option, item->getName(), line, isSelected);

    auto album = qobject_cast<Album *>(item);
    if (album && album->getYear() > 0) {
        drawBadge(painter, QString::number(album->getYear()), line);
    }

    painter->restore();
    return line;
}

void FinderItemDelegate::paint(QPainter *painter,
                               const QStyleOptionViewItem &option,
                               const QModelIndex &index) const {
    const int itemType = index.data(Finder::ItemTypeRole).toInt();
    if (itemType == Finder::ItemTypeTrack) {
        paintTrack(painter, option, index);
    } else if (itemType == Finder::ItemTypeFolder) {
        paintFolder(painter, option, index);
    } else {
        auto item = index.data(Finder::ItemObjectRole).value<ItemPointer>();
        if (item) paintItem(painter, option, index, item);
    }
}

void FinderItemDelegate::paintTrack(QPainter *painter,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const {
    Track *track = index.data(Finder::DataObjectRole).value<TrackPointer>();

    QRect line = paintItem(painter, option, index, track);

    painter->save();
    QPoint offset((option.rect.width() - itemWidth) / 2, (option.rect.height() - itemHeight) / 2);
    painter->translate(option.rect.topLeft() + offset);

    int trackNumber = track->getNumber();
    if (trackNumber > 0) drawCentralLabel(painter, option, QString::number(trackNumber), line);

    painter->restore();
}

void FinderItemDelegate::paintFolder(QPainter *painter,
                                     const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const {
    Folder *folder = index.data(Finder::DataObjectRole).value<FolderPointer>().data();
    if (!folder) return;

    painter->save();
    QPoint offset((option.rect.width() - itemWidth) / 2, (option.rect.height() - itemHeight) / 2);
    painter->translate(option.rect.topLeft() + offset);
    const QRect line(0, 0, itemWidth, itemHeight);

    const bool isHovered = view->isHovered(index);
    const bool isSelected = option.state & QStyle::State_Selected;

    QPixmap symbol = IconUtils::icon("folder").pixmap(128);
    if (isSelected) {
        IconUtils::tint(symbol, option.palette.color(QPalette::Highlight));
    }

    painter->save();
    painter->setOpacity(.5);
    int x = (itemWidth - symbol.width()) / 2;
    int y = (itemWidth - symbol.height()) / 2;
    painter->drawPixmap(x, y, symbol);
    painter->restore();

    // play icon overlayed on the thumb
    if (isHovered) {
        double animation = view->animationFrame();
        bool playIconHovered = view->isPlayIconHovered();
        paintPlayIcon(painter, line, animation, playIconHovered);
    }

    const int seconds = folder->getTotalLength();
    if (seconds > 0) {
        QString duration = DataUtils::formatDuration(seconds);
        drawCentralLabel(painter, option, duration, line);
    }

    drawName(painter, option, folder->getName(), line, isSelected);

    painter->restore();
}

void FinderItemDelegate::paintPlayIcon(QPainter *painter,
                                       const QRect &rect,
                                       double opacity,
                                       bool isHovered) const {
    // qDebug() << opacity << isHovered;

    const qreal pixelRatio = painter->device()->devicePixelRatioF();

    const QPixmap &playIcon = getPlayIcon(isHovered, pixelRatio);

    painter->save();
    painter->translate((rect.width() - playIcon.width()) * pixelRatio, 0);

    if (isHovered)
        painter->setOpacity(opacity * .75);
    else
        painter->setOpacity(.75);

    painter->drawPixmap(0, 0, playIcon);

    if (isHovered) {
        painter->setOpacity(.9 - opacity * .9);
        painter->drawPixmap(0, 0, playIcon);
    }

    painter->restore();
}

void FinderItemDelegate::drawName(QPainter *painter,
                                  const QStyleOptionViewItem &option,
                                  const QString &name,
                                  const QRect &rect,
                                  bool isSelected) const {
    QRect nameBox = rect;
    nameBox.setHeight(itemHeight - itemWidth);
    nameBox.moveBottom(itemHeight);

    painter->save();

    bool tooBig = false;
    QRect textBox = painter->boundingRect(nameBox, Qt::AlignCenter | Qt::TextWordWrap, name);
    if (textBox.height() > nameBox.height()) {
        painter->setFont(FontUtils::small());
        textBox = painter->boundingRect(nameBox, Qt::AlignCenter | Qt::TextWordWrap, name);
        if (textBox.height() > nameBox.height()) {
            // it's still too big
            painter->setClipRect(nameBox);
            tooBig = true;
        }
    }

    if (!isSelected) painter->setOpacity(.9);

    if (tooBig)
        painter->drawText(nameBox, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, name);
    else
        painter->drawText(textBox, Qt::AlignCenter | Qt::TextWordWrap, name);

    painter->restore();
}

void FinderItemDelegate::drawBadge(QPainter *painter,
                                   const QString &text,
                                   const QRect &rect) const {
    static const int PADDING = 6;

    painter->save();
    painter->setFont(FontUtils::small());
    QRectF textBox = painter->boundingRect(rect, Qt::AlignLeft | Qt::AlignTop, text);
    textBox.adjust(0, 0, PADDING, PADDING);

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 128));
    painter->drawRect(textBox);

    painter->setPen(Qt::white);
    painter->drawText(textBox, Qt::AlignCenter, text);
    painter->restore();
}

void FinderItemDelegate::drawCentralLabel(QPainter *painter,
                                          const QStyleOptionViewItem &option,
                                          const QString &text,
                                          const QRect &rect) const {
    painter->save();
    auto square = rect;
    square.setHeight(rect.width());
    painter->setPen(option.palette.text().color());
    painter->drawText(square, Qt::AlignCenter, text);
    painter->restore();
}

void FinderItemDelegate::drawCentralPixmap(QPainter *painter,
                                           const QStyleOptionViewItem &option,
                                           const QPixmap pixmap,
                                           const QRect &rect) const {
    painter->save();

    QRect pixmapRect = pixmap.rect();

    QRect picRect = rect;
    picRect.setHeight(picRect.width());
    QRect circleRect = pixmapRect.adjusted(-PADDING, -PADDING, PADDING, PADDING);
    circleRect.moveCenter(picRect.center());
    pixmapRect.moveCenter(circleRect.center());

    painter->setRenderHints(QPainter::Antialiasing, true);
    painter->setBrush(option.palette.base());
    painter->drawEllipse(circleRect.center(), circleRect.width() / 2, circleRect.width() / 2);

    painter->setPen(option.palette.text().color());
    painter->drawPixmap(pixmapRect, pixmap);
    painter->restore();
}
