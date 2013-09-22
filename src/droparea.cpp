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

#include <QtGui>
#include "droparea.h"
#include "utils.h"
#include "trackmimedata.h"
#include "model/track.h"
#include "playlistmodel.h"

DropArea::DropArea(QWidget *parent) : QLabel(parent) {
    setMargin(15);
    setAlignment(Qt::AlignCenter);
    setAcceptDrops(true);
    setAutoFillBackground(true);
    clear();
}

void DropArea::dragEnterEvent(QDragEnterEvent *event) {
    setBackgroundRole(QPalette::Highlight);
    setForegroundRole(QPalette::HighlightedText);

    event->acceptProposedAction();
    emit changed(event->mimeData());
}

void DropArea::dragMoveEvent(QDragMoveEvent *event) {
    event->acceptProposedAction();
}

void DropArea::dropEvent(QDropEvent *event) {
    const QMimeData *mimeData = event->mimeData();
    const TrackMimeData* trackMimeData = dynamic_cast<const TrackMimeData*>(mimeData);
    if (trackMimeData) {
        QList<Track*> tracks = trackMimeData->tracks();
        playlistModel->addTracks(tracks);
        event->acceptProposedAction();
    }
}

void DropArea::dragLeaveEvent(QDragLeaveEvent *event) {
    clear();
    event->accept();
}

void DropArea::clear() {
    setText("<b>" + tr("Drop here to append to the playlist") + "</b>");
    setPixmap(Utils::icon("list-add").pixmap(24, 24));
    setBackgroundRole(QPalette::Base);
    setForegroundRole(QPalette::Text);

    emit changed();
}
