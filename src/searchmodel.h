#ifndef SEARCHMODEL_H
#define SEARCHMODEL_H

#include <QtGui>

class ArtistSqlModel;
class AlbumSqlModel;
class TrackSqlModel;
class FileSystemModel;
class FinderWidget;
class Item;

class SearchModel : public QAbstractListModel {

    Q_OBJECT

public:
    SearchModel(QObject *parent = 0);
    void search(QString query);
    void setHoveredRow(int row);

public slots:
    void clearHover();
    void enterPlayIconHover();
    void exitPlayIconHover();

protected:
    int rowCount(const QModelIndex & /* parent */) const;
    QVariant data(const QModelIndex &item, int role) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const { return 1; }

private slots:
    void updatePlayIcon();

    void itemEntered(const QModelIndex &index);
    void itemActivated(const QModelIndex &index);
    void itemPlayed(const QModelIndex &index);

private:
    Item* itemAt(const QModelIndex &index) const;

    ArtistSqlModel *artistListModel;
    AlbumSqlModel *albumListModel;
    TrackSqlModel *trackListModel;
    FileSystemModel *fileSystemModel;

    // drag and drop
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QStringList mimeTypes() const;
    Qt::DropActions supportedDropActions() const;
    QMimeData* mimeData( const QModelIndexList &indexes ) const;

    int hoveredRow;
    QTimeLine * timeLine;
    bool playIconHovered;

    FinderWidget *finder;

};

#endif // SEARCHMODEL_H
