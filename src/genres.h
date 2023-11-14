#ifndef METAGENRES_H
#define METAGENRES_H

#include <QtCore>

class Genre;
class Item;

class Genres : public QObject {
    Q_OBJECT

public:
    Genres();
    const QList<Item *> &getItems() const { return items; }

public slots:
    void init();

signals:
    void initializing();
    void initialized();

private:
    void loadGenres();
    void loadDecades();

    QList<Item *> items;
};

#endif // METAGENRES_H
