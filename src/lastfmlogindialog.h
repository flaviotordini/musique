#ifndef LASTFMLOGINDIALOG_H
#define LASTFMLOGINDIALOG_H

#include <QtGui>

class LastFmLoginDialog : public QDialog {

    Q_OBJECT

public:
    LastFmLoginDialog(QWidget *parent = 0);
    QString getUsername() const { return userEdit->text(); }
    QString getPassword() const { return passwordEdit->text(); }

signals:
    void authenticate(const QString& username, const QString& password);

private slots:
    void login();
    void authenticationError(QString);
    void checkFields();

private:
    QLineEdit* userEdit;
    QLineEdit* passwordEdit;
    QLabel* errorIconLabel;
    QLabel* errorLabel;
    QLabel* forgotLabel;
    QPushButton* loginButton;
    
};

#endif // LASTFMLOGINDIALOG_H
