#ifndef ITEMDELEGATE_H
#define ITEMDELEGATE_H

#include <QtGui>

class Artist;
class Album;
class Folder;

class FinderItemDelegate : public QStyledItemDelegate {

    Q_OBJECT

public:
    FinderItemDelegate( QObject* parent = 0 );
    ~FinderItemDelegate();

    QSize sizeHint( const QStyleOptionViewItem&, const QModelIndex&) const;
    void paint( QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const;

private:
    QPixmap createPlayIcon(bool hovered);
    void createMissingItemBackground();
    void createMissingItemPixmap();
    void paintArtist( QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const;
    void paintAlbum( QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const;
    void paintFolder( QPainter*, const QStyleOptionViewItem&, const QModelIndex&) const;
    void paintPlayIcon(QPainter *painter, const QRect& rect, double animation = 0., bool hoverAnimation = false) const;
    void drawName(QPainter *painter, QString time, const QRect&, bool selected) const;
    void drawYear(QPainter *painter, QString year, const QRect&) const;
    QPixmap getArtistPixmap(Artist*) const;
    QPixmap getAlbumPixmap(Album*) const;
    QPixmap getFolderPixmap(Folder*) const;

    static const int ITEM_WIDTH;
    static const int ITEM_HEIGHT;
    static const int PADDING;

    QFont boldFont;
    QFont smallerFont;
    QFont smallerBoldFont;

    QPixmap playIcon;
    QPixmap hoveredPlayIcon;

    QPixmap missingItemBackground;
    QPixmap missingItemPixmap;
    QPixmap noImage;
};

#endif // ITEMDELEGATE_H
