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
#ifdef APP_ACTIVATION
#include "activation.h"
#endif
#ifdef APP_MAC
#include "macutils.h"
#include "mac_startup.h"
#endif

AboutView::AboutView(QWidget *parent) : QWidget(parent) {
    
    QBoxLayout *aboutlayout = new QHBoxLayout(this);
    aboutlayout->setAlignment(Qt::AlignCenter);
    aboutlayout->setMargin(30);
    aboutlayout->setSpacing(30);
    
    QLabel *logo = new QLabel(this);
    logo->setPixmap(QPixmap(":/images/app.png"));
    aboutlayout->addWidget(logo, 0, Qt::AlignTop);
    
    QBoxLayout *layout = new QVBoxLayout();
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(30);
    aboutlayout->addLayout(layout);

    QString info = "<html><style>a { color: palette(text); text-decoration: none; font-weight: bold }</style><body><h1>" +
            QString(Constants::NAME) + "</h1>"
            "<p>" + tr("Version %1").arg(Constants::VERSION) + "</p>"
            + QString("<p><a href=\"%1/\">%1</a></p>").arg(Constants::WEBSITE);

        #if !defined(APP_MAC) && !defined(Q_WS_WIN)
            info += "<p>" +  tr("%1 is Free Software but its development takes precious time.").arg(Constants::NAME) + "<br/>"
            + tr("Please <a href='%1'>donate</a> to support the continued development of %2.")
            .arg(QString(Constants::WEBSITE).append("#donate"), Constants::NAME) + "</p>";
        #endif

        #ifdef APP_ACTIVATION
            if (Activation::instance().isActivated())
                info += "<p>" + tr("Licensed to: %1").arg("<b>" + Activation::instance().getEmail() + "</b>");
        #endif

            info += "<p>" + tr("You may want to try my other apps as well:") + "</p>"
            + "<ul>"

            + "<li>" + tr("%1, a YouTube app")
            .arg("<a href='http://flavio.tordini.org/minitube'>Minitube</a>")
            + "</li>"

            + "<li>" + tr("%1, a YouTube music player")
            .arg("<a href='http://flavio.tordini.org/musictube'>Musictube</a>")
            + "</li>"

            + "</ul>"

            "<p>" + tr("Translate %1 to your native language using %2").arg(Constants::NAME)
            .arg("<a href='http://www.transifex.net/projects/p/" + QLatin1String(Constants::UNIX_NAME) + "/'>Transifex</a>")
            + "</p>"

        #if !defined(APP_MAC) && !defined(Q_WS_WIN)
            "<p>" + tr("Released under the <a href='%1'>GNU General Public License</a>")
            .arg("http://www.gnu.org/licenses/gpl.html") + "</p>"
        #endif

            "<p style='color:palette(mid)'>&copy; 2010-2013 " + Constants::ORG_NAME + "</p>"
            "</body></html>";
    QLabel *infoLabel = new QLabel(info, this);
    infoLabel->setOpenExternalLinks(true);
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);
    
    QLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignLeft);
    QPushButton *closeButton = new QPushButton(tr("&Close"), this);
    closeButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    
    closeButton->setDefault(true);
    closeButton->setFocus(Qt::OtherFocusReason);
    connect(closeButton, SIGNAL(clicked()), parent, SLOT(goBack()));
    buttonLayout->addWidget(closeButton);
    
    layout->addLayout(buttonLayout);
    
}

void AboutView::paintEvent(QPaintEvent * /*event*/) {
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

void AboutView::appear() {
#ifdef APP_MAC
    mac::uncloseWindow(window()->winId());
#ifdef APP_ACTIVATION
    mac::CheckForUpdates();
#endif
#endif
}
