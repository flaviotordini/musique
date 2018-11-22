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

#include "aboutview.h"
#include "constants.h"
#include "iconutils.h"
#ifdef APP_ACTIVATION
#include "activation.h"
#endif
#ifdef APP_MAC
#include "mac_startup.h"
#include "macutils.h"
#endif
#include "appwidget.h"

AboutView::AboutView(QWidget *parent) : View(parent) {
    const int padding = 30;

    connect(window()->windowHandle(), SIGNAL(screenChanged(QScreen *)), SLOT(screenChanged()));

    QBoxLayout *verticalLayout = new QVBoxLayout(this);
    verticalLayout->setMargin(0);
    verticalLayout->setSpacing(0);

    QBoxLayout *aboutlayout = new QHBoxLayout();
    verticalLayout->addLayout(aboutlayout, 1);
    aboutlayout->setAlignment(Qt::AlignCenter);
    aboutlayout->setMargin(padding);
    aboutlayout->setSpacing(padding);

    logo = new QLabel();
    logo->setPixmap(IconUtils::pixmap(":/images/app.png"));
    aboutlayout->addWidget(logo, 0, Qt::AlignTop);

    QBoxLayout *layout = new QVBoxLayout();
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(padding);
    aboutlayout->addLayout(layout);

    QString info = "<html><style>a { color: palette(text); text-decoration: none; font-weight: "
                   "bold }</style><body>";

    info += "<h1 style='font-weight:100'>" + QString(Constants::NAME) + "</h1>";

    info += "<p>" + tr("Version %1").arg(Constants::VERSION) + "</p>";

    info += QString("<p><a href='%1/'>%1</a></p>").arg(Constants::WEBSITE);

#ifdef APP_ACTIVATION
    if (Activation::instance().isActivated()) {
        info += "<p>" +
                tr("Licensed to: %1").arg("<b>" + Activation::instance().getEmail() + "</b>") +
                "</p>";
    }
#endif

    info += "<p>" +
            tr("Translate %1 to your native language using %2")
                    .arg(Constants::NAME)
                    .arg("<a href='http://www.transifex.net/projects/p/" +
                         QLatin1String(Constants::UNIX_NAME) + "/'>Transifex</a>") +
            "</p>";

#ifndef APP_EXTRA
    "<p>" +
            tr("Released under the <a href='%1'>GNU General Public License</a>")
                    .arg("http://www.gnu.org/licenses/gpl.html") +
            "</p>";
#endif

    const char *buildYear = __DATE__ + 7;
    info += "<p>&copy; " + QLatin1String(buildYear) + " " + QLatin1String(Constants::ORG_NAME) +
            "</p></body></html>";

    QLabel *infoLabel = new QLabel(info, this);
    infoLabel->setOpenExternalLinks(true);
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);

    QLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignLeft);
    QPushButton *closeButton = new QPushButton(tr("&Close"), this);
    closeButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    closeButton->setDefault(true);
    closeButton->setFocus();
    connect(closeButton, SIGNAL(clicked()), parent, SLOT(goBack()));
    buttonLayout->addWidget(closeButton);

    layout->addLayout(buttonLayout);

    verticalLayout->addWidget(new AppsWidget());
}

void AboutView::paintEvent(QPaintEvent *e) {
    Q_UNUSED(e);
    QPainter painter(this);
    QBrush brush = window()->isActiveWindow() ? palette().base() : palette().window();
    painter.fillRect(rect(), brush);
}

void AboutView::appear() {
#ifdef APP_MAC
    mac::uncloseWindow(window()->winId());
#ifndef APP_MAC_STORE
    mac::CheckForUpdates();
#endif
#endif
}

void AboutView::screenChanged() {
    logo->setPixmap(IconUtils::pixmap(":/images/app.png"));
}
