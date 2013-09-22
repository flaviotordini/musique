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
#include "utils.h"

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
            if (previousTrack->getAlbum() != track->getAlbum()) {
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

QPixmap PlaylistItemDelegate::createPlayIcon() const {
    QIcon icon = Utils::icon("media-playback-start");
    return icon.pixmap(16, 16);
}

QPixmap PlaylistItemDelegate::getPlayIcon() const {
    static QPixmap playIcon;
    if (playIcon.isNull()) {
        playIcon = createPlayIcon();
    }
    return playIcon;
}

void PlaylistItemDelegate::paintTrack(QPainter *painter,
                                      const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const {

    // get the data object
    const TrackPointer trackPointer = index.data(Playlist::DataObjectRole).value<TrackPointer>();
    Track *track = trackPointer.data();

    // const PlaylistModel* playlistModel = dynamic_cast<const PlaylistModel*>(index.model());

    const bool isActive = index.data(Playlist::ActiveItemRole).toBool();
    // const bool isHovered = index.data(Playlist::HoveredItemRole).toBool();
    const bool isSelected = option.state & QStyle::State_Selected;

    if (isSelected)
        QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter);

    painter->save();

    painter->translate(option.rect.topLeft());
    QRect line(0, 0, option.rect.width(), option.rect.height());

#if defined(APP_MAC) | defined(APP_WIN)
    if (isSelected) {
        paintSelectedOverlay(painter, line);
    }
#endif

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
        QFont boldFont = painter->font();
        boldFont.setBold(true);
        painter->setFont(boldFont);
        // play icon
        painter->drawPixmap(PADDING*2, (ITEM_HEIGHT - 16) / 2, 16, 16, getPlayIcon());
    } else {
        paintTrackNumber(painter, option, line, track);
    }

    // qDebug() << "painting" << track;
    paintTrackTitle(painter, option, line, track);
    paintTrackLength(painter, option, line, track);

    // separator
    painter->setPen(option.palette.color(QPalette::Midlight));
    painter->drawLine(0, line.height()-1, line.width(), line.height()-1);

    painter->restore();
}

void PlaylistItemDelegate::paintAlbumHeader(
        QPainter* painter, const QStyleOptionViewItem& option, QRect line, Track* track) const {

    QString headerTitle;
    Album *album = track->getAlbum();
    if (album) headerTitle = album->getTitle();
    Artist *artist = track->getArtist();
    if (artist) headerTitle += " - " + artist->getName();

    painter->save();

    // cover background
    /*
    QImage p = album->getPhoto();
    if (!p.isNull()) {
        painter->drawTiledPixmap(line, QPixmap::fromImage(p));
        QLinearGradient linearGrad(0, 0, 0, line.height());
        linearGrad.setColorAt(0, QColor(0,0,0, 96));
        linearGrad.setColorAt(1, QColor(0,0,0, 64));
        painter->fillRect(line, QBrush(linearGrad));
    } else {
        QLinearGradient linearGrad(0, 0, 0, line.height());
        linearGrad.setColorAt(0, option.palette.color(QPalette::Mid));
        linearGrad.setColorAt(1, option.palette.midlight().color());
        painter->fillRect(line, QBrush(linearGrad));
    }*/

    QLinearGradient linearGrad(0, 0, 0, line.height());
#ifdef APP_MAC
    linearGrad.setColorAt(0, QColor(0x99, 0x99, 0x99, 0xFF));
    linearGrad.setColorAt(1, QColor(0xCC, 0xCC, 0xCC, 0xFF));
#else
    linearGrad.setColorAt(0, option.palette.color(QPalette::Mid));
    linearGrad.setColorAt(1, option.palette.color(QPalette::Midlight));
#endif
    painter->fillRect(line, QBrush(linearGrad));

    // borders
    // painter->setPen(option.palette.color(QPalette::Light));
    // painter->drawLine(0, 0, line.width(), 0);
    painter->setPen(option.palette.color(QPalette::Mid));
    painter->drawLine(0, line.height()-1, line.width(), line.height()-1);

    // font
    QFont boldFont = painter->font();
    boldFont.setBold(true);
    painter->setFont(boldFont);

    // text size
    QFontMetrics fontMetrics = QFontMetrics(painter->font());
    QSize trackStringSize(fontMetrics.size(Qt::TextSingleLine, headerTitle));

    int width = trackStringSize.width();
    const int maxWidth = line.width() - 80;
    if (width > maxWidth) width = maxWidth;

    QPoint textLoc(15, 0);
    QRect trackTextBox(textLoc.x(), textLoc.y(), width, line.height());
    headerTitle = fontMetrics.elidedText(headerTitle, Qt::ElideRight, trackTextBox.width());

    // text shadow
    painter->setPen(QColor(0, 0, 0, 64));
    painter->drawText(trackTextBox.translated(0, -1), Qt::AlignLeft | Qt::AlignVCenter, headerTitle);

    // text
    painter->setPen(option.palette.color(QPalette::Light));
    painter->drawText(trackTextBox, Qt::AlignLeft | Qt::AlignVCenter, headerTitle);
    
    // album length
    if (album) {
        // TODO this is the album duration, but not necessarily what we have in the playlist
        QString albumLength = album->formattedDuration();
        QFont normalFont = painter->font();
        normalFont.setBold(false);
        // normalFont.setPointSize(boldFont.pointSize()*.9);
        painter->setFont(normalFont);

        // text shadow
        painter->setPen(QColor(0, 0, 0, 64));
        painter->drawText(line.translated(-PADDING, -1), Qt::AlignRight | Qt::AlignVCenter, albumLength);

        painter->setPen(option.palette.color(QPalette::Light));
        painter->drawText(line.translated(-PADDING, 0), Qt::AlignRight | Qt::AlignVCenter, albumLength);
    }

    // TODO album year

    painter->restore();
}

