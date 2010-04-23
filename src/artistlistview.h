#ifndef ARTISTLISTVIEW_H
#define ARTISTLISTVIEW_H

#include <QListView>
#include "basefinderview.h"

class ArtistListView : public BaseFinderView {

    Q_OBJECT

public:
    ArtistListView(QWidget *parent);
public slots:
    void appear();

};

#endif // ARTISTLISTVIEW_H
