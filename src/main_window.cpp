 /****************************************************************************
|
| Copyright (c) 2011 Novell, Inc.
| All Rights Reserved.
|
| This program is free software; you can redistribute it and/or
| modify it under the terms of version 2 of the GNU General Public License as
| published by the Free Software Foundation.
|
| This program is distributed in the hope that it will be useful,
| but WITHOUT ANY WARRANTY; without even the implied warranty of
| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
| GNU General Public License for more details.
|
| You should have received a copy of the GNU General Public License
| along with this program; if not, contact Novell, Inc.
|
| To contact Novell about this file by physical or electronic mail,
| you may find current contact information at www.novell.com
|
 \***************************************************************************/

#include "main_window.h"
#include "i18n.h"

#include <unistd.h>
#include <sys/param.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>

#include <QApplication>
#include <QLayout>
#include <QLabel>
#include <QToolBar>
#include <QLineEdit>
#include <QDockWidget>
#include <QListView>
#include <QDebug>
#include <QQueue>
#include <QSettings>
#include <QStatusBar>
#include <QTimer>
#include <QMessageBox>

#include "kcategorizedsortfilterproxymodel.h"
#include "kcategorizedview.h"
#include "kcategorydrawer.h"
#include "yqmodulesproxymodel.h"

#include "yqmodulesmodel.h"
#include "yqmodulegroupsmodel.h"
#include "listview.h"

//#include "moduleiconitem.h"
#define ORG_NAME "YaST2"
#define APP_NAME "y2controlcenter-qt"
#define USED_QUEUE_SIZE 5

#define GROUPSIZE QSize(200,350)

QJsonObject ansible_object;

/*
  Textdomain "control-center"
*/

class MainWindow::Private
{
public:
    Private()
        : modmodel(0L)
        , groupview(0L)
        , modview(0L)
        , kcsfpm(0L)
	, gcsfpm(0L)
	, lastCallTime(0)
	, minwait(5)
    {
    }
    ~Private()
     {
     }

    YQModulesModel *modmodel;
    ListView *groupview;
    KCategorizedView * modview;
    // category proxy model
    KCategorizedSortFilterProxyModel * kcsfpm;
    QSortFilterProxyModel *gcsfpm;

    QLineEdit *searchField;

    QQueue <QString>  recentlyUsed;

    QModelIndex lastCalledIndex;
    time_t lastCallTime;
    time_t minwait;

    bool noBorder;
    bool fullScreen;

};

