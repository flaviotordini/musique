#ifndef FILESYSTEMFINDERVIEW_H
#define FILESYSTEMFINDERVIEW_H

#include <QtGui>
#include "basefinderview.h"

class FileSystemModel;

class FileSystemFinderView : public BaseFinderView {

    Q_OBJECT

public:
    FileSystemFinderView(QWidget *parent = 0);
    void setFileSystemModel(FileSystemModel *fileSystemModel) {
        this->fileSystemModel = fileSystemModel;
    }

public slots:
    void appear();

private:
    FileSystemModel *fileSystemModel;

};

#endif // FILESYSTEMFINDERVIEW_H
