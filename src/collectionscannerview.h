#ifndef COLLECTIONSCANNERVIEW_H
#define COLLECTIONSCANNERVIEW_H

#include <QtGui>
#include "view.h"
#include "collectionscannerthread.h"

class CollectionScannerView : public QWidget, public View {

    Q_OBJECT

public:
    CollectionScannerView(QWidget *parent);

    void appear() { progressBar->setMaximum(0); }
    void disappear() {}
    QHash<QString, QVariant> metadata() {
        QHash<QString, QVariant> metadata;
        metadata.insert("description", tr("Go grab a coffee"));
        return metadata;
    }

protected:
    void paintEvent(QPaintEvent *);

public slots:
    void setCollectionScannerThread(CollectionScannerThread *scannerThread);

private slots:
    void progress(int value);
    void scanFinished();
    void scanError(QString message);

private:
    QProgressBar *progressBar;

};

#endif // COLLECTIONSCANNERVIEW_H
