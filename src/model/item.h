#ifndef ITEM_H
#define ITEM_H

#include <QtCore>

class Track;

class Item : public QObject {

    Q_OBJECT

public:
    Item(QObject *parent = 0) : QObject(parent), id(0) { }
    int getId() const { return id; }
    void setId(int id) { this->id = id; }
    virtual QString getName() = 0;
    virtual QList<Track*> getTracks() = 0;

protected:
    int id;

};

// This is required in order to use QPointer<Item> as a QVariant
typedef QPointer<Item> ItemPointer;
Q_DECLARE_METATYPE(ItemPointer)

#endif // ITEM_H
