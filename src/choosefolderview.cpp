#include "choosefolderview.h"
#include "constants.h"
#include "fontutils.h"
#include "database.h"

static const int PADDING = 30;

ChooseFolderView::ChooseFolderView( QWidget *parent ) : QWidget(parent) {

    QBoxLayout *layout = new QHBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(PADDING);
    layout->setMargin(PADDING);

    QLabel *logo = new QLabel(this);
    logo->setPixmap(QPixmap(":/images/app.png"));
    layout->addWidget(logo, 0, Qt::AlignTop);

    QBoxLayout *vLayout = new QVBoxLayout();
    vLayout->setAlignment(Qt::AlignCenter);
    vLayout->setSpacing(PADDING);
    vLayout->setMargin(0);
    layout->addLayout(vLayout);

    // hLayout->addSpacing(PADDING);

    welcomeLabel =
            new QLabel("<h1 style='font-weight:normal'>" +
                       tr("Welcome to <a href='%1'>%2</a>,")
                       .replace("<a href", "<a style='text-decoration:none; color:palette(text); font-weight:bold' href")
                       .arg(Constants::WEBSITE, Constants::NAME)
                       + "</h1>", this);
    welcomeLabel->setOpenExternalLinks(true);
    vLayout->addWidget(welcomeLabel);

    // layout->addSpacing(PADDING);

    tipLabel = new QLabel(
            tr("%1 needs to scan your music collection.").arg(Constants::NAME)
            , this);
    tipLabel->setFont(FontUtils::big());
    vLayout->addWidget(tipLabel);

    QBoxLayout *buttonLayout = new QHBoxLayout();
    vLayout->addLayout(buttonLayout);

    cancelButton = new QPushButton(tr("Cancel"));
    connect(cancelButton, SIGNAL(clicked()), parent, SLOT(goBack()));
    buttonLayout->addWidget(cancelButton);

#ifdef APP_MAC_NO
    QPushButton *useiTunesDirButton = new QPushButton(tr("Use iTunes collection"));
    connect(useiTunesDirButton, SIGNAL(clicked()), SLOT(iTunesDirChosen()));
    useiTunesDirButton->setDefault(true);
    useiTunesDirButton->setFocus(Qt::NoFocusReason);
    buttonLayout->addWidget(useiTunesDirButton);
#endif

    QString musicLocation = QDesktopServices::storageLocation(QDesktopServices::MusicLocation);
    QString homeLocation = QDesktopServices::storageLocation(QDesktopServices::HomeLocation);
    if (QFile::exists(musicLocation) && musicLocation != homeLocation + "/") {
        QPushButton *useSystemDirButton = new QPushButton(tr("Use %1").arg(musicLocation));
        connect(useSystemDirButton, SIGNAL(clicked()), SLOT(systemDirChosen()));
#ifndef APP_MAC_NO
        useSystemDirButton->setDefault(true);
        useSystemDirButton->setFocus(Qt::NoFocusReason);
#endif
        buttonLayout->addWidget(useSystemDirButton);
    }

    QPushButton *chooseDirButton = new QPushButton(tr("Choose a folder..."));
    connect(chooseDirButton, SIGNAL(clicked()), SLOT(chooseFolder()));
    buttonLayout->addWidget(chooseDirButton);

#if !defined(APP_MAC) && !defined(Q_WS_WIN)
    QLabel *privacyLabel =
            new QLabel(
                    tr("%1 will connect to the Last.fm web services and pass artist names and album titles in order to fetch covert art, biographies and much more.")
                    .arg(Constants::NAME) + " " +
                    tr("If you have privacy concerns about this you can quit now.")
                    , this);
    privacyLabel->setFont(FontUtils::small());
    privacyLabel->setOpenExternalLinks(true);
    privacyLabel->setWordWrap(true);
    vLayout->addWidget(privacyLabel);
#endif

}

void ChooseFolderView::chooseFolder() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Where's your music collection?"),
                                                    QDesktopServices::storageLocation(QDesktopServices::HomeLocation),
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
        emit locationChanged(dir);
    }

}

void ChooseFolderView::systemDirChosen() {
    QString musicLocation = QDesktopServices::storageLocation(QDesktopServices::MusicLocation);
    emit locationChanged(musicLocation);
}

void ChooseFolderView::iTunesDirChosen() {
    QString musicLocation = QDesktopServices::storageLocation(QDesktopServices::MusicLocation) + "/iTunes/iTunes Music";
    emit locationChanged(musicLocation);
}

void ChooseFolderView::appear() {
    Database &db = Database::instance();
    if (db.status() == ScanComplete) {
        tipLabel->setText(tr("Select the location of your music collection."));
        cancelButton->show();
        welcomeLabel->hide();
    } else {
        tipLabel->setText(tr("%1 needs to scan your music collection.").arg(Constants::NAME));
        cancelButton->hide();
        welcomeLabel->show();
    }
}

void ChooseFolderView::paintEvent(QPaintEvent * /*event*/) {
#if defined(APP_MAC) | defined(APP_WIN)
    QBrush brush;
    if (window()->isActiveWindow()) {
        brush = QBrush(QColor(0xdd, 0xe4, 0xeb));
    } else {
        brush = palette().window();
    }
    QPainter painter(this);
    painter.fillRect(0, 0, width(), height(), brush);
#endif
}
