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

#include <QListView>

class BaseFinderView : public QListView {

    Q_OBJECT

public:
    BaseFinderView(QWidget *parent);

public slots:
    void appear();
    void disappear();

signals:
    void play(const QModelIndex &index);

protected:
    void leaveEvent(QEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private:
    bool isHoveringPlayIcon(QMouseEvent *event);

};

#endif // BASEFINDERVIEW_H
