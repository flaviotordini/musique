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

#include <QtWidgets>

class Breadcrumb;
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
class FinderListView;
class GenresModel;

namespace Finder {

enum FinderDataRoles {
    ItemTypeRole = Qt::UserRole,
    DataObjectRole,
    ItemObjectRole,
    ActiveItemRole
};

enum FinderItemTypes {
    ItemTypeArtist = 1,
    ItemTypeAlbum,
    ItemTypeTrack,
    ItemTypeFolder,
    ItemTypeGenre,
    ItemTypeDecade
};

} // namespace Finder

class FinderWidget : public QWidget {
    Q_OBJECT

public:
    FinderWidget(QWidget *parent);
    void setPlaylistModel(PlaylistModel *playlistModel) { this->playlistModel = playlistModel; }
    void setPlaylistView(PlaylistView *playlistView) { this->playlistView = playlistView; }
    void showSearch(const QString &query);
    void addTracksAndPlay(const QList<Track *> &tracks);
    void artistActivated(Artist *artist);
    void albumActivated(Album *album);
    void trackActivated(Track *track);

private slots:
    void goBack();
    void folderGoBack();
    void showArtists();
    void showAlbums();
    void showGenres();
    void showFolders();

    void artistActivated(const QModelIndex &index);
    void artistPlayed(const QModelIndex &index);

    void albumActivated(const QModelIndex &index);
    void albumPlayed(const QModelIndex &index);

    void trackActivated(const QModelIndex &index);

    void folderActivated(const QModelIndex &index);
    void folderPlayed(const QModelIndex &index);

    void genreActivated(const QModelIndex &index);
    void genrePlayed(const QModelIndex &index);

private:
    void restoreSavedView();
    void setupBar();
    void setupArtists();
    void setupAlbums();
    void setupGenres();
    void setupFolders();
    void setupTracks();
    void setupSearch();
    void showWidget(QWidget *widget, bool isRoot);
    void maybeShowMessage();

    SegmentedControl *bar;
    QAction *artistsAction;
    QAction *albumsAction;
    QAction *genresAction;
    QAction *foldersAction;

    QStack<QWidget *> history;
    QStackedWidget *stackedWidget;
    Breadcrumb *breadcrumb;
    Breadcrumb *folderBreadcrumb;

    PlaylistModel *playlistModel;
    PlaylistView *playlistView;

    FileSystemFinderView *fileSystemView;
    FileSystemModel *fileSystemModel;
    FilteringFileSystemModel *filteringFileSystemModel;

    ArtistListView *artistListView;
    ArtistSqlModel *artistListModel;

    AlbumListView *albumListView;
    AlbumSqlModel *albumListModel;

    FinderListView *genresListView;
    GenresModel *genresModel;

    QListView *trackListView;
    TrackSqlModel *trackListModel;

    SearchModel *searchModel;
    SearchView *searchView;
};

#endif // FINDERWIDGET_H
