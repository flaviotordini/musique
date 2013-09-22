/* $BEGIN_LICENSE

This file is part of Musique.
Copyright 2013, Flavio Tordini <flavio.tordini@gmail.com>

Musique is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Musique is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Musique.  If not, see <http://www.gnu.org/licenses/>.

$END_LICENSE */

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
