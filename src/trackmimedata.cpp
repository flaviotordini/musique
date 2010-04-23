#include "trackmimedata.h"

TrackMimeData::TrackMimeData() { }

QStringList TrackMimeData::formats() const {
    QStringList formats( QMimeData::formats() );
    formats.append(TRACK_MIME);
    return formats;
}

bool TrackMimeData::hasFormat( const QString &mimeType ) const {
    return mimeType == TRACK_MIME;
}

