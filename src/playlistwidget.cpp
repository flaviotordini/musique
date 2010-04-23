#include "playlistwidget.h"

namespace The {
    QMap<QString, QAction*>* globalActions();
    QMap<QString, QMenu*>* globalMenus();
}

PlaylistWidget::PlaylistWidget(PlaylistView *parent)
        // : QWidget(parent)
{

    QBoxLayout *layout = new QVBoxLayout();
    layout->setMargin(0);
    layout->setSpacing(0);

    this->playlistView = parent;
    layout->addWidget(playlistView);

    /*
    QToolBar *toolBar = new QToolBar(this);
    toolBar->addAction(The::globalActions()->value("clearPlaylist"));
    layout->addWidget(toolBar);
    */


    setLayout(layout);
}
