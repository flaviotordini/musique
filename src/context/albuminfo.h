#ifndef ALBUMINFO_H
#define ALBUMINFO_H

#include <QtGui>

class Album;
class TrackListView;
class TrackSqlModel;

class AlbumInfo : public QWidget {

    Q_OBJECT

public:
    AlbumInfo(QWidget *parent = 0);
    void setAlbum(Album *album);
    void clear();

private slots:
#ifdef APP_AFFILIATE_AMAZON
    void amazonClicked();
#endif

private:
    QLabel *titleLabel;
    QLabel *photoLabel;
    QLabel *wikiLabel;
#ifdef APP_AFFILIATE_AMAZON
    QPushButton *buyOnAmazonButton;
#endif

    /*
    TrackListView *trackListView;
    TrackSqlModel *trackListModel;
    */

};

#endif // ALBUMINFO_H