MainWindow::MainWindow( Qt::WindowFlags wflags )
  : QMainWindow( 0, wflags )
  , d(new Private)
{
    qDebug();

    // Central widget
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QHBoxLayout *layout = new QHBoxLayout;
    centralWidget->setLayout(layout);

    // setup central widget
    d->modview = new KCategorizedView( this );
    KCategoryDrawer * drawer = new KCategoryDrawer;

    layout->addWidget(d->modview);

    d->modview->setSelectionMode(QAbstractItemView::SingleSelection);
//        tv->setSpacing(KDialog::spacingHint());
    d->modview->setCategoryDrawer( drawer );
    d->modview->setViewMode( QListView::IconMode );
    d->modview->setItemDelegate( new ModuleIconItemDelegate( this , d->modview->style() ) );
    d->modview->setMouseTracking( true );
    d->modview->viewport()->setAttribute( Qt::WA_Hover );

    // init the models
    d->modmodel = new YQModulesModel(this);
    d->kcsfpm = new YQModulesProxyModel( this );
    d->kcsfpm->setCategorizedModel( true );
    d->kcsfpm->setSourceModel( d->modmodel) ;

    //kcsfpm->setFilterRole( KCModuleModel::UserFilterRole );
    //kcsfpm->setFilterCaseSensitivity( Qt::CaseInsensitive );
    d->kcsfpm->sort( 0 );
    d->modview->setModel( d->kcsfpm );

    // Setup Dock Widget with groups and search field
    QDockWidget *groupdock = new QDockWidget(this);
    groupdock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    QWidget *leftPanel = new QWidget( this );
    QVBoxLayout *leftPanelLayout = new QVBoxLayout( leftPanel );

    QHBoxLayout *searchLayout = new QHBoxLayout();
    QLabel *searchLabel = new QLabel();
    d->searchField = new QLineEdit();
    searchLabel->setText( _("&Search") );
    searchLabel->setBuddy( d->searchField );
    searchLayout->addWidget(searchLabel);
    searchLayout->addWidget(d->searchField);

    leftPanelLayout->addLayout( searchLayout );

    d->gcsfpm = new QSortFilterProxyModel( this );
    d->gcsfpm->setSourceModel( d->modmodel->groupsModel() );
    d->gcsfpm->setFilterKeyColumn( 3 );
    d->gcsfpm->setFilterRole( Qt::UserRole );

    d->groupview = new ListView(  );

    d->groupview->setModel(d->gcsfpm);
    d->groupview->setIconSize( QSize(32,32) );
    d->groupview->setSizeHint( readGroupViewSize() );

    //now pre-select something
    d->groupview->setSelectionMode( QAbstractItemView::SingleSelection );
    QModelIndex selection = d->groupview->model()->index(0,0);
    d->groupview->setCurrentIndex( selection );
    d->modview->selectCategory( d->modmodel->groupsModel()->data(selection).toString() );

    leftPanelLayout->addWidget( d->groupview );

    groupdock->setWidget( leftPanel );

    addDockWidget(Qt::LeftDockWidgetArea, groupdock);

    // Setup Dock Widget with system information
    QStringList arguments;
    arguments << "localhost" << "-m" << "setup";
    QString program_stdout = runAnsible(arguments);
    ansible_object = parseAnsible(program_stdout);


    QString distribution = ansible_object["ansible_distribution"].toString();
    QString distribution_version = ansible_object["ansible_distribution_version"].toString();
    const QString distroNameVersion = QStringLiteral("%1 %2").arg(distribution, distribution_version);

    QString product_name = ansible_object["ansible_product_name"].toString();
    if (product_name.compare("To be filled by O.E.M.") == 0)
    {
        product_name = ansible_object["ansible_board_name"].toString();
    }
    QString architecture = ansible_object["ansible_architecture"].toString();
    QString bios_date = ansible_object["ansible_bios_date"].toString();
    QString bios_version = ansible_object["ansible_bios_version"].toString();
    QJsonObject default_nic_object = ansible_object["ansible_default_ipv4"].toObject();
    /* Example of default IP4 output from ansible:
    "ansible_default_ipv4": {
            "address": "192.168.1.171",
            "alias": "br0",
            "broadcast": "192.168.1.255",
            "gateway": "192.168.1.1",
            "interface": "br0",
            "macaddress": "90:2b:34:03:75:62",
            "mtu": 1500,
            "netmask": "255.255.255.0",
            "network": "192.168.1.0",
            "type": "bridge"
        },
     */

    QJsonObject default_dns_object = ansible_object["ansible_dns"].toObject();
    /* Example of dns output from ansible
        "ansible_dns": {
            "nameservers": [
                "192.168.1.254",
                "8.8.8.8"
            ],
            "search": [
                "bunker.home"
            ]
        }
     */

    QString default_nic = "hostname: " +  ansible_object["ansible_hostname"].toString() + "\n" +
                          "address: " + default_nic_object["address"].toString() + "\n" +
                          "interface: " + default_nic_object["interface"].toString() + "\n" +
                          "gateway: " + default_nic_object["gateway"].toString() + "\n" +
                          "MAC: " + default_nic_object["macaddress"].toString() + "\n" +
                          "netmask: " + default_nic_object["netmask"].toString() + "\n" +
                          QStringLiteral("MTU: %1").arg(default_nic_object["mtu"].toInt()) + "\n";

    //std::cout << qPrintable() << std::endl;
    QString dns_servers = "";
    foreach(QJsonValue i, default_dns_object["nameservers"].toArray())
    {
        dns_servers.append(i.toString() + ", ");
    }
    QString domains = "";
    foreach(QJsonValue i, default_dns_object["search"].toArray())
    {
        domains.append(i.toString() + ", ");
    }
    QString default_dns = "DNS: " + dns_servers + "\n" +
                          "Domain: " + domains;
    // read group desktop files
    QStringList single_command_args;
    single_command_args << "localhost" << "-m" << "command" << "-a" << "cat /etc/xdg/kcm-about-distrorc";
    QString about_distrorc = runAnsible(single_command_args);
    QString logoPath = "";
    QString webSite = "";
    if ( about_distrorc.contains("rc=0") )
    {
        QStringList kde_distrorc = about_distrorc.split("\n");
        logoPath = kde_distrorc[kde_distrorc.indexOf(QRegExp("^LogoPath=.*$"))].split("=")[1];
        webSite = kde_distrorc[kde_distrorc.indexOf(QRegExp("^Website=.*$"))].split("=")[1];
    }

    //std::cout << qPrintable(logoPath) << std::endl;

    QDockWidget *sysinfodock = new QDockWidget(this);
    sysinfodock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    QWidget *rightPanel = new QWidget( this );
    QVBoxLayout *rightPanelLayout = new QVBoxLayout( rightPanel );
    QVBoxLayout *systemLayout = new QVBoxLayout();
    // from https://github.com/KDE/kinfocenter/blob/master/Modules/about-distro/src/Module.cpp
    QPixmap logo;
    logo = QPixmap(logoPath);
    QLabel *logoLabel = new QLabel();
    logoLabel->setPixmap(logo);
    logoLabel->setAlignment(Qt::AlignCenter);
    QLabel *nameVersionLabel = new QLabel();

    QFont font;
    font.setPixelSize(24);
    font.setBold(true);
    nameVersionLabel->setFont(font);
    nameVersionLabel->setText(distroNameVersion);



    QLabel *urlLabel = new QLabel();
    urlLabel->setText("Distro Site: <a href='" + webSite + "'>" + webSite + "</a>");
    urlLabel->setOpenExternalLinks(true);

    QLabel *systemLabel = new QLabel();
    systemLabel->setText("Board Name: " + product_name + "\n" +
                         "Bios Date: " + bios_date + "\n" +
                         "Bios version: " + bios_version);
    QLabel *networkLabel = new QLabel();
    networkLabel->setText("Default Network Setting:\n" + default_nic + "\n" + default_dns);



    systemLayout->addWidget(nameVersionLabel);
    systemLayout->addWidget(urlLabel);
    systemLayout->addWidget(systemLabel);
    systemLayout->addWidget(networkLabel);

    rightPanelLayout->addWidget(logoLabel);
    rightPanelLayout->addLayout(systemLayout);
    rightPanelLayout->setAlignment(Qt::AlignVCenter);

    sysinfodock->setWidget( rightPanel );
    addDockWidget(Qt::RightDockWidgetArea, sysinfodock);

    readSettings();

    initActions();
    addAction( shutdown );
    addAction( saveLogs );
    //d->modview->addAction( addToF );
    //d->modview->setContextMenuPolicy( Qt::ActionsContextMenu );

    logSaver = new YQSaveLogs();


    setWinTitle();
    statusBar()->showMessage( _("Ready") );



    connect( d->groupview, SIGNAL( clicked( const QModelIndex & ) ),
             SLOT( slotGroupPressed( const QModelIndex & ) ) );

    connect( d->modview, SIGNAL( leftMouseClick ( const QModelIndex  &) ),
             SLOT( slotLaunchModule( const QModelIndex & ) ) );

    connect( d->modview, SIGNAL( activated( const QModelIndex & ) ),
             SLOT( slotLaunchModule( const QModelIndex & ) ) );

    connect( d->searchField, SIGNAL( textChanged( const QString &)),
	     SLOT( slotFilterChanged() ));

    connect( shutdown, &QAction::triggered, qApp, &QApplication::quit );

    connect( saveLogs, &QAction::triggered, logSaver, &YQSaveLogs::save );

    connect( logSaver, SIGNAL( statusMsg( const QString &)), statusBar(),
	     SLOT( showMessage( const QString &) ));


//    d->groupview->setSizeHint(500,500);
}

