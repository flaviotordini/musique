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

#ifndef BASEFINDERVIEW_H
#define BASEFINDERVIEW_H

#include <QtWidgets>

class FinderItemDelegate;

class FinderListView : public QListView {
    Q_OBJECT

public:
    FinderListView(QWidget *parent);

    int isHovered(const QModelIndex &index) const { return hoveredRow == index.row(); }
    bool isPlayIconHovered() const { return playIconHovered; }
    double animationFrame() const { return timeLine->currentFrame() / 100.; }

public slots:
    void appear();
    void disappear();

    void setHoveredIndex(const QModelIndex &index);
    void setHoveredRow(int row);
    void clearHover();
    void enterPlayIconHover();
    void exitPlayIconHover();
    void refreshIndex(const QModelIndex &index);
    void refreshRow(int row);
    void updatePlayIcon();

signals:
    void play(const QModelIndex &index);

protected:
    void leaveEvent(QEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *event);

    FinderItemDelegate *delegate;

private:
    bool isHoveringPlayIcon(QMouseEvent *event);

    int hoveredRow;
    QTimeLine *timeLine;
    bool playIconHovered;
};

#endif // BASEFINDERVIEW_H
