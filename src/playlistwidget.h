#ifndef PLAYLISTWIDGET_H
#define PLAYLISTWIDGET_H

#include <QtGui>

class PlaylistView;
class DropArea;

class PlaylistArea : public QWidget {

    Q_OBJECT;

public:
    PlaylistArea(PlaylistView *playlistView, DropArea *dropArea, QWidget *parent);

};

#endif // PLAYLISTWIDGET_H