void MainWindow::setFullScreen( bool fs )
{
    d->fullScreen = fs;
}

void MainWindow::setNoBorder( bool nb )
{
    d->noBorder = nb ;
}

void MainWindow::initActions()
{
   //addToF = new QAction( "Add to Favourites", this );
   shutdown = new QAction( this );
   shutdown->setShortcut( Qt::CTRL + Qt::Key_Q );

   saveLogs = new QAction( this );
   saveLogs->setShortcut( Qt::SHIFT + Qt::Key_F8);
}

void MainWindow::slotGroupPressed( const QModelIndex &index )
{
    qDebug() << "Group Click";
    // scroll to the category in the main view
    QModelIndex modidx = d->modmodel->firstModuleInGroup(index);
    qDebug() << "Scroll to  :" << d->modmodel->groupsModel()->data(index).toString();
    qDebug() << "  1st item :" << d->modmodel->data(modidx).toString();
    if ( modidx.isValid() )
    {
	d->modview->selectCategory( d->modmodel->groupsModel()->data(index).toString() );
        d->modview->scrollTo(d->kcsfpm->mapFromSource(modidx));
    }
}

void MainWindow::slotModulePressed( const QModelIndex &index )
{
    // map the categorized index to the modules model index
    QModelIndex srcidx = d->kcsfpm->mapToSource(index);
    if ( ! srcidx.isValid() )
        return;

    qDebug() << "Module Click:" << d->modmodel->data(srcidx).toString();
    qDebug() << "-> " << srcidx.row() << " : " << d->modmodel->propertyValue(srcidx, "GenericName").toString();
}

