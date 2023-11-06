#ifndef FINDERITEMDELEGATE_H
#define FINDERITEMDELEGATE_H

#include <QtWidgets>

#include "finderwidget.h"

class Item;
class Artist;
class Album;

class FinderItemDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    FinderItemDelegate(FinderListView *parent);
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const;
    void paint(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const;

    void setItemSize(int width, int height);
    int getItemWidth() const { return itemWidth; }
    int getItemHeight() const { return itemHeight; }

private:
    static QPixmap createPlayIcon(bool hovered, qreal pixelRatio);
    static const QPixmap &getPlayIcon(bool hovered, qreal pixelRatio);

    QRect paintItem(QPainter *painter,
                    const QStyleOptionViewItem &option,
                    const QModelIndex &index,
                    Item *item) const;
    void paintTrack(QPainter *, const QStyleOptionViewItem &, const QModelIndex &) const;
    void paintFolder(QPainter *painter,
                     const QStyleOptionViewItem &option,
                     const QModelIndex &index) const;

    void paintPlayIcon(QPainter *painter,
                       const QRect &rect,
                       double animation = 0.,
                       bool hoverAnimation = false) const;
    void drawName(QPainter *painter,
                  const QStyleOptionViewItem &option,
                  const QString &name,
                  const QRect &rect,
                  bool selected) const;
    void drawBadge(QPainter *painter, const QString &text, const QRect &) const;
    void drawCentralLabel(QPainter *painter,
                          const QStyleOptionViewItem &option,
                          const QString &text,
                          const QRect &rect) const;
    void drawCentralPixmap(QPainter *painter,
                           const QStyleOptionViewItem &option,
                           const QPixmap pixmap,
                           const QRect &rect) const;

    static const int PADDING;

    FinderListView *view;
    int itemWidth = 0;
    int itemHeight = 0;
    QSize gridSize;
};

#endif // FINDERITEMDELEGATE_H
