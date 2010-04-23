#ifndef FINDERHOMEWIDGET_H
#define FINDERHOMEWIDGET_H

#include <QtGui>

class FinderHomeWidget : public QWidget {

    Q_OBJECT;

public:
    FinderHomeWidget(QWidget *parent);

protected:
    void paintEvent(QPaintEvent *);

private slots:
    void itemActivated(QListWidgetItem*);

};

#endif // FINDERHOMEWIDGET_H
