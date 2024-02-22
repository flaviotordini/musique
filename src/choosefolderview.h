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

#include <QtWidgets>

class ChooseFolderView : public QWidget {
    Q_OBJECT

public:
    ChooseFolderView(QWidget *parent);

signals:
    void locationChanged(QString dir);

protected:
    void showEvent(QShowEvent *e);
    void paintEvent(QPaintEvent *e);

private slots:
    void chooseFolder();
    void systemDirChosen();
    void iTunesDirChosen();
    void folderChosen(const QString &folder);

private:
    QString getMusicLocation();

    QLabel *welcomeLabel;
    QLabel *tipLabel;
    QPushButton *cancelButton;
};
#endif
