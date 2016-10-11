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

#include "playlistview.h"
#include "model/track.h"
#include "playlistmodel.h"
#include "playlistitemdelegate.h"
#include "droparea.h"
#include "globalshortcuts.h"
#include "fontutils.h"
#include "mainwindow.h"
#include "datautils.h"

PlaylistView::PlaylistView(QWidget *parent) :
        QListView(parent),
        playlistModel(0), overlayLabel(0) {

    // delegate
    setItemDelegate(new PlaylistItemDelegate(this));

    // cosmetics
    setMinimumWidth(fontInfo().pixelSize()*25);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setFrameShape(QFrame::NoFrame);
    setAttribute(Qt::WA_MacShowFocusRect, false);

    // behaviour
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    // dragndrop
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::DragDrop);

    // actions
    connect(MainWindow::instance()->getActionMap().value("remove"), SIGNAL(triggered()), SLOT(removeSelected()));
    connect(MainWindow::instance()->getActionMap().value("moveUp"), SIGNAL(triggered()), SLOT(moveUpSelected()));
    connect(MainWindow::instance()->getActionMap().value("moveDown"), SIGNAL(triggered()), SLOT(moveDownSelected()));

    // respond to the user doubleclicking a playlist item
    connect(this, SIGNAL(activated(const QModelIndex &)), SLOT(itemActivated(const QModelIndex &)));
}

void PlaylistView::setPlaylistModel(PlaylistModel *playlistModel) {

    this->playlistModel = playlistModel;
    setModel(playlistModel);

    // needed to restore the selection after dragndrop
    connect(playlistModel, SIGNAL(needSelectionFor(QList<Track*>)), SLOT(selectTracks(QList<Track*>)));

    connect(selectionModel(),
            SIGNAL(selectionChanged ( const QItemSelection & , const QItemSelection & )),
            SLOT(selectionChanged ( const QItemSelection & , const QItemSelection & )));
    connect(playlistModel, SIGNAL(layoutChanged()), this, SLOT(updatePlaylistActions()));
    connect(playlistModel, SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(updatePlaylistActions()));
    connect(playlistModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this, SLOT(updatePlaylistActions()));
    connect(playlistModel, SIGNAL(modelReset()), SLOT(updatePlaylistActions()));
    connect(MainWindow::instance()->getActionMap().value("clearPlaylist"), SIGNAL(triggered()), playlistModel, SLOT(clear()));
    connect(MainWindow::instance()->getActionMap().value("skip"), SIGNAL(triggered()), playlistModel, SLOT(skipForward()));
    connect(MainWindow::instance()->getActionMap().value("previous"), SIGNAL(triggered()), playlistModel, SLOT(skipBackward()));

    GlobalShortcuts &shortcuts = GlobalShortcuts::instance();
    connect(&shortcuts, SIGNAL(Next()), MainWindow::instance()->getActionMap().value("skip"), SLOT(trigger()));
    connect(&shortcuts, SIGNAL(Previous()), MainWindow::instance()->getActionMap().value("previous"), SLOT(trigger()));
}

void PlaylistView::itemActivated(const QModelIndex &index) {
    if (playlistModel->rowExists(index.row()))
        playlistModel->setActiveRow(index.row(), true);
}

void PlaylistView::removeSelected() {
    if (!this->selectionModel()->hasSelection()) return;
    QModelIndexList indexes = this->selectionModel()->selectedIndexes();
    playlistModel->removeIndexes(indexes);
}

void PlaylistView::selectTracks(QList<Track*> tracks) {
    selectionModel()->clear();
    foreach (Track *track, tracks) {
        QModelIndex index = playlistModel->indexForTrack(track);
        if (index.isValid())
            selectionModel()->select(index, QItemSelectionModel::Select);
        else qDebug() << __PRETTY_FUNCTION__ << "Invalid index";
    }
}

void PlaylistView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
    QListView::selectionChanged(selected, deselected);

    const bool gotSelection = this->selectionModel()->hasSelection();
    MainWindow::instance()->getActionMap().value("remove")->setEnabled(gotSelection);
    MainWindow::instance()->getActionMap().value("moveUp")->setEnabled(gotSelection);
    MainWindow::instance()->getActionMap().value("moveDown")->setEnabled(gotSelection);
}

void PlaylistView::updatePlaylistActions() {

    const int rowCount = playlistModel->rowCount(QModelIndex());
    const bool isPlaylistEmpty = rowCount < 1;
    MainWindow::instance()->getActionMap().value("clearPlaylist")->setEnabled(!isPlaylistEmpty);
    if (!isPlaylistEmpty)
        MainWindow::instance()->getActionMap().value("play")->setEnabled(true);

    // TODO also check if we're on first/last track
    MainWindow::instance()->getActionMap().value("skip")->setEnabled(rowCount > 1);
    MainWindow::instance()->getActionMap().value("previous")->setEnabled(rowCount > 1);

    if (isPlaylistEmpty) {
        setStatusTip(tr("Playlist is empty"));
    } else {
        const int totalLength = playlistModel->getTotalLength();
        QString playlistLength = DataUtils::formatDuration(totalLength);
        setStatusTip(tr("%1 tracks - Total length is %2")
                     .arg(QString::number(rowCount), playlistLength));
    }
}

void PlaylistView::moveUpSelected() {
    if (!this->selectionModel()->hasSelection()) return;
    QModelIndexList indexes = this->selectionModel()->selectedIndexes();
    playlistModel->move(indexes, true);

}

void PlaylistView::moveDownSelected() {
    if (!this->selectionModel()->hasSelection()) return;
    QModelIndexList indexes = this->selectionModel()->selectedIndexes();
    playlistModel->move(indexes, false);
}

/*
void PlaylistView::dragEnterEvent(QDragEnterEvent *event) {
    QListView::dragEnterEvent(event);

    qDebug() << "dragEnter";
    if (verticalScrollBar()->isVisible()) {
        qDebug() << "need drop area";
        // emit needDropArea();
        dropArea->show();
        willHideDropArea = false;
    }
}


void PlaylistView::dragLeaveEvent(QDragLeaveEvent *event) {
    QListView::dragLeaveEvent(event);

    qDebug() << "dragLeave";
    // emit DropArea();
    willHideDropArea = true;
    QTimer::singleShot(1000, dropArea, SLOT(hide()));
}
*/

void PlaylistView::paintEvent(QPaintEvent *event) {    
    QListView::paintEvent(event);

    if ( playlistModel->rowCount() == 0
         && !emptyMessage.isEmpty()) {

        event->accept();

        QPainter painter(this->viewport());
        QPen textPen;
        textPen.setBrush(palette().mid());
        painter.setPen(textPen);
        painter.setFont(FontUtils::big());

        QSize textSize(QFontMetrics(painter.font()).size(Qt::TextSingleLine, emptyMessage));
        QPoint centerPoint((this->width()-textSize.width())/2,
                           ((this->height()-textSize.height())/2));
        QRect centerRect(centerPoint, textSize);
        QRect boundRect;
        painter.drawText(centerRect, Qt::AlignCenter, emptyMessage, &boundRect);
    }
}

