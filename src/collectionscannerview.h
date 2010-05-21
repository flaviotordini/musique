#ifndef COLLECTIONSCANNERVIEW_H
#define COLLECTIONSCANNERVIEW_H

#include <QtGui>
#include "view.h"
#include "collectionscannerthread.h"

class CollectionScannerView : public QWidget, public View {

    Q_OBJECT

public:
    CollectionScannerView(QWidget *parent);

    void appear() {}
    void disappear() {}
    QMap<QString, QVariant> metadata() {
        QMap<QString, QVariant> metadata;
        metadata.insert("title", "");
        metadata.insert("description", tr("Go grab a coffee"));
        return metadata;
    }

public slots:
    void startScan(QString dir);

private slots:
    void progress(int value);
    void scanFinished();
    void scanError(QString message);

private:
    QProgressBar *progressBar;

};

#endif // COLLECTIONSCANNERVIEW_H
