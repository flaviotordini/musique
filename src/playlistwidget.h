#ifndef PLAYLISTWIDGET_H
#define PLAYLISTWIDGET_H

#include <QtGui>
#include "playlistview.h"

class PlaylistWidget : public QWidget {

    Q_OBJECT;

public:
    PlaylistWidget(PlaylistView *parent);

private:
    PlaylistView *playlistView;

};

#endif // PLAYLISTWIDGET_H
