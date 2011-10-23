#ifndef BASEFINDERVIEW_H
#define BASEFINDERVIEW_H

#include <QListView>

class BaseFinderView : public QListView {

    Q_OBJECT

public:
    BaseFinderView(QWidget *parent);

public slots:
    void appear();
    void disappear();

signals:
    void play(const QModelIndex &index);

protected:
    void leaveEvent(QEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

    bool playIconHovered;

private:
    bool isHoveringPlayIcon(QMouseEvent *event);

};

#endif // BASEFINDERVIEW_H