void PlaylistItemDelegate::paintTrackNumber(
        QPainter* painter, const QStyleOptionViewItem& option, QRect line, Track* track) const {

    const int trackNumber = track->getNumber();
    if (trackNumber < 1) return;

    painter->save();

    // track number
    QFont boldFont = painter->font();
    boldFont.setBold(true);
    boldFont.setPointSize(boldFont.pointSize()*.9);
    painter->setFont(boldFont);
    QString trackString = QString("%1").arg(trackNumber, 2, 10, QChar('0'));
    QSize trackStringSize(QFontMetrics(painter->font()).size( Qt::TextSingleLine, trackString));
    QPoint textLoc(PADDING*2, 0);
    QRect trackTextBox(textLoc.x(), textLoc.y(), trackStringSize.width(), line.height());

    // rounded rect
    QRect trackRoundedRect = trackTextBox;
    trackRoundedRect.setY((line.height() - trackStringSize.height()) / 2);
    trackRoundedRect.setHeight(trackStringSize.height());
    trackRoundedRect.adjust(-PADDING/2, -PADDING/3, PADDING/2, PADDING/3);
    painter->setOpacity(.5);
    painter->setRenderHints(QPainter::Antialiasing, true);
    painter->setBrush(option.palette.window());
    painter->setPen(option.palette.WindowText);

    painter->drawRoundedRect(trackRoundedRect, PADDING/2, PADDING/2, Qt::AbsoluteSize);
    painter->drawText(trackTextBox, Qt::AlignCenter, trackString);

    painter->restore();

}

void PlaylistItemDelegate::paintTrackTitle(
        QPainter* painter, const QStyleOptionViewItem& option, QRect line, Track* track) const {

    QFontMetrics fontMetrics = QFontMetrics(painter->font());

    QString trackTitle = track->getTitle();

    QSize trackStringSize(fontMetrics.size(Qt::TextSingleLine, trackTitle));
    QPoint textLoc(PADDING*6, 0);

    int width = trackStringSize.width();
    const int maxWidth = line.width() - 110;
    if (width > maxWidth) width = maxWidth;

    QRect trackTextBox(textLoc.x(), textLoc.y(), width, line.height());
    trackTitle = fontMetrics.elidedText(trackTitle, Qt::ElideRight, trackTextBox.width());

    painter->drawText(trackTextBox, Qt::AlignLeft | Qt::AlignVCenter, trackTitle);
}

void PlaylistItemDelegate::paintTrackLength(
        QPainter* painter, const QStyleOptionViewItem& option, QRect line, Track* track) const {

    QString trackLength;
    if (track->getLength() > 3600)
        trackLength =  QTime().addSecs(track->getLength()).toString("h:mm:ss");
    else if (track->getLength() > 0)
        trackLength = QTime().addSecs(track->getLength()).toString("m:ss");

    // QSize trackStringSize(QFontMetrics(painter->font()).size(Qt::TextSingleLine, trackLength));
    QPoint textLoc(PADDING*10, 0);
    QRect trackTextBox(textLoc.x(), textLoc.y(), line.width() - textLoc.x() - PADDING, line.height());

    const bool isSelected = option.state & QStyle::State_Selected;
    painter->save();
    if (!isSelected) painter->setPen(option.palette.color(QPalette::Mid));
    painter->drawText(trackTextBox, Qt::AlignRight | Qt::AlignVCenter, trackLength);
    painter->restore();

}

void PlaylistItemDelegate::paintActiveOverlay(
        QPainter *painter, const QStyleOptionViewItem& option, QRect line) const {

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

void PlaylistItemDelegate::paintSelectedOverlay( QPainter *painter, QRect line) const {
    QColor color1 = QColor::fromRgb(0x69, 0xa6, 0xd9);
    QColor color2 = QColor::fromRgb(0x14, 0x6b, 0xd4);
    painter->save();
    painter->setPen(Qt::NoPen);
    QLinearGradient linearGradient(0, 0, 0, line.height());
    linearGradient.setColorAt(0.0, color1);
    linearGradient.setColorAt(1.0, color2);
    painter->setBrush(linearGradient);
    painter->drawRect(line);
    painter->restore();
}
