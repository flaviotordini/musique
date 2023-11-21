#ifndef APPWIDGET_H
#define APPWIDGET_H

#include <QtWidgets>

class AppWidget : public QWidget {
    Q_OBJECT

public:
    AppWidget(const QString &name, const QString &unixName, const QString &ext, QWidget *parent);
    QLabel *iconLabel;

protected:
    void enterEvent(QEnterEvent *e);
    void leaveEvent(QEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);

private:
    QPushButton *downloadButton;
    QString name;
    QString unixName;
    QString ext;
};

class AppsWidget : public QWidget {
    Q_OBJECT

public:
    AppsWidget(QWidget *parent);
    void add(QString name, QString unixName, QString ext);

protected:
    void paintEvent(QPaintEvent *e);
};

#endif // APPWIDGET_H
