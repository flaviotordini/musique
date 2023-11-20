#ifndef QUERYITEM_H
#define QUERYITEM_H

#include <QtCore>
#include <QtGui>

#include "item.h"

class Album;

class Decade : public Item {
    Q_OBJECT

public:
    Decade();

    QString getName() { return name; }
    QList<Track *> getTracks();
    QPixmap getThumb(int width, int height, qreal pixelRatio);

    void setName(const QString &value) { name = value; }
    void setStartYear(int value) { startYear = value; }

private:
    QString name;
    int startYear;

    QList<QPixmap> randomPics();
    QList<QPixmap> pics;
    QPixmap thumb;
};

typedef QPointer<Decade> DecadePointer;
Q_DECLARE_METATYPE(DecadePointer)

#endif // QUERYITEM_H
