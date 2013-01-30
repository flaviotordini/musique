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
    QHash<QString, QVariant> metadata() {
        QHash<QString, QVariant> metadata;
        metadata.insert("title", tr("Locate your collection"));
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
    void folderChosen(const QString &folder);

private:
    QLabel *welcomeLabel;
    QLabel *tipLabel;
    QPushButton *cancelButton;

};
#endif
