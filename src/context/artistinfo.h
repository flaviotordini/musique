#ifndef ARTISTINFO_H
#define ARTISTINFO_H

#include <QtGui>

class Artist;

class ArtistInfo : public QWidget {

    Q_OBJECT

public:
    ArtistInfo(QWidget *parent = 0);
    void setArtist(Artist *artist);
    void clear();

private:
    QLabel *titleLabel;
    QLabel *photoLabel;
    QLabel *bioLabel;

};

#endif // ARTISTINFO_H