void MainWindow::slotLaunchModule( const QModelIndex &index)
{
    if ( index == d->lastCalledIndex )
    {
            time_t now = time( NULL );

            // ignore accidential double clicks

            if ( now >= d->lastCallTime &&                 // make sure we not going backward in time
                                                        // (system time or time zone changed by
                                                        // some YaST2 module - see bug #71816)
                 now - d->lastCallTime < d->minwait )         // elapsed time too short?
            {
		qDebug() << "Ignoring 2nd click, clicking too fast" ;
                return; // ignore this click
            }
    }

    d->lastCallTime     = time( NULL );
    d->lastCalledIndex = index;

    QModelIndex i1 = d->modmodel->index( d->kcsfpm->mapToSource( index ).row(), YQDesktopFilesModel::Call );
    QModelIndex i2 = d->modmodel->index( d->kcsfpm->mapToSource( index ).row(), YQDesktopFilesModel::Argument );
    QModelIndex i3 = d->modmodel->index( d->kcsfpm->mapToSource( index ).row(), YQDesktopFilesModel::Name );

    if ( !i1.isValid() || !i2.isValid())
	return;

    QString client = d->modmodel->data( i1, Qt::UserRole ).toString();
    QString argument = d->modmodel->data( i2, Qt::UserRole ).toString();
    qDebug() << argument;
    QString name = d->modmodel->data( i3, Qt::UserRole ).toString();

    QString cmd = QString("/sbin/yast2 ");
    cmd += client;

    if ( d->noBorder )
        cmd += " --noborder ";
    if ( d->fullScreen )
        cmd += " --fullscreen ";

    if (!argument.isEmpty() )
    {
        cmd +=" '";
        cmd += argument;
        cmd +="'";
    }
    cmd += " &";

    //FIXME: use something more intelligent (unique) to remember used modules, names suck
    d->recentlyUsed.enqueue( name );
    if( d->recentlyUsed.size() == USED_QUEUE_SIZE )
    {
	d->recentlyUsed.dequeue();
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);

    qDebug() << "Run command: " << cmd.toUtf8();
    std::cout << "Run command: " << qPrintable(cmd) << std::endl;
    //Translators: module name comes here (%1) e.g. HTTP server, Scanner,...
    QString msg = _("Starting configuration module \"%1\"...").arg( name );
    statusBar()->showMessage( msg, 2000 );

    system( cmd.toUtf8() );

    QTimer::singleShot( 3*1000, this, SLOT( slotRestoreCursor() ) );
}

