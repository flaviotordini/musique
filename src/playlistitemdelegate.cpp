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
#include "datautils.h"
#include "iconutils.h"
#include "model/album.h"
#include "model/artist.h"
#include "model/track.h"
#include "playlistmodel.h"

const int PlaylistItemDelegate::PADDING = 10;
int PlaylistItemDelegate::ITEM_HEIGHT = 0;

namespace {

void drawElidedText(QPainter *painter, const QRect &textBox, const int flags, const QString &text) {
    QString elidedText =
            QFontMetrics(painter->font()).elidedText(text, Qt::ElideRight, textBox.width(), flags);
    painter->drawText(textBox, 0, elidedText);
}

} // namespace

PlaylistItemDelegate::PlaylistItemDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

QSize PlaylistItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const {
    // determine item height based on font metrics
    if (ITEM_HEIGHT == 0) {
        ITEM_HEIGHT = option.fontMetrics.height() * 1.8;
    }

    static const QSize headerSize = QSize(ITEM_HEIGHT * 2, ITEM_HEIGHT * 2);

    QModelIndex previousIndex = index.sibling(index.row() - 1, index.column());
    if (previousIndex.isValid()) {
        const TrackPointer previousTrackPointer =
                previousIndex.data(Playlist::DataObjectRole).value<TrackPointer>();
        Track *previousTrack = previousTrackPointer.data();
        if (previousTrack) {
            const TrackPointer trackPointer =
                    index.data(Playlist::DataObjectRole).value<TrackPointer>();
            Track *track = trackPointer.data();
            Album *previousAlbum = previousTrack->getAlbum();
            if (!previousAlbum && previousTrack->getArtist() != track->getArtist()) {
                return headerSize;
            }
            if (previousAlbum != track->getAlbum()) {
                return headerSize;
            }
        }
    } else {
        return headerSize;
    }

    return QSize(ITEM_HEIGHT, ITEM_HEIGHT);
}

void PlaylistItemDelegate::paint(QPainter *painter,
                                 const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const {
    paintTrack(painter, option, index);
}

QPixmap PlaylistItemDelegate::getPlayIcon(const QColor &color,
                                          const QStyleOptionViewItem &option) const {
    static QHash<QString, QPixmap> cache;
    const QString key = color.name();
    if (cache.contains(key)) return cache.value(key);
    const int iconSize = ITEM_HEIGHT / 2;
    QIcon icon = IconUtils::tintedIcon("media-playback-start", color, QSize(32, 32));
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
        QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewRow, &option, painter);

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
        // if (!isSelected) paintActiveOverlay(painter, option, line);
        static const QPixmap p = [] {
            QIcon icon = IconUtils::icon("media-playback-start");
            QPixmap p = icon.pixmap(16, 16);
            IconUtils::tint(p, qApp->palette().highlight().color());
            return p;
        }();
        painter->save();
        if (isSelected) painter->setCompositionMode(QPainter::CompositionMode_Plus);
        painter->drawPixmap(PADDING, (ITEM_HEIGHT - (p.height() / p.devicePixelRatio())) / 2, p);
        painter->restore();
    } else {
        paintTrackNumber(painter, option, line, track);
    }

    // qDebug() << "painting" << track;
    paintTrackTitle(painter, option, line, track, isActive);
    paintTrackLength(painter, option, line, track);

    painter->restore();
}

