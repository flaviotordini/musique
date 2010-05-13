#ifndef CONTEXTUALVIEW_H
#define CONTEXTUALVIEW_H

#include <QtGui>
#include "view.h"

class Track;
class ArtistInfo;
class AlbumInfo;
class TrackInfo;

class ContextualView : public QWidget, public View {

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

protected:
    void paintEvent(QPaintEvent *event);

private:
    ArtistInfo *artistInfo;
    AlbumInfo *albumInfo;
    TrackInfo *trackInfo;

};

#endif // CONTEXTUALVIEW_H
