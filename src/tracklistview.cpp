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

#include "tracklistview.h"
#include "trackitemdelegate.h"
#include "database.h"
#include <QtSql>
#include "tracksqlmodel.h"

TrackListView::TrackListView(QWidget *parent) : QListView(parent) {
    setWindowTitle(tr("Tracks"));

    this->setItemDelegate(new TrackItemDelegate(this));
    this->setSelectionMode(QAbstractItemView::ExtendedSelection);
    this->setMouseTracking(true);

    // layout
    this->setResizeMode(QListView::Adjust);
    this->setMovement(QListView::Static);
    this->setUniformItemSizes(true);

    // colors
    QPalette p = palette();
    p.setBrush(QPalette::Base, Qt::black);
    p.setBrush(QPalette::Text, Qt::white);
    this->setPalette(p);

    // dragndrop
    this->setDragEnabled(true);
    // this->setAcceptDrops(true);
    // this->setDropIndicatorShown(true);
    this->setDragDropMode(QAbstractItemView::DragOnly);

    // cosmetics
    this->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    this->setFrameShape( QFrame::NoFrame );
    this->setAttribute(Qt::WA_MacShowFocusRect, false);

    verticalScrollBar()->setPageStep(3);
    verticalScrollBar()->setSingleStep(1);

}

void TrackListView::appear() {

}
