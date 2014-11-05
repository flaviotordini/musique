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

#ifndef FINDERWIDGET_H
#define FINDERWIDGET_H

#include <QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets>
#endif

class BreadcrumbWidget;
class PlaylistModel;
class PlaylistView;
class SegmentedControl;
class ArtistListView;
class ArtistSqlModel;
class AlbumListView;
class AlbumSqlModel;
class TrackSqlModel;
class FileSystemModel;
class FilteringFileSystemModel;
class FileSystemFinderView;
class Track;
class SearchModel;
class SearchView;
class Artist;
class Album;

namespace Finder {

    enum FinderDataRoles {
        ItemTypeRole = Qt::UserRole,
        DataObjectRole,
        ActiveItemRole,
        HoveredItemRole,
        PlayIconAnimationItemRole,
        PlayIconHoveredRole
    };

    enum FinderItemTypes {
        ItemTypeGenre = 1,
        ItemTypeArtist ,
        ItemTypeAlbum,
        ItemTypeTrack,
        ItemTypeFolder
    };

}

class FinderWidget : public QWidget {

    Q_OBJECT

public:
    FinderWidget(QWidget *parent);
    void setPlaylistModel(PlaylistModel *playlistModel) {
        this->playlistModel = playlistModel;
    }
    void setPlaylistView(PlaylistView *playlistView) {
        this->playlistView = playlistView;
    }
    void appear();
    void disappear();
    void showSearch(QString query);
    void addTracksAndPlay(QList<Track*> tracks);
    void artistActivated(Artist *artist);
    void albumActivated(Album *album);
    void trackActivated(Track *track);

protected:
    void paintEvent(QPaintEvent *e);

private slots:
    void goBack();
    void folderGoBack();
    void showArtists();
    void showAlbums();
    void showFolders();

    void artistEntered(const QModelIndex &index);
    void artistActivated(const QModelIndex &index);
    void artistPlayed(const QModelIndex &index);

    void albumEntered(const QModelIndex &index);
    void albumActivated(const QModelIndex &index);
    void albumPlayed(const QModelIndex &index);

    void trackEntered(const QModelIndex &index);
    void trackActivated(const QModelIndex &index);

    void folderEntered(const QModelIndex &index);
    void folderActivated(const QModelIndex &index);
    void folderPlayed(const QModelIndex &index);

private:
    void restoreSavedView();
    void setupBar();
    void setupArtists();
    void setupAlbums();
    void setupFolders();
    void setupTracks();
    void setupSearch();
    void showWidget(QWidget *widget, bool isRoot);

    SegmentedControl *finderBar;
    QAction *artistsAction;
    QAction *albumsAction;
    QAction *foldersAction;

    QStack<QWidget*> *history;
    QStackedWidget *stackedWidget;
    BreadcrumbWidget *breadcrumb;
    BreadcrumbWidget *folderBreadcrumb;

    PlaylistModel *playlistModel;
    PlaylistView *playlistView;

    FileSystemFinderView *fileSystemView;
    FileSystemModel *fileSystemModel;
    FilteringFileSystemModel *filteringFileSystemModel;

    ArtistListView *artistListView;
    ArtistSqlModel *artistListModel;

    AlbumListView *albumListView;
    AlbumSqlModel *albumListModel;

    QListView *trackListView;
    TrackSqlModel *trackListModel;

    SearchModel *searchModel;
    SearchView *searchView;

};

#endif // FINDERWIDGET_H
