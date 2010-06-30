#include "filesystemfinderview.h"
#include "database.h"
#include "filesystemmodel.h"

FileSystemFinderView::FileSystemFinderView(QWidget *parent) :
        BaseFinderView(parent) { }

void FileSystemFinderView::appear() {
    BaseFinderView::appear();
    if (fileSystemModel) {
        fileSystemModel->setRootPath(fileSystemModel->rootPath());
    }
}
