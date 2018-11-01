#ifndef FRAMELESSWINDOW_H
#define FRAMELESSWINDOW_H

#include <QtWidgets>

class FramelessWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit FramelessWindow(QWidget *parent = nullptr);
    void setMainToolBar(QToolBar *v) { mainToolBar = v; }

public slots:
    void enableFrameless();
    void disableFrameless();

protected:
    void paintEvent(QPaintEvent *e);

private:
    bool enabled;
    QToolBar *mainToolBar;
    Qt::WindowFlags originalWindowFlags;
};

#endif // FRAMELESSWINDOW_H