void MainWindow::slotFilterChanged()
{
    QString stext = d->searchField->text();
    d->kcsfpm->customFilterFunction( stext );

    QString gr_filter = d->kcsfpm->matchingGroupFilterRegexp();
    d->gcsfpm->setFilterRegExp ( gr_filter );
}

void MainWindow::initialMsg()
{
    if ( !d->modmodel->isRoot() )
	QMessageBox::information(this, _("YaST Control Center"),
            _("YaST Control Center is not running as root.\n"
	    "You will only see modules which do not require root privileges."));
}

void MainWindow::slotRestoreCursor()
{
    QApplication::restoreOverrideCursor();
}

void MainWindow::readSettings()
{
    QSettings settings(ORG_NAME, APP_NAME);

    settings.beginGroup("MainWindow");
    resize(settings.value("Size", QSize(680,420)).toSize());
    move(settings.value("Position", QPoint(200,200)).toPoint());
    settings.endGroup();

    settings.beginGroup("PersonalItems");
    QString used =  settings.value("RecentlyUsed").toString();
    QStringList used_list = used.split(",", QString::SkipEmptyParts);
    foreach( QString f, used_list )
    {
       d->recentlyUsed.enqueue( f );
    }
    settings.endGroup();

}

QSize MainWindow::readGroupViewSize()
{
    QSettings settings(ORG_NAME, APP_NAME);
    QSize size;

    settings.beginGroup("GroupView");
    size = settings.value("Size", GROUPSIZE).toSize();
    settings.endGroup();

    return size;
}


void MainWindow::writeSettings()
{
    QSettings settings(ORG_NAME, APP_NAME);

    settings.beginGroup("MainWindow");
    settings.setValue("Size", size());
    settings.setValue("Position", pos());
    settings.endGroup();

    settings.beginGroup( "PersonalItems" );
    QStringList used_list( d->recentlyUsed );
    settings.setValue( "RecentlyUsed", used_list.join(",") );
    settings.endGroup();

    settings.beginGroup("GroupView");
    settings.setValue("Size", d->groupview->size());
    settings.endGroup();

}
/*void MainWindow::setWinTitle()
{
    QString title = _("YaST Control Center");
    char hostname[ MAXHOSTNAMELEN+1 ];
    if ( gethostname( hostname, sizeof( hostname )-1 ) == 0 )
    {
    hostname[ sizeof( hostname ) -1 ] = '\0'; // make sure it's terminated

    if ( strlen( hostname ) > 0 && strcmp( hostname, "(none)" ) != 0 )
    {
        title += " @ ";
        title += hostname;
    }
    }
    setWindowTitle( title );
    QCoreApplication::setApplicationName( title );
}
*/
void MainWindow::setWinTitle()
{
    QString title = _("YaST Control Center");
    title += " @ " + ansible_object["ansible_hostname"].toString();
    setWindowTitle( title );
    QCoreApplication::setApplicationName( title );
}

QString MainWindow::runAnsible(QStringList arguments)
{
    //TODO: find ansible command
    QString program = "/usr/bin/ansible";
    //Run ansible Ad-Hoc command and store output
    QProcess runAnsible;
    runAnsible.start(program, arguments);
    runAnsible.waitForFinished();
    //read result and trim top line(non JSON)
    QString program_stdout = runAnsible.readAll();
    //TODO check if command is successful.
    return program_stdout.remove("localhost | SUCCESS =>");
}

QJsonObject MainWindow::parseAnsible(QString json_text)
{
    //Convert stdout to JSON
    QJsonParseError jerror;
    QJsonDocument facts = QJsonDocument::fromJson(json_text.toLatin1(), &jerror);
    if (jerror.error != QJsonParseError::NoError)
    {
        qDebug() << "Error happened:" << jerror.errorString();
    }
    //Get object from Document & Unpack main ansible_facts section
    QJsonObject facts_object = facts.object()["ansible_facts"].toObject();
    return facts_object;
}

void MainWindow::closeEvent (QCloseEvent *event)
{
    writeSettings();
    event->accept();
}

MainWindow::~MainWindow()
{
    delete d;
    delete logSaver;
}

#include "main_window.moc"
