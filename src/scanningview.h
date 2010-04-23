#ifndef SCANNINGVIEW_H
#define SCANNINGVIEW_H

#include <QtGui>
#include "view.h"

class ScanningView : public QWidget, public View {

    Q_OBJECT

public:
    ScanningView(QWidget *parent);

    void appear() {}
    void disappear() {}
    QMap<QString, QVariant> metadata() {
        QMap<QString, QVariant> metadata;
        metadata.insert("title", "");
        metadata.insert("description", tr("Scanning your music collection"));
        return metadata;
    }

private:
    QProgressBar *progressBar;

};

#endif // SCANNINGVIEW_H
