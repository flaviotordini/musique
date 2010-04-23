#ifndef BREADCRUMBWIDGET_H
#define BREADCRUMBWIDGET_H

#include <QtGui>

class BreadcrumbWidget : public QToolBar {

    Q_OBJECT

public:
    BreadcrumbWidget(QWidget *parent);
    void addWidget(QWidget *widget);
    void removeWidget(QWidget *widget);
    void clear();

signals:
    void goBack();

protected:
    // void paintEvent(QPaintEvent *event);

private:
    QHash<QWidget*, QLabel*> widgetLabels;

};

#endif // BREADCRUMBWIDGET_H
