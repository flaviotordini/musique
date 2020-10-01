#ifndef LYRICS_H
#define LYRICS_H

#include <QObject>

class Lyrics : public QObject {
    Q_OBJECT

public:
    explicit Lyrics(QObject *parent = nullptr);
    static Lyrics &get(QString artist, QString song);

    template <typename Functor> Lyrics &onData(Functor lambda) {
        connect(this, &Lyrics::data, this, lambda);
        return *this;
    }

    template <typename Functor> Lyrics &onError(Functor lambda) {
        connect(this, &Lyrics::error, this, lambda);
        return *this;
    }

signals:
    void data(QString lyrics);
    void error(QString message);
};

#endif // LYRICS_H
