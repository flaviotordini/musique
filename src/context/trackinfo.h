#ifndef TRACKINFO_H
#define TRACKINFO_H

#include <QtGui>

class Track;

class TrackInfo : public QWidget {

    Q_OBJECT

public:
    TrackInfo(QWidget *parent = 0);
    void setTrack(Track *track);
    void clear();

private:
    QLabel *titleLabel;
    QLabel *trackNumberLabel;
    QLabel *lyricsLabel;

};

#endif // TRACKINFO_H
