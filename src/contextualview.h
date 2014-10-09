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

#ifndef CONTEXTUALVIEW_H
#define CONTEXTUALVIEW_H

#include <QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets>
#endif
#include "view.h"

class Track;
class ArtistInfo;
class AlbumInfo;
class TrackInfo;

class ScrollingContextualView : public QWidget {

    Q_OBJECT

public:
    ScrollingContextualView(QWidget *parent);

    ArtistInfo *artistInfo;
    AlbumInfo *albumInfo;
    TrackInfo *trackInfo;

protected:
    void paintEvent(QPaintEvent *event);

};


class ContextualView : public QScrollArea, public View {

    Q_OBJECT

public:
    ContextualView(QWidget *parent);

    void appear() {}
    void disappear();

    void setTrack(Track* track);

private:
    ScrollingContextualView *scrollingContextualView;

};

#endif // CONTEXTUALVIEW_H
