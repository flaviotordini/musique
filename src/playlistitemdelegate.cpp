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

#include "playlistitemdelegate.h"
#include "model/track.h"
#include "model/album.h"
#include "model/artist.h"
#include "playlistmodel.h"
#include "iconutils.h"
#include "datautils.h"

const int PlaylistItemDelegate::PADDING = 10;
int PlaylistItemDelegate::ITEM_HEIGHT = 0;

PlaylistItemDelegate::PlaylistItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent) {

}

QSize PlaylistItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {

    // determine item height based on font metrics
    if (ITEM_HEIGHT == 0) {
        ITEM_HEIGHT = option.fontMetrics.height() * 2;
    }

    QModelIndex previousIndex = index.sibling(index.row()-1, index.column());
    if (previousIndex.isValid()) {
        const TrackPointer previousTrackPointer = previousIndex.data(Playlist::DataObjectRole).value<TrackPointer>();
        Track *previousTrack = previousTrackPointer.data();
        if (previousTrack) {
            const TrackPointer trackPointer = index.data(Playlist::DataObjectRole).value<TrackPointer>();
            Track *track = trackPointer.data();
            if (previousTrack->getAlbum() != track->getAlbum()
                    || previousTrack->getArtist() != track->getArtist()) {
                return QSize(ITEM_HEIGHT*2, ITEM_HEIGHT*2);
            }
        }
    } else {
        return QSize(ITEM_HEIGHT*2, ITEM_HEIGHT*2);
    }

    return QSize(ITEM_HEIGHT, ITEM_HEIGHT);
}

void PlaylistItemDelegate::paint(
        QPainter* painter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index) const {
    paintTrack(painter, option, index);
}

QPixmap PlaylistItemDelegate::getPlayIcon(const QColor& color, const QStyleOptionViewItem &option) const {
    static QHash<QString, QPixmap> cache;
    const QString key = color.name();
    if (cache.contains(key)) return cache.value(key);
    const int iconSize = ITEM_HEIGHT / 2;
    QIcon icon = IconUtils::tintedIcon("media-playback-start", color, QList<QSize>() << QSize(32, 32));
    QPixmap pixmap = icon.pixmap(iconSize, iconSize);
    cache.insert(key, pixmap);
    return pixmap;
}

void PlaylistItemDelegate::paintTrack(QPainter *painter,
                                      const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const {

    Track *track = index.data(Playlist::DataObjectRole).value<TrackPointer>().data();

    const bool isActive = index.data(Playlist::ActiveItemRole).toBool();
    // const bool isHovered = index.data(Playlist::HoveredItemRole).toBool();
    const bool isSelected = option.state & QStyle::State_Selected;

    if (isSelected)
        QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter);

    painter->save();

    painter->translate(option.rect.topLeft());
    QRect line(0, 0, option.rect.width(), option.rect.height());

    // text color
    if (isSelected)
        painter->setPen(QPen(option.palette.brush(QPalette::HighlightedText), 0));
    else
        painter->setPen(QPen(option.palette.brush(QPalette::Text), 0));

    if (line.height() > ITEM_HEIGHT) {
        // qDebug() << "header at index" << index.row();
        line.setHeight(ITEM_HEIGHT);
        paintAlbumHeader(painter, option, line, track);

        // now modify our rect and painter
        // to make them similar to "headerless" items
        line.moveBottom(ITEM_HEIGHT);
        painter->translate(0, ITEM_HEIGHT);
    }

    if (isActive) {
        if (!isSelected) paintActiveOverlay(painter, option, line);
        // play icon
        QPixmap p = getPlayIcon(painter->pen().color(), option);
        painter->drawPixmap(PADDING*1.5, (ITEM_HEIGHT - p.height()) / 2, p);
    } else {
        paintTrackNumber(painter, option, line, track);
    }

    // qDebug() << "painting" << track;
    paintTrackTitle(painter, option, line, track, isActive);
    paintTrackLength(painter, option, line, track);

    // separator
    if (!isActive) {
        painter->setOpacity(.1);
        painter->drawLine(0, line.height()-1, line.width(), line.height()-1);
    }

    painter->restore();
}

