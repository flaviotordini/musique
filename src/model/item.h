/* $BEGIN_LICENSE

This file is part of Musique.
Copyright 2013, Flavio Tordini <flavio.tordini@gmail.com>

Musique is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Musique is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Musique.  If not, see <http://www.gnu.org/licenses/>.

$END_LICENSE */

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
