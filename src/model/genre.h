#ifndef GENRE_H
#define GENRE_H

#include <QtCore>
#include <QtGui>

#include "item.h"

class Track;
class Artist;

class Genre : public Item {
    Q_OBJECT

public:
    static void clearCache();
    static Genre *forId(int id);
    static Genre *maybeCreateByName(const QString &name);
    static Genre *forHash(const QString &hash);
    static int idForHash(const QString &hash);
    static QString cleanGenreName(QStringRef &genreName);

    Genre(QObject *parent = nullptr);

    QVector<Track *> getTracks();

    const QString &getHash() const { return hash; }
    void setHash(const QString &value) { hash = value; }

    QString getName() { return name; }
    void setName(const QString &value) { name = value; }

    int getTrackCount() const { return trackCount; }
    void setTrackCount(int value) { trackCount = value; }
    int getTotalTrackCount() const;

    QPixmap getThumb(int width, int height, qreal pixelRatio);

    bool hasChildren() const { return !children.isEmpty(); }
    const QVector<Genre *> &getChildren() const { return children; }
    void addChild(Genre *child) {
        child->setParent(this);
        child->setRow(children.size());
        children << child;
    }

    void setParent(Genre *value) { parent = value; }
    Genre *getParent() const { return parent; }
    // int row() { return parent ? parent->getChildren().indexOf(this) : -1; }

    int getRow() const;
    void setRow(int value);

private:
    Artist *randomArtist();

    QString hash;
    QString name;
    int trackCount;

    QPixmap pixmap;
    Artist *pixmapArtist;

    QVector<Genre *> children;
    Genre *parent;
    int row;
};

typedef QPointer<Genre> GenrePointer;
Q_DECLARE_METATYPE(GenrePointer)

#endif // GENRE_H
