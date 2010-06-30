#ifndef BREADCRUMBWIDGET_H
#define BREADCRUMBWIDGET_H

#include <QtGui>

class BreadcrumbWidget : public QToolBar {

    Q_OBJECT

public:
    BreadcrumbWidget(QWidget *parent);
    void addItem(QString title);
    void clear();
    void goBack();

signals:
    void goneBack();

private:
    QAction *backAction;

};

#endif // BREADCRUMBWIDGET_H
