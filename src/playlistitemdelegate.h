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

#ifndef PLAYLISTITEMDELEGATE_H
#define PLAYLISTITEMDELEGATE_H

#include <QtWidgets>

class Track;

class PlaylistItemDelegate : public QStyledItemDelegate {

    Q_OBJECT

public:
    PlaylistItemDelegate(QObject *parent = 0);
    QSize sizeHint( const QStyleOptionViewItem&, const QModelIndex&) const;
    void paint( QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const;

private:
    QPixmap getPlayIcon(const QColor &color, const QStyleOptionViewItem &option) const;
    void paintTrack(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paintAlbumHeader(QPainter* painter, const QStyleOptionViewItem& option,
                          const QRect &line, Track* track) const;
    void paintTrackNumber(QPainter* painter, const QStyleOptionViewItem& option,
                          const QRect &line, Track* track) const;
    void paintTrackTitle(QPainter* painter, const QStyleOptionViewItem& option,
                         const QRect &line, Track* track, bool isActive) const;
    void paintTrackLength(QPainter* painter, const QStyleOptionViewItem& option,
                          const QRect &line, Track* track) const;
    void paintActiveOverlay(QPainter *painter, const QStyleOptionViewItem& option,
                            const QRect &line) const;

    static const int PADDING;
    static int ITEM_HEIGHT;

};

#endif // PLAYLISTITEMDELEGATE_H
