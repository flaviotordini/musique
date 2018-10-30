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

#ifndef ITEMDELEGATE_H
#define ITEMDELEGATE_H

#include <QtWidgets>

class Artist;
class Album;
class Folder;

class FinderItemDelegate : public QStyledItemDelegate {

    Q_OBJECT

public:
    FinderItemDelegate( QObject* parent = 0 );
    QSize sizeHint( const QStyleOptionViewItem&, const QModelIndex&) const;
    void paint( QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const;
    static const int ITEM_WIDTH;
    static const int ITEM_HEIGHT;

private:
    static QPixmap createPlayIcon(bool hovered, qreal pixelRatio);
    static QPixmap createMissingItemBackground(qreal pixelRatio);
    static const QPixmap &getMissingItemPixmap(const QString &type);
    static const QPixmap &getPlayIcon(bool hovered);
    static const QPixmap &getMissingItemBackground(qreal pixelRatio);
    static const QPixmap &getMissingArtistPixmap();
    static const QPixmap &getMissingAlbumPixmap();
    static const QPixmap &getMissingTrackPixmap();
    void paintArtist( QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const;
    void paintAlbum( QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const;
    void paintFolder( QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const;
    void paintTrack( QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const;
    void paintPlayIcon(QPainter *painter, const QRect& rect, double animation = 0., bool hoverAnimation = false) const;
    void drawName(QPainter *painter, const QStyleOptionViewItem &option, const QString& time, const QRect&, bool selected) const;
    void drawBadge(QPainter *painter, const QString &text, const QRect&) const;
    void drawCentralLabel(QPainter *painter, const QString &text, const QRect&) const;

    static const int PADDING;

};

#endif // ITEMDELEGATE_H
