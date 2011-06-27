#ifndef SEARCHVIEW_H
#define SEARCHVIEW_H

#include <QtGui>
#include "basefinderview.h"

class SearchView : public BaseFinderView {

    Q_OBJECT

public:
    SearchView(QWidget *parent);
    void search(QString query);

protected:
    void paintEvent(QPaintEvent *event);

private:
    QLabel *label;

};

#endif // SEARCHVIEW_H
