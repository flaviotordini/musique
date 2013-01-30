#ifndef TRACKLISTVIEW_H
#define TRACKLISTVIEW_H

#include <QListView>
#include "basefinderview.h"

class TrackListView : public QListView {

    Q_OBJECT

public:
    TrackListView(QWidget *parent);
public slots:
    void appear();
    void disappear() { }

};

#endif // TRACKLISTVIEW_H
