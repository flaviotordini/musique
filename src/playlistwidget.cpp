#include "playlistwidget.h"
#include "droparea.h"
#include "playlistview.h"
#include "mainwindow.h"

PlaylistArea::PlaylistArea(
        PlaylistView *playlistView, DropArea *dropArea, QWidget *parent)
            : QWidget(parent) {

    QBoxLayout *layout = new QVBoxLayout();
    layout->setMargin(0);
    layout->setSpacing(0);

    layout->addWidget(playlistView);
    layout->addWidget(dropArea);

    setLayout(layout);
}

void PlaylistArea::resizeEvent(QResizeEvent *e) {
    int w = e->size().width();
    MainWindow::instance()->getStatusToolbar()->setMinimumWidth(w);
}
