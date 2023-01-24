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

#include "choosefolderview.h"
#include "constants.h"
#include "database.h"
#include "fontutils.h"
#include "iconutils.h"

ChooseFolderView::ChooseFolderView(QWidget *parent) : View(parent) {
    const int padding = 30;

    QBoxLayout *layout = new QHBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(padding);
    layout->setMargin(padding);

    QLabel *logo = new QLabel(this);
    auto setLogoPixmap = [logo] {
        logo->setPixmap(IconUtils::pixmap(":/images/app.png", logo->devicePixelRatioF()));
    };
    setLogoPixmap();
    connect(window()->windowHandle(), &QWindow::screenChanged, this, setLogoPixmap);
    layout->addWidget(logo, 0, Qt::AlignTop);

    QBoxLayout *vLayout = new QVBoxLayout();
    vLayout->setAlignment(Qt::AlignCenter);
    vLayout->setSpacing(padding);
    vLayout->setContentsMargins(0, 0, 0, 0);
    layout->addLayout(vLayout);

    // hLayout->addSpacing(PADDING);

    welcomeLabel = new QLabel(
            "<h1 style='font-weight:100'>" +
                    tr("Welcome to <a href='%1'>%2</a>,")
                            .replace("<a href", "<a style='text-decoration:none; "
                                                "color:palette(text); font-weight:normal' href")
                            .arg(Constants::WEBSITE, Constants::NAME) +
                    "</h1>",
            this);
    welcomeLabel->setOpenExternalLinks(true);
    welcomeLabel->setFont(FontUtils::light(welcomeLabel->font().pointSize() * 1.5));
    vLayout->addWidget(welcomeLabel);

    // layout->addSpacing(PADDING);

    tipLabel = new QLabel(tr("%1 needs to scan your music collection.").arg(Constants::NAME), this);
    tipLabel->setFont(FontUtils::medium());
    QColor lightTextColor = palette().text().color();
#ifdef APP_MAC
    lightTextColor.setAlphaF(.75);
#endif
    tipLabel->setStyleSheet(QLatin1String("color:") + lightTextColor.name(QColor::HexArgb));
    vLayout->addWidget(tipLabel);

    QBoxLayout *buttonLayout = new QHBoxLayout();
    vLayout->addLayout(buttonLayout);

    cancelButton = new QPushButton(tr("Cancel"));
    connect(cancelButton, SIGNAL(clicked()), parent, SLOT(goBack()));
    buttonLayout->addWidget(cancelButton);

#if defined APP_MAC && !defined APP_MAC_STORE
    QPushButton *useiTunesDirButton = new QPushButton(tr("Use %1 library").arg("Apple Music"));
    connect(useiTunesDirButton, SIGNAL(clicked()), SLOT(iTunesDirChosen()));
    buttonLayout->addWidget(useiTunesDirButton);
#endif

    QString musicLocation = getMusicLocation();
    if (!musicLocation.isEmpty()) {
        QString musicFolderName = QStandardPaths::displayName(QStandardPaths::MusicLocation);
        QPushButton *useSystemDirButton = new QPushButton(tr("Use %1 folder").arg(musicFolderName));
        connect(useSystemDirButton, SIGNAL(clicked()), SLOT(systemDirChosen()));
        useSystemDirButton->setDefault(true);
        useSystemDirButton->setFocus(Qt::NoFocusReason);
        buttonLayout->addWidget(useSystemDirButton);
    }

    QPushButton *chooseDirButton = new QPushButton(tr("Choose a folder..."));
    connect(chooseDirButton, SIGNAL(clicked()), SLOT(chooseFolder()));
    buttonLayout->addWidget(chooseDirButton);

    QLabel *privacyLabel = new QLabel(
            tr("%1 will connect to the Last.fm web services and pass artist names and album titles "
               "in order to fetch covert art, biographies and much more.")
                    .arg(Constants::NAME),
            this);
    privacyLabel->setFont(FontUtils::small());
    privacyLabel->setOpenExternalLinks(true);
    privacyLabel->setWordWrap(true);
    vLayout->addWidget(privacyLabel);
}

void ChooseFolderView::chooseFolder() {
#ifdef APP_MAC
    QFileDialog *dialog = new QFileDialog(this);
    dialog->setFileMode(QFileDialog::Directory);
    dialog->setOptions(QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks |
                       QFileDialog::ReadOnly);
    dialog->setDirectory(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    dialog->open(this, SLOT(folderChosen(const QString &)));
#else
    QString folder = QFileDialog::getExistingDirectory(
            window(), tr("Where's your music collection?"),
            QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks | QFileDialog::ReadOnly);
    if (!folder.isEmpty()) emit locationChanged(folder);
#endif
}

void ChooseFolderView::folderChosen(const QString &folder) {
    if (!folder.isEmpty()) emit locationChanged(folder);
}

QString ChooseFolderView::getMusicLocation() {
    QString musicLocation = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    QString homeLocation = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    qDebug() << musicLocation << homeLocation;

    bool validMusicLocation = QFile::exists(musicLocation) && musicLocation != homeLocation &&
                              musicLocation != homeLocation + "/";
    if (!validMusicLocation) {
        QString guessedMusicLocation = homeLocation + "/Music";
        validMusicLocation = QFile::exists(guessedMusicLocation);
        if (validMusicLocation) musicLocation = guessedMusicLocation;
    }

    if (!validMusicLocation) return QString();
    return musicLocation;
}

void ChooseFolderView::systemDirChosen() {
    QString musicLocation = getMusicLocation();
    emit locationChanged(musicLocation);
}

void ChooseFolderView::iTunesDirChosen() {
    QString musicLocation =
            QStandardPaths::writableLocation(QStandardPaths::MusicLocation) + "/Music/";
    emit locationChanged(musicLocation);
}

void ChooseFolderView::appear() {
    Database &db = Database::instance();
    if (db.status() == ScanComplete) {
        welcomeLabel->hide();
        tipLabel->setText(tr("Select the location of your music collection."));
        cancelButton->show();
    } else {
        welcomeLabel->show();
        tipLabel->setText(tr("%1 needs to scan your music collection.").arg(Constants::NAME));
        cancelButton->hide();
    }
}

void ChooseFolderView::paintEvent(QPaintEvent *e) {
    Q_UNUSED(e);
    QPainter painter(this);
    const QBrush &brush = window()->isActiveWindow() ? palette().base() : palette().window();
    painter.fillRect(rect(), brush);
}
