#ifndef METAGENRES_H
#define METAGENRES_H

#include <QtCore>

class Genre;
class Item;

class Genres : public QObject {
    Q_OBJECT

public:
    Genres();
    const QVector<Item *> &getItems() const { return items; }

public slots:
    void init();

signals:
    void initialized();

private:
    void loadGenres();
    void loadDecades();

    QVector<Item *> items;
};

#endif // METAGENRES_H
