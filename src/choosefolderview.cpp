#include "choosefolderview.h"
#include "constants.h"
#include "fontutils.h"

static const int PADDING = 30;

ChooseFolderView::ChooseFolderView( QWidget *parent ) : QWidget(parent) {

    QBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(PADDING);
    layout->setMargin(0);

    QBoxLayout *hLayout = new QHBoxLayout();
    hLayout->setAlignment(Qt::AlignLeft);
    hLayout->setSpacing(0);
    hLayout->setMargin(0);
    layout->addLayout(hLayout);

    QLabel *logo = new QLabel(this);
    logo->setPixmap(QPixmap(":/images/app.png"));
    hLayout->addWidget(logo, 0, Qt::AlignTop);

    // hLayout->addSpacing(PADDING);

    QLabel *welcomeLabel =
            new QLabel("<h1>" +
                       tr("Welcome to <a href='%1'>%2</a>,")
                       .replace("<a ", "<a style='color:palette(text)'")
                       .arg(Constants::WEBSITE, Constants::APP_NAME)
                       + "</h1>", this);
    welcomeLabel->setOpenExternalLinks(true);
    hLayout->addWidget(welcomeLabel);

    // layout->addSpacing(PADDING);

    QLabel *tipLabel = new QLabel(
            tr("%1 needs to scan your music collection.").arg(Constants::APP_NAME)
            , this);
    tipLabel->setFont(FontUtils::big());
    layout->addWidget(tipLabel);

    QBoxLayout *buttonLayout = new QHBoxLayout();
    layout->addLayout(buttonLayout);

#ifdef Q_WS_MAC
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
#ifndef Q_WS_MAC
        useSystemDirButton->setDefault(true);
        useSystemDirButton->setFocus(Qt::NoFocusReason);
#endif
        buttonLayout->addWidget(useSystemDirButton);
    }

    QPushButton *chooseDirButton = new QPushButton(tr("Choose a folder..."));
    connect(chooseDirButton, SIGNAL(clicked()), SLOT(chooseFolder()));
    buttonLayout->addWidget(chooseDirButton);

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
