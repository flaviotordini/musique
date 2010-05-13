#ifndef DROPAREA_H
#define DROPAREA_H

#include <QLabel>

QT_BEGIN_NAMESPACE
class QMimeData;
QT_END_NAMESPACE

class DropArea : public QLabel {
    Q_OBJECT

public:
    DropArea(QWidget *parent = 0);

public slots:
    void clear();

signals:
    void changed(const QMimeData *mimeData = 0);

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);

};

#endif
