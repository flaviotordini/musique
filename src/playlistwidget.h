#ifndef PLAYLISTWIDGET_H
#define PLAYLISTWIDGET_H

#include <QtGui>

class PlaylistView;
class DropArea;

class PlaylistWidget : public QWidget {

    Q_OBJECT;

public:
    PlaylistWidget(PlaylistView *playlistView, DropArea *dropArea, QWidget *parent);

};

#endif // PLAYLISTWIDGET_H
