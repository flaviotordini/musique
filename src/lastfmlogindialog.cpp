#include "lastfmlogindialog.h"
#include "fontutils.h"
#include "lastfm.h"

namespace The {
QHash<QString, QAction*>* globalActions();
}

LastFmLoginDialog::LastFmLoginDialog(QWidget *parent) : QDialog(parent) {
    setWindowModality(Qt::WindowModal);

    QFormLayout* layout = new QFormLayout(this);

    QLabel *label = new QLabel(tr("Log in to %1").arg("<b>Last.fm</b>"));
    label->setFont(FontUtils::big());
    layout->addWidget(label);

    // hack around https://bugreports.qt-project.org/browse/QTBUG-18308
    setMinimumWidth(label->sizeHint().width() * 2);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    errorIconLabel = new QLabel();
    errorIconLabel->setMargin(10);
    errorIconLabel->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(16, 16));
    errorIconLabel->hide();
    errorLabel = new QLabel();
    errorLabel->setTextFormat(Qt::PlainText);
    errorLabel->hide();
    layout->addRow(errorIconLabel, errorLabel);

    userEdit = new QLineEdit(this);
    userEdit->setText(LastFm::instance().getUsername());
    connect(userEdit, SIGNAL(textChanged(QString)), SLOT(checkFields()));
    layout->addRow(tr("&Username:"), userEdit);

    passwordEdit = new QLineEdit(this);
    passwordEdit->setEchoMode(QLineEdit::Password);
    if (!userEdit->text().isEmpty()) passwordEdit->setFocus();
    connect(passwordEdit, SIGNAL(textChanged(QString)), SLOT(checkFields()));
    layout->addRow(tr("&Password:"), passwordEdit);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);

    QPushButton *cancelButton = new QPushButton(tr("Cancel"));
    connect(cancelButton, SIGNAL(clicked()), SLOT(close()));
    buttonBox->addButton(cancelButton, QDialogButtonBox::RejectRole);

    loginButton = new QPushButton("Log In");
    loginButton->setEnabled(false);
    connect(loginButton, SIGNAL(clicked()), SLOT(login()));
    buttonBox->addButton(loginButton, QDialogButtonBox::AcceptRole);

    layout->addWidget(buttonBox);

    QLabel *signupLabel = new QLabel("<a href='http://www.last.fm/join' style='color:palette(text)'>" +
                                     tr("Signup for a %1 account").arg("Last.fm") +
                                     "</a>");
    signupLabel->setOpenExternalLinks(true);
    signupLabel->setFont(FontUtils::small());
    connect(signupLabel, SIGNAL(linkActivated(QString)), SLOT(close()));
    layout->addWidget(signupLabel);

    forgotLabel = new QLabel(
                tr("Forgot your <a href='%1'>username</a> or <a href='%2'>password</a>?")
                .arg("https://www.last.fm/settings/lostusername/",
                     "https://www.last.fm/settings/lostpassword/")
                .replace("<a href=", "<a style='color:palette(text)' href=")
                );
    forgotLabel->hide();
    forgotLabel->setOpenExternalLinks(true);
    forgotLabel->setFont(FontUtils::small());
    connect(forgotLabel, SIGNAL(linkActivated(QString)), SLOT(close()));
    layout->addWidget(forgotLabel);

}

void LastFmLoginDialog::login() {
    errorLabel->hide();
    forgotLabel->hide();
    connect(&LastFm::instance(), SIGNAL(authenticated()),
            SLOT(accept()), Qt::UniqueConnection);
    connect(&LastFm::instance(), SIGNAL(error(QString)),
            SLOT(authenticationError(QString)), Qt::UniqueConnection);
    LastFm::instance().authenticate(userEdit->text(), passwordEdit->text());
}

void LastFmLoginDialog::authenticationError(QString message) {
    errorLabel->setText(message);
    errorLabel->show();
#ifdef APP_MAC
    errorIconLabel->show();
#endif
    forgotLabel->show();
    userEdit->setFocus();
    userEdit->selectAll();
}

void LastFmLoginDialog::checkFields() {
    loginButton->setEnabled(!userEdit->text().isEmpty() && !passwordEdit->text().isEmpty());
}
