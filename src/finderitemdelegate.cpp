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

#include "finderitemdelegate.h"
#include "basesqlmodel.h"
#include "finderlistview.h"
#include "finderwidget.h"
#include "fontutils.h"
#include "iconutils.h"
#include "model/folder.h"
#include "model/item.h"

const int FinderItemDelegate::ITEM_WIDTH = 180;
const int FinderItemDelegate::ITEM_HEIGHT = 180;

namespace {
const int PADDING = 10;
}

FinderItemDelegate::FinderItemDelegate(FinderListView *parent)
    : QStyledItemDelegate(parent), view(parent) {}

QPixmap FinderItemDelegate::createPlayIcon(bool hovered, qreal pixelRatio) {
    const int iconHeight = 24;
    const int iconWidth = 24;
    const int padding = 4;

    QPixmap playIcon = QPixmap(iconWidth * 2 * pixelRatio, iconHeight * 2 * pixelRatio);
    playIcon.setDevicePixelRatio(pixelRatio);
    playIcon.fill(Qt::transparent);
    QPainter painter(&playIcon);
    painter.setRenderHints(QPainter::Antialiasing, true);

    QColor fillColor = Qt::black;
    QColor borderColor = Qt::white;
    if (hovered) {
        fillColor = Qt::white;
        borderColor = Qt::black;
    }

    painter.setBrush(fillColor);
    QPen pen(borderColor);
    pen.setWidth(2);
    painter.setPen(pen);
    painter.drawEllipse(QPoint(iconWidth, iconHeight), iconWidth - 2, iconHeight - 2);

    painter.translate(iconWidth / 2, iconHeight / 2);
    QPolygon polygon;
    polygon << QPoint(padding * 2, padding) << QPoint(iconWidth - padding, iconHeight / 2)
            << QPoint(padding * 2, iconHeight - padding);
    painter.setBrush(borderColor);
    pen.setColor(borderColor);
    pen.setWidth(3);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.drawPolygon(polygon);

    return playIcon;
}

QPixmap FinderItemDelegate::createMissingItemBackground(qreal pixelRatio) const {
    const int w = itemWidth * pixelRatio;
    const int h = itemHeight * pixelRatio;

    QPixmap pixmap = QPixmap(w, h);

    QPainter painter(&pixmap);
    painter.fillRect(QRect(0, 0, w, h), QBrush(QColor(0x30, 0x30, 0x30)));

    pixmap.setDevicePixelRatio(pixelRatio);
    return pixmap;
}

const QPixmap &FinderItemDelegate::getMissingItemPixmap(const QString &type,
                                                        qreal pixelRatio) const {
    static QHash<QString, QPixmap> cache;
    const QString key = QString::number(itemWidth) + type + QString::number(pixelRatio);
    auto i = cache.constFind(key);
    if (i != cache.constEnd()) return i.value();

    QPixmap pixmap = getMissingItemBackground(pixelRatio);
    pixmap.setDevicePixelRatio(pixelRatio);
    QPainter painter(&pixmap);

    QPixmap symbol = IconUtils::pixmap(":/images/item/" + type + ".png", pixelRatio);
    painter.setOpacity(.1);
    painter.drawPixmap(((itemWidth - symbol.width()) / 2) * pixelRatio,
                       ((itemHeight - symbol.height()) / 3) * pixelRatio, symbol);

    return cache.insert(key, pixmap).value();
}

const QPixmap &FinderItemDelegate::getPlayIcon(bool hovered, qreal pixelRatio) {
    static QMap<QString, QPixmap> cache;
    const QString key =
            (hovered ? QLatin1String("1|") : QLatin1String("0|")) + QString::number(pixelRatio);
    auto i = cache.constFind(key);
    if (i != cache.constEnd()) return i.value();
    QPixmap pixmap = createPlayIcon(hovered, pixelRatio);
    return cache.insert(key, pixmap).value();
}

const QPixmap &FinderItemDelegate::getMissingItemBackground(qreal pixelRatio) const {
    static QHash<QString, QPixmap> cache;
    const QString key = QString::number(itemWidth) + '|' + QString::number(pixelRatio);
    auto i = cache.constFind(key);
    if (i != cache.constEnd()) return i.value();
    QPixmap pixmap = createMissingItemBackground(pixelRatio);
    return cache.insert(key, pixmap).value();
}

QSize FinderItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const {
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(itemWidth, itemHeight);
}

