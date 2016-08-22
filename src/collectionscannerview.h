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

#ifndef COLLECTIONSCANNERVIEW_H
#define COLLECTIONSCANNERVIEW_H

#include <QtWidgets>
#include "view.h"
#include "collectionscannerthread.h"

class CollectionScannerView : public View {

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

public slots:
    void setCollectionScannerThread(CollectionScannerThread *scannerThread);

private slots:
    void progress(int value);
    void scanError(QString message);

private:
    QProgressBar *progressBar;

};

#endif // COLLECTIONSCANNERVIEW_H
