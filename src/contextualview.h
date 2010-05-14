#ifndef CONTEXTUALVIEW_H
#define CONTEXTUALVIEW_H

#include <QtGui>
#include "view.h"

class Track;
class ArtistInfo;
class AlbumInfo;
class TrackInfo;

class ScrollingContextualView : public QWidget {

    Q_OBJECT

public:
    ScrollingContextualView(QWidget *parent);

    ArtistInfo *artistInfo;
    AlbumInfo *albumInfo;
    TrackInfo *trackInfo;

protected:
    void paintEvent(QPaintEvent *event);

};


class ContextualView : public QScrollArea, public View {

    Q_OBJECT

public:
    ContextualView(QWidget *parent);

    void appear() {}
    void disappear();
    QMap<QString, QVariant> metadata() {
        QMap<QString, QVariant> metadata;
        metadata.insert("title", "");
        metadata.insert("description", "");
        return metadata;
    }
    void setTrack(Track* track);

private:
    ScrollingContextualView *scrollingContextualView;

};

#endif // CONTEXTUALVIEW_H