void FinderItemDelegate::paint(QPainter *painter,
                               const QStyleOptionViewItem &option,
                               const QModelIndex &index) const {
    const int itemType = index.data(Finder::ItemTypeRole).toInt();
    if (itemType == Finder::ItemTypeFolder) {
        paintFolder(painter, option, index);
    } else if (itemType == Finder::ItemTypeTrack) {
        paintTrack(painter, option, index);
    } else
        paintItem(painter, option, index);
}

void FinderItemDelegate::paintFolder(QPainter *painter,
                                     const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const {
    // get the data object
    const QVariant dataObject = index.data(Finder::DataObjectRole);
    const FolderPointer folderPointer = dataObject.value<FolderPointer>();
    Folder *folder = folderPointer.data();
    if (!folder) return;

    // const bool isActive = index.data( ActiveItemRole ).toBool();
    const bool isHovered = view->isHovered(index);
    const bool isSelected = option.state & QStyle::State_Selected;

    painter->save();
    painter->translate(option.rect.topLeft());
    const QRect line(0, 0, option.rect.width(), option.rect.height());

    qreal pixelRatio = painter->device()->devicePixelRatioF();

    // thumb
    painter->drawPixmap(0, 0, getMissingItemBackground(pixelRatio));

    painter->save();
    painter->setPen(QColor(0x20, 0x20, 0x20));
    painter->drawLine(option.rect.width() - 1, 0, option.rect.width() - 1, option.rect.height());
    painter->restore();

    /*
#ifdef APP_LINUX
    static const QIcon fileIcon = IconUtils::icon("folder");
#else
    QIcon fileIcon = index.data(QFileSystemModel::FileIconRole).value<QIcon>();
#endif
    if (!fileIcon.isNull())
        painter->drawPixmap(itemWidth / 2 - 32, itemHeight / 3 - 32,
                            fileIcon.pixmap(QSize(64, 64)));
    */

    QPixmap symbol = IconUtils::pixmap(":/images/item/folder.png", pixelRatio);
    painter->save();
    painter->setOpacity(.1);
    painter->drawPixmap(((itemWidth - symbol.width()) / 2) * pixelRatio,
                        ((itemHeight - symbol.height()) / 3 - 8) * pixelRatio, symbol);
    painter->restore();

    // play icon overlayed on the thumb
    if (isHovered) {
        double animation = view->animationFrame();
        bool playIconHovered = view->isPlayIconHovered();
        paintPlayIcon(painter, line, animation, playIconHovered);
    }

    QString trackLength;
    int totalLength = folder->getTotalLength();
    if (totalLength > 3600)
        trackLength = QTime().addSecs(totalLength).toString("h:mm:ss");
    else if (totalLength > 0)
        trackLength = QTime().addSecs(totalLength).toString("m:ss");
    drawBadge(painter, trackLength, line);
    drawCentralLabel(painter, QString::number(folder->getTrackCount()), line);

    drawName(painter, option, folder->getName(), line, isSelected);

    painter->restore();
}

void FinderItemDelegate::paintTrack(QPainter *painter,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const {
    // get the data object
    const QVariant dataObject = index.data(Finder::DataObjectRole);
    const TrackPointer trackPointer = dataObject.value<TrackPointer>();
    Track *track = trackPointer.data();
    if (!track) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    // const bool isActive = index.data( ActiveItemRole ).toBool();
    const bool isHovered = view->isHovered(index);
    const bool isSelected = option.state & QStyle::State_Selected;

    painter->save();
    painter->translate(option.rect.topLeft());
    const QRect line(0, 0, option.rect.width(), option.rect.height());

    // thumb
    painter->drawPixmap(0, 0, getMissingItemBackground(painter->device()->devicePixelRatioF()));

    // play icon overlayed on the thumb
    if (isHovered) {
        double animation = view->animationFrame();
        bool playIconHovered = view->isPlayIconHovered();
        paintPlayIcon(painter, line, animation, playIconHovered);
    }

    QString trackLength;
    if (track->getLength() > 3600)
        trackLength = QTime().addSecs(track->getLength()).toString("h:mm:ss");
    else if (track->getLength() > 0)
        trackLength = QTime().addSecs(track->getLength()).toString("m:ss");
    drawBadge(painter, trackLength, line);

    int trackNumber = track->getNumber();
    if (trackNumber > 0) {
        QString trackString = QString("%1").arg(trackNumber, 2, 10, QChar('0'));
        if (track->getDiskCount() > 1) {
            trackString = QString::number(track->getDiskNumber()) + '.' + trackString;
        }
        drawCentralLabel(painter, trackString, line);
    }

    drawName(painter, option, track->getTitle(), line, isSelected);

    painter->restore();
}

void FinderItemDelegate::paintItem(QPainter *painter,
                                   const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const {
    Item *item = index.data(Finder::ItemObjectRole).value<ItemPointer>().data();
    if (!item) {
        qDebug() << "item is null";
        return;
    }

    const bool isHovered = view->isHovered(index);
    const bool isSelected = option.state & QStyle::State_Selected;

    painter->save();
    painter->translate(option.rect.topLeft());
    const QRect line(0, 0, option.rect.width(), option.rect.height());

    qreal pixelRatio = painter->device()->devicePixelRatioF();
    QPixmap pixmap = item->getThumb(itemWidth, itemHeight, pixelRatio);
    if (pixmap.isNull())
        pixmap = getMissingItemPixmap(QString(item->metaObject()->className()).toLower(),
                                      pixelRatio);
    painter->drawPixmap(0, 0, pixmap);

    if (isHovered) {
        double animation = view->animationFrame();
        bool playIconHovered = view->isPlayIconHovered();
        paintPlayIcon(painter, line, animation, playIconHovered);
    }

    drawName(painter, option, item->getName(), line, isSelected);

    painter->restore();
}

void FinderItemDelegate::paintPlayIcon(QPainter *painter,
                                       const QRect &rect,
                                       double opacity,
                                       bool hovered) const {
    const qreal pixelRatio = painter->device()->devicePixelRatioF();
    const QPixmap &playIcon = getPlayIcon(hovered, pixelRatio);

    painter->save();
    painter->translate((rect.width() - playIcon.width() - PADDING) * pixelRatio,
                       PADDING * pixelRatio);
    if (hovered)
        painter->setOpacity(opacity);
    else
        painter->setOpacity(.8);
    painter->drawPixmap(0, 0, playIcon);
    painter->restore();
}

void FinderItemDelegate::drawName(QPainter *painter,
                                  const QStyleOptionViewItem &option,
                                  const QString &name,
                                  const QRect &rect,
                                  bool isSelected) const {
    QRect nameBox = rect;
    nameBox.adjust(0, 0, 0, -itemHeight * 2 / 3);
    nameBox.translate(0, itemHeight - nameBox.height());

    painter->save();
    painter->setPen(Qt::NoPen);
    if (isSelected) {
        painter->setOpacity(.9);
        painter->setBrush(option.palette.highlight());
    } else {
        painter->setBrush(QColor(0, 0, 0, 128));
    }

    painter->drawRect(nameBox);
    painter->restore();

    painter->save();

    bool tooBig = false;
    QRect textBox = painter->boundingRect(nameBox, Qt::AlignCenter | Qt::TextWordWrap, name);
    if (textBox.height() >= nameBox.height()) {
        painter->setFont(FontUtils::small());
        textBox = painter->boundingRect(nameBox, Qt::AlignCenter | Qt::TextWordWrap, name);
        if (textBox.height() > nameBox.height()) {
            // it's still too big
            painter->setClipRect(nameBox);
            tooBig = true;
        }
    }

    if (isSelected) painter->setPen(QPen(option.palette.highlightedText(), 0));

    if (tooBig)
        painter->drawText(nameBox, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, name);
    else
        painter->drawText(textBox, Qt::AlignCenter | Qt::TextWordWrap, name);

    painter->restore();
}

void FinderItemDelegate::drawBadge(QPainter *painter,
                                   const QString &text,
                                   const QRect &rect) const {
    static const int PADDING = 4;

    painter->save();
    painter->setFont(FontUtils::small());
    QRectF textBox = painter->boundingRect(rect, Qt::AlignLeft | Qt::AlignTop, text);
    textBox.adjust(0, 0, PADDING, 0);

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 128));
    painter->drawRect(textBox);

    painter->setPen(Qt::white);
    painter->drawText(textBox, Qt::AlignCenter, text);
    painter->restore();
}

void FinderItemDelegate::drawCentralLabel(QPainter *painter,
                                          const QString &text,
                                          const QRect &rect) const {
    painter->save();
    painter->setFont(FontUtils::small());
    QSizeF textSize(QFontMetrics(painter->font()).size(Qt::TextSingleLine, text));
    QRect textBox((rect.width() - textSize.width()) / 2,
                  (rect.height() - textSize.height()) / 3 + 4, textSize.width(), textSize.height());
    QRect roundedRect = textBox;
    roundedRect.adjust(-PADDING / 2, -PADDING / 3, PADDING / 2, PADDING / 3);
    painter->setPen(QColor(255, 255, 255, 200));
    painter->drawText(textBox, Qt::AlignCenter, text);
    painter->restore();
}