void PlaylistItemDelegate::paintAlbumHeader(
        QPainter* painter, const QStyleOptionViewItem& option,
        const QRect & line, Track* track) const {

    QString headerTitle;
    Album *album = track->getAlbum();
    if (album) headerTitle = album->getTitle();
    Artist *artist = track->getArtist();
    if (artist) {
        if (!headerTitle.isEmpty()) headerTitle += " - ";
        headerTitle += artist->getName();
    }

    painter->save();

    const int h = line.height();

    static const int saturation = 24;
    static const int value = 192;
    QColor highlightColor = option.palette.color(QPalette::Highlight);
    QColor topColor = QColor::fromHsv(highlightColor.hue(), saturation, value);
    QColor bottomColor = QColor::fromHsv(topColor.hue(), saturation, value - 16);
    QLinearGradient linearGradient(0, 0, 0, h);
    linearGradient.setColorAt(0.0, topColor);
    linearGradient.setColorAt(1.0, bottomColor);
    painter->fillRect(line, linearGradient);

    // border
    QColor borderColor = QColor::fromHsv(topColor.hue(), saturation, value - 32);
    painter->setPen(borderColor);
    painter->drawLine(0, line.height()-1, line.width(), line.height()-1);

    if (album) {
        QPixmap p = album->getThumb();
        if (!p.isNull()) {
            p = p.scaled(h, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            painter->drawPixmap(0, 0, p);
            painter->drawLine(h, 0, h, h-1);
        }
    } else if (artist) {
        QPixmap p = artist->getPhoto();
        if (!p.isNull()) {
            p = p.scaled(h, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            painter->drawPixmap(0, 0, p);
            painter->drawLine(h, 0, h, h-1);
        }
    }

    // album length
    /*
    if (album) {
        // TODO this is the album duration, but not necessarily what we have in the playlist
        QString albumLength = album->formattedDuration();
        QFont normalFont = painter->font();
        normalFont.setBold(false);
        // normalFont.setPointSize(boldFont.pointSize()*.9);
        painter->setFont(normalFont);

        // text shadow
        painter->setPen(QColor(0, 0, 0, 96));
        painter->drawText(line.translated(-PADDING, -1), Qt::AlignRight | Qt::AlignVCenter, albumLength);

        painter->setPen(Qt::white);
        painter->drawText(line.translated(-PADDING, 0), Qt::AlignRight | Qt::AlignVCenter, albumLength);
    }
    */

    // font
    QFont boldFont = painter->font();
    boldFont.setBold(true);
    painter->setFont(boldFont);

    const QFontMetrics fontMetrics = QFontMetrics(boldFont);
    static const int textLeft = h + fontMetrics.height()/2;

    // text size
    QSize trackStringSize(fontMetrics.size(Qt::TextSingleLine, headerTitle));

    int width = trackStringSize.width();
    const int maxWidth = line.width() - textLeft;
    if (width > maxWidth) width = maxWidth;

    QPoint textLoc(textLeft, 0);
    QRect trackTextBox(textLoc.x(), textLoc.y(), width, line.height());
    headerTitle = fontMetrics.elidedText(headerTitle, Qt::ElideRight, trackTextBox.width());

    // text shadow
    painter->setPen(QColor(0, 0, 0, 96));
    painter->drawText(trackTextBox.translated(0, -1), Qt::AlignLeft | Qt::AlignVCenter, headerTitle);

    // text
    painter->setPen(Qt::white);
    painter->drawText(trackTextBox, Qt::AlignLeft | Qt::AlignVCenter, headerTitle);

    painter->restore();
}

void PlaylistItemDelegate::paintTrackNumber(QPainter* painter, const QStyleOptionViewItem& option,
                                            const QRect &line, Track* track) const {

    const int trackNumber = track->getNumber();
    if (trackNumber < 1) return;

    painter->save();

    // track number
    QFont boldFont = painter->font();
    boldFont.setBold(true);
    boldFont.setPointSize(boldFont.pointSize()-1);
    painter->setFont(boldFont);
    QString trackString = QString("%1").arg(trackNumber, 2, 10, QChar('0'));
    QSize trackStringSize(QFontMetrics(painter->font()).size( Qt::TextSingleLine, trackString));
    QPoint textLoc(PADDING*1.5, 0);
    QRect trackTextBox(textLoc.x(), textLoc.y(), trackStringSize.width(), line.height());

    painter->setOpacity(.5);
    painter->drawText(trackTextBox, Qt::AlignCenter, trackString);
    painter->restore();
}

void PlaylistItemDelegate::paintTrackTitle(QPainter* painter, const QStyleOptionViewItem& option,
                                           const QRect &line, Track* track, bool isActive) const {

    Q_UNUSED(isActive);
    Q_UNUSED(option);

    painter->save();

    QFontMetrics fontMetrics = QFontMetrics(painter->font());

    QString trackTitle = track->getTitle();

    QSize trackStringSize(fontMetrics.size(Qt::TextSingleLine, trackTitle));
    QPoint textLoc(PADDING*4.5, 0);

    int width = trackStringSize.width();
    const int maxWidth = line.width() - 110;
    if (width > maxWidth) width = maxWidth;

    QRect trackTextBox(textLoc.x(), textLoc.y(), width, line.height());
    trackTitle = fontMetrics.elidedText(trackTitle, Qt::ElideRight, trackTextBox.width());

    painter->drawText(trackTextBox, Qt::AlignLeft | Qt::AlignVCenter, trackTitle);
    painter->restore();
}

void PlaylistItemDelegate::paintTrackLength(QPainter* painter, const QStyleOptionViewItem& option,
                                            const QRect &line, Track* track) const {
    const QString trackLength = DataUtils::formatDuration(track->getLength());

    // QSize trackStringSize(QFontMetrics(painter->font()).size(Qt::TextSingleLine, trackLength));
    QPoint textLoc(PADDING*10, 0);
    QRect trackTextBox(textLoc.x(), textLoc.y(), line.width() - textLoc.x() - PADDING, line.height());

    painter->save();
    painter->setOpacity(.5);
    painter->drawText(trackTextBox, Qt::AlignRight | Qt::AlignVCenter, trackLength);
    painter->restore();

}

void PlaylistItemDelegate::paintActiveOverlay(
        QPainter *painter,const QStyleOptionViewItem& option, const QRect &line) const {

    QColor highlightColor = option.palette.color(QPalette::Highlight);
    QColor backgroundColor = option.palette.color(QPalette::Base);
    const float animation = 0.25;
    const int gradientRange = 16;

    QColor color2 = QColor::fromHsv(
                highlightColor.hue(),
                (int) (backgroundColor.saturation() * (1.0f - animation) + highlightColor.saturation() * animation),
                (int) (backgroundColor.value() * (1.0f - animation) + highlightColor.value() * animation)
                );
    QColor color1 = QColor::fromHsv(
                color2.hue(),
                qMax(color2.saturation() - gradientRange, 0),
                qMin(color2.value() + gradientRange, 255)
                );

    painter->save();
    painter->setPen(Qt::NoPen);
    QLinearGradient linearGradient(0, 0, 0, line.height());
    linearGradient.setColorAt(0.0, color1);
    linearGradient.setColorAt(1.0, color2);
    painter->fillRect(line, linearGradient);
    painter->restore();
}
