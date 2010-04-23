#ifndef ARTISTINFO_H
#define ARTISTINFO_H

#include <QtGui>

class Artist;

class ArtistInfo : public QWidget {

    Q_OBJECT

public:
    ArtistInfo(QWidget *parent = 0);
    void setArtist(Artist *artist);

private:
    QLabel *artistLabel;
    QLabel *artistPhoto;
    QLabel *artistBio;
    QLabel *artistBioMore;

};

#endif // ARTISTINFO_H
