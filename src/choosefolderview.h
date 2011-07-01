#ifndef CHOOSEFOLDERVIEW_H
#define CHOOSEFOLDERVIEW_H

#include <QtGui>
#include "view.h"

class ChooseFolderView : public QWidget, public View {

    Q_OBJECT

public:
    ChooseFolderView(QWidget *parent);
    void appear();
    void disappear() {}
    QMap<QString, QVariant> metadata() {
        QMap<QString, QVariant> metadata;
        metadata.insert("title", tr("Locate your collection"));
        metadata.insert("description", "");
        return metadata;
    }

signals:
    void locationChanged(QString dir);

protected:
    void paintEvent(QPaintEvent *);

private slots:
    void chooseFolder();
    void systemDirChosen();
    void iTunesDirChosen();

private:
    QLabel *welcomeLabel;
    QLabel *tipLabel;
    QPushButton *cancelButton;

};
#endif
