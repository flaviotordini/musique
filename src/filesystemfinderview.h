#ifndef FILESYSTEMFINDERVIEW_H
#define FILESYSTEMFINDERVIEW_H

#include <QtGui>
#include "basefinderview.h"

class FileSystemFinderView : public BaseFinderView {

    Q_OBJECT

public:
    FileSystemFinderView(QWidget *parent = 0);

public slots:
    void appear();

};

#endif // FILESYSTEMFINDERVIEW_H
