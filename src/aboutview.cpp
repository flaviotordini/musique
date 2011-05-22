#include "aboutview.h"
#include "constants.h"

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

    QString info = "<html><style>a { color: palette(text); }</style><body><h1>" +
                   QString(Constants::APP_NAME) + "</h1>"
                   "<p>" + tr("Version %1").arg(Constants::VERSION) + "</p>"
                   + QString("<p><a href=\"%1/\">%1</a></p>").arg(Constants::WEBSITE) +

                   "<p>"
                   + tr("Report bugs and send in your ideas to %1")
                   .arg(QString("<a href=\"mailto:%1\">%1</a>").arg("flavio.tordini@gmail.com")) + "</p>"
#if !defined(APP_MAC) && !defined(Q_WS_WIN)
                   "<p>" +  tr("%1 is Free Software but its development takes precious time.").arg(Constants::APP_NAME) + "<br/>"
                   + tr("Please <a href='%1'>donate</a> to support the continued development of %2.")
                   .arg(QString(Constants::WEBSITE).append("#donate"), Constants::APP_NAME) + "</p>"
#endif

                    "<p>" + tr("Translated by the cool people at %1")
                    .arg("<a href='http://www.transifex.net/projects/p/minitunes/resource/main/'>Transifex</a>")
                    + "</p>"

#if !defined(APP_MAC) && !defined(Q_WS_WIN)
                   "<p>" + tr("Released under the <a href='%1'>GNU General Public License</a>")
                   .arg("http://www.gnu.org/licenses/gpl.html") + "</p>"
#endif

                   "<p style='color:palette(mid)'>&copy; 2010 " + Constants::ORG_NAME + "</p>"
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
