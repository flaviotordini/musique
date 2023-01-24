#include "genres.h"

#include <QtSql>

#include "database.h"
#include "datautils.h"
#include "mainwindow.h"

#include "model/decade.h"
#include "model/genre.h"
#include "model/item.h"

Genres::Genres() : QObject() {
    connect(MainWindow::instance(), &MainWindow::collectionCreated, this, &Genres::init);
}

void Genres::init() {
    emit initializing();
    items.clear();
    loadGenres();
    loadDecades();
    emit initialized();
}

void Genres::loadGenres() {
    static const auto genreTree = [] {
        QVector<QPair<QString, QStringList>> map;
        QFile f(":/res/genre-tree.csv");
        if (f.open(QFile::ReadOnly)) {
            QTextStream stream(&f);
            QString line;
            while (!stream.atEnd()) {
                stream.readLineInto(&line);
                if (line.isEmpty()) continue;
                QString badWord;
                QString goodWord;
                const auto fields = line.split(',');
                QString metaGenre;
                QStringList genres;
                for (const auto &field : fields) {
                    if (metaGenre.isNull())
                        metaGenre = field;
                    else
                        genres << field;
                }
                map.append(qMakePair(metaGenre, genres));
            }
        }
        return map;
    }();
    qDebug() << genreTree;

    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select id from genres where trackCount > 5 "
                  "order by trackCount desc");
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();

    QVector<Genre *> metaGenres;
    metaGenres.reserve(genreTree.size());

    QVector<Genre *> otherGenres;
    otherGenres.reserve(query.size() / 2);

    while (query.next()) {
        Genre *genre = Genre::forId(query.value(0).toInt());
        const QString &genreHash = genre->getHash();
        qDebug() << "Processing" << genreHash;

        Genre *metaGenre = nullptr;
        bool addAsChild = false;

        for (const auto &pair : genreTree) {
            const QString &metaGenreHash = pair.first;

            if (genreHash == metaGenreHash) {
                qDebug() << "It's a meta" << metaGenreHash;
                metaGenre = Genre::forHash(metaGenreHash);
                break;
            }

            if (genreHash.contains(metaGenreHash)) {
                qDebug() << "It's a word child" << metaGenreHash << genreHash;
                metaGenre = Genre::forHash(metaGenreHash);
                addAsChild = true;
                break;
            }

            const QStringList &subGenreHashes = pair.second;
            for (const auto &subGenreHash : subGenreHashes) {
                if (genreHash.contains(subGenreHash)) {
                    qDebug() << "Found subgenre" << subGenreHash << metaGenreHash;
                    metaGenre = Genre::forHash(metaGenreHash);
                    addAsChild = true;

                    const auto &siblings = metaGenre->getChildren();
                    bool parentFound = false;
                    for (Genre *sibling : siblings) {
                        if (genreHash.contains(sibling->getHash())) {
                            qDebug() << genreHash << sibling->getHash();
                            sibling->addChild(genre);
                            parentFound = true;
                            break;
                        }
                    }
                    if (!parentFound) metaGenre->addChild(genre);

                    break;
                }
            }

            if (metaGenre) break;
        }

        if (metaGenre) {
            if (!metaGenres.contains(metaGenre)) metaGenres << metaGenre;

            if (addAsChild) {
                const auto &siblings = metaGenre->getChildren();
                bool parentFound = false;
                for (Genre *sibling : siblings) {
                    if (genreHash.contains(sibling->getHash())) {
                        qDebug() << genreHash << sibling->getHash();
                        sibling->addChild(genre);
                        parentFound = true;
                        break;
                    }
                }
                if (!parentFound) metaGenre->addChild(genre);
            }

        } else if (genre->getTrackCount() > 20) {
            otherGenres << genre;
        }
    }

    // remove almost empty metagenres
    for (auto i = metaGenres.begin(); i != metaGenres.end(); /* NOTHING */) {
        Genre *g = *i;
        qDebug() << g->getName() << g->getTotalTrackCount();
        if (g->getTotalTrackCount() <= 30) {
            qDebug() << "Dropping" << g->getName();
            metaGenres.removeOne(g);
        } else
            ++i;
    }

    items.reserve(metaGenres.size() + otherGenres.size());
    for (const auto &g : qAsConst(metaGenres)) {
        g->setRow(items.size());
        items.append(g);
    }
    for (const auto &g : qAsConst(otherGenres))
        items.append(g);
}

void Genres::loadDecades() {
    QSqlDatabase db = Database::instance().getConnection();
    QSqlQuery query(db);
    query.prepare("select distinct year from tracks where year >= 1900 "
                  "order by year desc");
    bool success = query.exec();
    if (!success) qDebug() << query.lastError().text();
    if (query.size() == 0) return;

    Decade *currentDecade = nullptr;
    int currentStartYear = 0;
    while (query.next()) {
        int trackYear = query.value(0).toInt();
        int decadeStartYear = qFloor(trackYear / 10.) * 10;
        if (decadeStartYear != currentStartYear) {
            currentStartYear = decadeStartYear;
            qDebug() << "Decade" << decadeStartYear;
            currentDecade = new Decade();
            currentDecade->setName(QString::number(decadeStartYear) + 's');
            currentDecade->setStartYear(decadeStartYear);
            items.append(currentDecade);
        }
    }
}