void PlaylistItemDelegate::paintAlbumHeader(QPainter *painter,
                                            const QStyleOptionViewItem &option,
                                            const QRect &line,
                                            Track *track) const {
    QString headerTitle;
    Album *album = track->getAlbum();
    if (album) headerTitle = album->getTitle();
    Artist *artist = track->getArtist();
    if (artist) {
        if (!headerTitle.isEmpty()) headerTitle += QLatin1String(" - ");
        headerTitle += artist->getName();
    }

    painter->save();

    const int h = line.height();

    const QColor &highlightColor = option.palette.highlight().color();
    const int hue = highlightColor.hue();
    const int saturation = highlightColor.saturation() * .2;
    int value = option.palette.window().color().value();
    value = value > 128 ? value * .75 : value * 2;
    const int value2 = value - 16;
    const QColor topColor = QColor::fromHsv(hue, saturation, value);
    const QColor bottomColor = QColor::fromHsv(hue, saturation, value2);
    QLinearGradient linearGradient(0, 0, 0, h);
    linearGradient.setColorAt(0.0, topColor);
    linearGradient.setColorAt(1.0, bottomColor);
    painter->fillRect(line, linearGradient);

    const qreal pixelRatio = painter->device()->devicePixelRatioF();

    if (album) {
        QPixmap p = album->getPhoto();
        if (!p.isNull()) {
            const int ph = h * pixelRatio;
            p = p.scaled(ph, ph, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            p.setDevicePixelRatio(pixelRatio);
            painter->drawPixmap(0, 0, p);
        }
    } else if (artist) {
        QPixmap p = artist->getPhoto();
        if (!p.isNull()) {
            const int ph = h * pixelRatio;
            p = p.scaled(ph, ph, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            p.setDevicePixelRatio(pixelRatio);
            painter->drawPixmap(0, 0, p);
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

        painter->setPen(Qt::white);
        painter->drawText(line.translated(-PADDING, 0), Qt::AlignRight | Qt::AlignVCenter,
    albumLength);
    }
    */

    const QFontMetrics fontMetrics = QFontMetrics(painter->font());
    const int textLeft = h + fontMetrics.height() / 2;

    // text size
    QSize trackStringSize(fontMetrics.size(Qt::TextSingleLine, headerTitle));

    int width = trackStringSize.width();
    const int maxWidth = line.width() - textLeft;
    if (width > maxWidth) width = maxWidth;

    QPoint textLoc(textLeft, 0);
    QRect trackTextBox(textLoc.x(), textLoc.y(), width, line.height());
    headerTitle = fontMetrics.elidedText(headerTitle, Qt::ElideRight, trackTextBox.width());

    // text
    painter->setPen(bottomColor.value() < 200 ? Qt::white : Qt::black);
    painter->drawText(trackTextBox, Qt::AlignLeft | Qt::AlignVCenter, headerTitle);

    painter->restore();
}

void PlaylistItemDelegate::paintTrackNumber(QPainter *painter,
                                            const QStyleOptionViewItem &option,
                                            const QRect &line,
                                            Track *track) const {
    const int trackNumber = track->getNumber();
    if (trackNumber < 1) return;

    painter->save();

    const qreal pixelRatio = painter->device()->devicePixelRatioF();

    // track number
    QFont font = painter->font();
    font.setPointSize(font.pointSize() - pixelRatio);
    painter->setFont(font);
    QString trackString = QString("%1").arg(trackNumber, 2, 10, QChar('0'));

    if (track->getDiskCount() > 1) {
        trackString = QString::number(track->getDiskNumber()) + '.' + trackString;
    }

    QRect trackTextBox(0, 0, line.height(), line.height());

    painter->setOpacity(.5);
    painter->drawText(trackTextBox, Qt::AlignCenter, trackString);
    painter->restore();
}

void PlaylistItemDelegate::paintTrackTitle(QPainter *painter,
                                           const QStyleOptionViewItem &option,
                                           const QRect &line,
                                           Track *track,
                                           bool isActive) const {
    Q_UNUSED(isActive);
    Q_UNUSED(option);

    painter->save();

    QFontMetrics fontMetrics = QFontMetrics(painter->font());

    QString trackTitle = track->getTitle();

    QSize trackStringSize(fontMetrics.size(Qt::TextSingleLine, trackTitle));
    const int textLeft = line.height() + fontMetrics.height() / 2;
    QPoint textLoc(textLeft, 0);

    int width = trackStringSize.width();
    const int maxX = line.width() - textLeft;

    QRect trackTextBox(textLoc.x(), textLoc.y(), width, line.height());
    if (trackTextBox.right() > maxX) trackTextBox.setRight(maxX);

    trackTitle = fontMetrics.elidedText(trackTitle, Qt::ElideRight, trackTextBox.width());

    painter->drawText(trackTextBox, Qt::AlignLeft | Qt::AlignVCenter, trackTitle);

    // track artist
    if (trackTextBox.right() < maxX) {
        Album *album = track->getAlbum();
        if (album && album->getArtist()) {
            Artist *albumArtist = album->getArtist();
            Artist *trackArtist = track->getArtist();
            if (albumArtist && trackArtist && albumArtist->getId() != trackArtist->getId()) {
                static const QString by = "—";
                const int x = trackTextBox.right();
                QRect textBox(x, line.height(), 0, 0);
                const int flags = Qt::AlignVCenter | Qt::AlignLeft;
                QString artistName = track->getArtist()->getName();

                painter->save();

                textBox = painter->boundingRect(
                        line.adjusted(textBox.x() + textBox.width() + PADDING, 0, 0, 0), flags, by);
                if (textBox.right() > maxX) textBox.setRight(maxX);
                drawElidedText(painter, textBox, flags, by);

                textBox = painter->boundingRect(
                        line.adjusted(textBox.x() + textBox.width() + PADDING, 0, 0, 0), flags,
                        artistName);
                if (textBox.right() > maxX) textBox.setRight(maxX);
                drawElidedText(painter, textBox, flags, artistName);
                painter->restore();
            }
        }
    }

    painter->restore();
}

void PlaylistItemDelegate::paintTrackLength(QPainter *painter,
                                            const QStyleOptionViewItem &option,
                                            const QRect &line,
                                            Track *track) const {
    const QString trackLength = DataUtils::formatDuration(track->getLength());

    // QSize trackStringSize(QFontMetrics(painter->font()).size(Qt::TextSingleLine, trackLength));
    QPoint textLoc(PADDING * 10, 0);
    QRect trackTextBox(textLoc.x(), textLoc.y(), line.width() - textLoc.x() - PADDING,
                       line.height());

    const bool isSelected = option.state & QStyle::State_Selected;
    painter->save();
    const qreal pixelRatio = painter->device()->devicePixelRatioF();
    QFont font = painter->font();
    font.setPointSize(font.pointSize() - pixelRatio);
    painter->setFont(font);
    if (isSelected)
        painter->setPen(option.palette.highlightedText().color());
    else
        painter->setOpacity(.5);
    painter->drawText(trackTextBox, Qt::AlignRight | Qt::AlignVCenter, trackLength);
    painter->restore();
}

void PlaylistItemDelegate::paintActiveOverlay(QPainter *painter,
                                              const QStyleOptionViewItem &option,
                                              const QRect &line) const {
    QColor highlightColor = option.palette.color(QPalette::Highlight);
    QColor backgroundColor = option.palette.color(QPalette::Base);
    const float animation = 0.25;
    const int gradientRange = 16;

    QColor color2 = QColor::fromHsv(highlightColor.hue(),
                                    (int)(backgroundColor.saturation() * (1.0f - animation) +
                                          highlightColor.saturation() * animation),
                                    (int)(backgroundColor.value() * (1.0f - animation) +
                                          highlightColor.value() * animation));
    QColor color1 = QColor::fromHsv(color2.hue(), qMax(color2.saturation() - gradientRange, 0),
                                    qMin(color2.value() + gradientRange, 255));

    painter->save();
    painter->setPen(Qt::NoPen);
    QLinearGradient linearGradient(0, 0, 0, line.height());
    linearGradient.setColorAt(0.0, color1);
    linearGradient.setColorAt(1.0, color2);
    painter->fillRect(line, linearGradient);
    painter->restore();
}
