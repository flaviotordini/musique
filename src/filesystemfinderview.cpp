#include "filesystemfinderview.h"

FileSystemFinderView::FileSystemFinderView(QWidget *parent) :
        BaseFinderView(parent) {
    setWindowTitle(tr("Folders"));
}

void FileSystemFinderView::appear() {
    QFileSystemModel *fileSystemModel = static_cast<QFileSystemModel*>(model());
    if (fileSystemModel) {
        QSettings settings;
        const QString path = settings.value("collectionRoot").toString();
        fileSystemModel->setRootPath(path);
        fileSystemModel->setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
        setRootIndex(fileSystemModel->index(path));
    }
}
