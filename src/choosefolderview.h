#ifndef CHOOSEFOLDERVIEW_H
#define CHOOSEFOLDERVIEW_H

#include <QtGui>
#include "view.h"

class ChooseFolderView : public QWidget, public View {

    Q_OBJECT

public:
    ChooseFolderView(QWidget *parent);
    void appear() {}
    void disappear() {}
    QMap<QString, QVariant> metadata() {
        QMap<QString, QVariant> metadata;
        metadata.insert("title", tr("Locate your collection"));
#ifdef Q_WS_MAC
        metadata.insert("description", tr("%1 wants you to ignore the filesystem, it's time to grow up").arg("Apple"));
#else
        metadata.insert("description", tr("I bet it is ~/Music"));
#endif
        return metadata;
    }

signals:
    void locationChanged(QString dir);

private slots:
    void chooseFolder();
    void systemDirChosen();
    void iTunesDirChosen();

private:

};
#endif
