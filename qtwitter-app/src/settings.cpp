/***************************************************************************
 *   Copyright (C) 2008-2009 by Dominik Kapusta       <d@ayoy.net>         *
 *   Copyright (C) 2009 by Anna Nowak           <wiorka@gmail.com>         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License as        *
 *   published by the Free Software Foundation; either version 2.1 of      *
 *   the License, or (at your option) any later version.                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this program; if not, write to                     *
 *   the Free Software Foundation, Inc.,                                   *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/


#include <QNetworkProxy>
#include <QTranslator>
#include <QFile>
#include <QDir>
#include <QFont>
#include <QAuthenticator>
#include <QPushButton>
#include <QCheckBox>
#include <QPoint>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QDesktopServices>
#include <QProcess>
#include <QSettings>
#include <QApplication>
#include <QMessageBox>
#include <urlshortener/urlshortener.h>
#include <twitterapi/twitterapi_global.h>
#include <qticonloader.h>
#include "settings.h"
#include "qtwitter.h"
#include "configfile.h"
#include "core.h"
#include "mainwindow.h"
#include "updater.h"

extern ConfigFile settings;


Settings::Settings( MainWindow *mainwinSettings, Core *coreSettings, QWidget *parent ) :
    QDialog( parent ),
    updateAccountsOnExit( false ),
    mainWindow( mainwinSettings ),
    core( coreSettings )
{
  // Sorry, but this has to be here and not in Qtwitter::Qtwitter() for the core to be aware
  // of the signal emitted in Settings::Settings()
  connect( this, SIGNAL(createAccounts(QWidget*)), core, SLOT(createAccounts(QWidget*)) );

  ui.setupUi( this );

  QFont smallerFont = ui.shortenerInfoLabel->font();
  smallerFont.setPointSize( smallerFont.pointSize() - 1 );
  ui.shortenerInfoLabel->setFont( smallerFont );

  ui.shortenerInfoLabel->setText( tr( "Tip: use %1 to shorten links" ).arg( QKeySequence( Qt::CTRL + Qt::Key_J ).toString( QKeySequence::NativeText ) ) );

  themes.insert( Themes::STYLESHEET_COCOA.first,   Themes::STYLESHEET_COCOA.second);
  themes.insert( Themes::STYLESHEET_GRAY.first,    Themes::STYLESHEET_GRAY.second);
  themes.insert( Themes::STYLESHEET_GREEN.first,   Themes::STYLESHEET_GREEN.second);
  themes.insert( Themes::STYLESHEET_PURPLE.first,  Themes::STYLESHEET_PURPLE.second);
  themes.insert( Themes::STYLESHEET_SKY.first,     Themes::STYLESHEET_SKY.second);

  for (int i = 0; i < themes.keys().size(); ++i ) {
    ui.colorBox->addItem( themes.keys()[i] );
  }

  createLanguageMenu();
  createUrlShortenerMenu();

#ifdef Q_WS_X11
  QHBoxLayout *hlayout = new QHBoxLayout;

  useCustomBrowserCheckBox = new QCheckBox( tr( "Use custom web browser" ), ui.networkTab );
  selectBrowserEdit = new QLineEdit( ui.networkTab );
  selectBrowserButton = new QPushButton( tr( "Browse" ), ui.networkTab );

  hlayout->addWidget(selectBrowserEdit);
  hlayout->addWidget(selectBrowserButton);
  ui.verticalLayout_2->addWidget(useCustomBrowserCheckBox);
  ui.verticalLayout_2->addLayout(hlayout);

  selectBrowserEdit->setEnabled( false );
  selectBrowserButton->setEnabled( false );


  connect( useCustomBrowserCheckBox, SIGNAL(toggled(bool)), selectBrowserEdit, SLOT(setEnabled(bool)) );
  connect( useCustomBrowserCheckBox, SIGNAL(toggled(bool)), selectBrowserButton, SLOT(setEnabled(bool)) );
  connect( selectBrowserButton, SIGNAL(clicked()), this, SLOT(setBrowser()) );
#endif

  connect( ui.languageCombo, SIGNAL( currentIndexChanged( int )), this, SLOT( switchLanguage( int ) ) );
  connect( ui.colorBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(changeTheme(QString)) );
  connect( ui.checkNowButton, SIGNAL(clicked()), this, SLOT(checkForUpdate()) );
  ui.portEdit->setValidator( new QIntValidator( 1, 65535, this ) );

  emit createAccounts( ui.widget );
  loadConfig();
}

Settings::~Settings() {}

void Settings::loadConfig( bool dialogRejected )
{
  settings.beginGroup( "General" );
    ui.refreshCombo->setCurrentIndex( settings.value( "refresh-index", 3 ).toInt() );

    int lang = ui.languageCombo->findData( settings.value( "language", QLocale::system().name() ).toString() );
    if ( lang == -1 )
      lang = 0;
    ui.languageCombo->setCurrentIndex( lang );

    ui.urlShortenerCombo->setCurrentIndex( ui.urlShortenerCombo->findData( settings.value( "url-shortener", UrlShortener::SHORTENER_ISGD ).toInt() ) );
    ui.confirmDeletionBox->setChecked( settings.value( "confirmTweetDeletion", true ).toBool() );
    ui.notificationsBox->setChecked( settings.value( "notifications", true ).toBool() );
  settings.endGroup();
  settings.beginGroup( "Network" );
    settings.beginGroup( "Proxy" );
      ui.proxyBox->setChecked( settings.value( "enabled", false ).toBool() );
      ui.hostEdit->setText( settings.value( "host" ).toString() );
      ui.portEdit->setText( settings.value( "port" ).toString() );
    settings.endGroup();
    settings.beginGroup( "updates" );
      ui.updatesCheckBox->setChecked( settings.value( "check", true ).toBool() );
      ui.lastChecked->setText( settings.value( "last", tr( "never" ) ).toString() );
    settings.endGroup();
#ifdef Q_WS_X11
    useCustomBrowserCheckBox->setChecked( settings.value( "customBrowser", Qt::Unchecked ).toBool() );
    selectBrowserEdit->setText( settings.value( "browser" ).toString() );
#endif
  settings.endGroup();
  settings.beginGroup( "Appearance" );
    ui.statusCountBox->setValue( settings.value( "tweet count", 20 ).toInt() );
    ui.colorBox->setCurrentIndex( settings.value( "color scheme", 2 ).toInt() );
    int displayMode = settings.value( "display-mode", 0 ).toInt();
    switch (displayMode) {
    case StatusModel::DisplayNames:
      ui.displayNamesButton->setChecked( true );
      break;
    case StatusModel::DisplayNicks:
      ui.displayNicksButton->setChecked( true );
      break;
    case StatusModel::DisplayBoth:
      ui.displayBothButton->setChecked( true );
      break;
    }
    int trayIcon = settings.value( "tray-icon", 0 ).toInt();
    switch (trayIcon) {
    case Qtwitter::VisibleAlways:
      ui.trayIconAlwaysVisible->setChecked( true );
      break;
    case Qtwitter::VisibleWhenMinimized:
      ui.trayIconVisibleWhenMinimized->setChecked( true );
      break;
    default:;
    }
    int closeButton = settings.value( "close-button", 0 ).toInt();
    switch (closeButton) {
    case Qtwitter::CloseButtonHidesApp:
      ui.radioHide->setChecked(true);
      break;
    case Qtwitter::CloseButtonClosesApp:
      ui.radioClose->setChecked(true);
      break;
    default:;
    }
  settings.endGroup();

  ui.hostEdit->setEnabled( (bool) ui.proxyBox->checkState() );
  ui.portEdit->setEnabled( (bool) ui.proxyBox->checkState() );

  if ( !dialogRejected ) {
    settings.beginGroup( "MainWindow" );
    mainWindow->resize( settings.value( "size", QSize(350,450) ).toSize() );
    QPoint offset( settings.value( "pos", QPoint(500,200) ).toPoint() );
    if ( QApplication::desktop()->width() < offset.x() + settings.value( "size" ).toSize().width() ) {
      offset.setX( QApplication::desktop()->width() - settings.value( "size" ).toSize().width() );
    }
    if ( QApplication::desktop()->height() < offset.y() + settings.value( "size" ).toSize().height() ) {
      offset.setY( QApplication::desktop()->height() - settings.value( "size" ).toSize().height() );
    }
    mainWindow->move( offset );
    settings.endGroup();
    offset = settings.value( "SettingsWindow/pos", QPoint(350,200) ).toPoint();
    if ( QApplication::desktop()->width() < offset.x() + size().width() ) {
      offset.setX( QApplication::desktop()->width() - size().width() );
    }
    if ( QApplication::desktop()->height() < offset.y() + size().height() ) {
      offset.setY( QApplication::desktop()->height() - size().height() );
    }
    move( offset );
    applySettings();
  }
  qDebug() << "settings loaded and applied";
}

void Settings::setProxy()
{
  QNetworkProxy proxy;
  if ( ui.proxyBox->isChecked() ) {
    proxy.setType( QNetworkProxy::HttpProxy );
    proxy.setHostName( ui.hostEdit->text() );
    proxy.setPort( ui.portEdit->text().toInt() );
  } else {
    proxy.setType( QNetworkProxy::NoProxy );
  }
  QNetworkProxy::setApplicationProxy( proxy );
}

void Settings::saveConfig( int quitting )
{
  settings.beginGroup( "MainWindow" );
    settings.setValue( "size", mainWindow->size() );
    settings.setValue( "pos", mainWindow->pos() );
  settings.endGroup();
  settings.setValue( "SettingsWindow/pos", pos() );
  settings.beginGroup( "General" );
    settings.setValue( "refresh-index", ui.refreshCombo->currentIndex() );
    settings.setValue( "refresh-value", ui.refreshCombo->currentText() );
    settings.setValue( "language", ui.languageCombo->itemData( ui.languageCombo->currentIndex() ).toString() );
    settings.setValue( "url-shortener", ui.urlShortenerCombo->itemData( ui.urlShortenerCombo->currentIndex() ).toInt() );
    settings.setValue( "confirmTweetDeletion", ui.confirmDeletionBox->isChecked() );
    settings.setValue( "notifications", ui.notificationsBox->isChecked() );
  settings.endGroup();
  settings.beginGroup( "Network" );
    settings.beginGroup( "Proxy" );
      settings.setValue( "enabled", ui.proxyBox->isChecked() );
      settings.setValue( "host", ui.hostEdit->text() );
      settings.setValue( "port", ui.portEdit->text() );
    settings.endGroup();
    settings.beginGroup( "updates" );
      settings.setValue( "check", ui.updatesCheckBox->isChecked() );
      settings.setValue( "last", ui.lastChecked->text() );
    settings.endGroup();
#ifdef Q_WS_X11
    if ( useCustomBrowserCheckBox->isChecked() && selectBrowserEdit->text().isEmpty() ) {
      useCustomBrowserCheckBox->setChecked( false );
    }
    settings.setValue( "customBrowser", useCustomBrowserCheckBox->isChecked() );
    settings.setValue( "browser", selectBrowserEdit->text() );
#endif
  settings.endGroup();
  settings.beginGroup( "Appearance" );
    settings.setValue( "tweet count", ui.statusCountBox->value() );
    settings.setValue( "color scheme", ui.colorBox->currentIndex() );
    if ( ui.displayNamesButton->isChecked() ) {
      settings.setValue( "display-mode", StatusModel::DisplayNames );
    } else if ( ui.displayNicksButton->isChecked() ) {
      settings.setValue( "display-mode", StatusModel::DisplayNicks );
    } else if ( ui.displayBothButton->isChecked() ) {
      settings.setValue( "display-mode", StatusModel::DisplayBoth );
    }

    if ( ui.trayIconAlwaysVisible->isChecked() ) {
      settings.setValue( "tray-icon", Qtwitter::VisibleAlways );
    } else if ( ui.trayIconVisibleWhenMinimized->isChecked() ) {
      settings.setValue( "tray-icon", Qtwitter::VisibleWhenMinimized );
    }

    if ( ui.radioClose->isChecked() ) {
      settings.setValue( "close-button", Qtwitter::CloseButtonClosesApp );
    } else if ( ui.radioHide->isChecked() ) {
      settings.setValue( "close-button", Qtwitter::CloseButtonHidesApp );
    }

  settings.endGroup();
  settings.sync();

  if ( !quitting ) {
    applySettings();
    qDebug() << "settings applied and saved";
  }
}

void Settings::checkForUpdate()
{
  Updater *updater = new Updater( this );
  connect( updater, SIGNAL(updateChecked(bool,QString,QString)), this, SLOT(readUpdateReply(bool,QString,QString)) );
  updater->checkForUpdate();
}

void Settings::readUpdateReply( bool available, const QString &version, const QString &changes )
{
  ui.lastChecked->setText( QDateTime::currentDateTime().toString( Qt::SystemLocaleShortDate ) );
  settings.setValue( "Network/updates/last", ui.lastChecked->text() );
  QMessageBox *messageBox;
  if ( available ) {
    messageBox = new QMessageBox( QMessageBox::Information, tr( "Update available" ),
                     tr( "An update to qTwitter is available!" ),
                     QMessageBox::Close, this );
    messageBox->setInformativeText( tr( "Current version is %1.<br>Download it from %2" )
                                    .arg( version, "<a href='http://www.qt-apps.org/content/show.php/qTwitter?content=99087'>"
                                                   "qt-apps.org</a>." ) );
    messageBox->setDetailedText( changes );
  } else {
    messageBox = new QMessageBox( QMessageBox::Information, tr( "No updates available" ),
                     tr( "Sorry, no updates for qTwitter are currently available" ),
                     QMessageBox::Close, this );
  }
  messageBox->setButtonText( QMessageBox::Close, tr( "Close" ) );
  messageBox->exec();
  messageBox->deleteLater();
  sender()->deleteLater();
}

void Settings::show()
{
  updateAccountsOnExit = true;
  core->setSettingsOpen( true );
  ui.tabs->setCurrentIndex( 0 );
  ui.lastChecked->setText( settings.value( "Network/updates/last" ).toString() );
  QDialog::show();
  adjustSize();
}

void Settings::accept()
{
  saveConfig( !updateAccountsOnExit );

  core->setSettingsOpen( false );
  QDialog::accept();
}

void Settings::reject()
{
  loadConfig( true );

  core->setSettingsOpen( false );
  QDialog::reject();
}

void Settings::changeTheme( const QString &theme )
{
  mainWindow->changeListBackgroundColor( themes.value( theme ).listBackgroundColor );
  core->setModelTheme( themes.value( theme ) );
}

void Settings::changeEvent( QEvent *e )
{
  switch (e->type()) {
  case QEvent::LanguageChange:
    ui.retranslateUi(this);
    break;
  default:;
  }
}

void Settings::retranslateUi()
{
  ui.languageCombo->setItemText( 0, tr( "Default" ) );
  ui.shortenerInfoLabel->setText( tr( "Tip: use %1 to shorten links" ).arg( QKeySequence( Qt::CTRL + Qt::Key_J ).toString( QKeySequence::NativeText ) ) );
#ifdef Q_WS_X11
  useCustomBrowserCheckBox->setText( tr( "Use custom web browser" ) );
  selectBrowserButton->setText( tr( "Browse" ) );
#endif
  ui.buttonBox->clear();
  ui.buttonBox->addButton("OK", QDialogButtonBox::AcceptRole)->setText( tr( "OK" ) );
  QPushButton *applyButton = ui.buttonBox->addButton("Apply", QDialogButtonBox::ApplyRole);
  applyButton->setText( tr( "Apply" ) );
  connect( applyButton, SIGNAL(clicked()), this, SLOT(saveConfig()) );
  ui.buttonBox->addButton("Cancel", QDialogButtonBox::RejectRole)->setText( tr( "Cancel" ) );
  update();
}

#ifdef Q_WS_X11
void Settings::setBrowser()
{
  QRegExp rx( ";HOME=(.+);", Qt::CaseSensitive );
  rx.setMinimal( true );
  rx.indexIn( QProcess::systemEnvironment().join( ";" ) );
  selectBrowserEdit->setText( QFileDialog::getOpenFileName( this, tr( "Select your browser executable" ), rx.cap( 1 ), tr( "All files (*)") ) );
}
#endif

void Settings::applySettings()
{
  setProxy();
  core->applySettings();
  if ( ui.displayNamesButton->isChecked() ) {
    StatusModel::instance()->setDisplayMode( StatusModel::DisplayNames );
  } else if ( ui.displayNicksButton->isChecked() ) {
    StatusModel::instance()->setDisplayMode( StatusModel::DisplayNicks );
  } else if ( ui.displayBothButton->isChecked() ) {
    StatusModel::instance()->setDisplayMode( StatusModel::DisplayBoth );
  }
  changeTheme( ui.colorBox->currentText() );
  if ( Qtwitter::instance() ) {
    if ( ui.trayIconAlwaysVisible->isChecked() ) {
      Qtwitter::instance()->setTrayIconMode( Qtwitter::VisibleAlways );
    } else if ( ui.trayIconVisibleWhenMinimized->isChecked() ) {
      Qtwitter::instance()->setTrayIconMode( Qtwitter::VisibleWhenMinimized );
    }
    if ( ui.radioClose->isChecked() ) {
      Qtwitter::instance()->setCloseButtonMode( Qtwitter::CloseButtonClosesApp );
    } else if ( ui.radioHide->isChecked() ) {
      Qtwitter::instance()->setCloseButtonMode( Qtwitter::CloseButtonHidesApp );
    }
  }
#ifdef Q_WS_X11
  if ( useCustomBrowserCheckBox->isChecked() ) {
    QDesktopServices::setUrlHandler( "http", core, "openBrowser");
    QDesktopServices::setUrlHandler( "https", core, "openBrowser");
    QDesktopServices::setUrlHandler( "ftp", core, "openBrowser");
    core->setBrowserPath( selectBrowserEdit->text() );
  } else {
    QDesktopServices::unsetUrlHandler( "http" );
    QDesktopServices::unsetUrlHandler( "https" );
    QDesktopServices::unsetUrlHandler( "ftp" );
    core->setBrowserPath( QString() );
  }
#endif
}

void Settings::createLanguageMenu()
{
#if defined Q_WS_X11
  QDir qmDir( SHARE_DIR );
#else
  QDir qmDir( ":" );
#endif
  if ( !qmDir.cd( "loc" ) ) {
    qmDir.cd( QApplication::applicationDirPath() );
    qmDir.cd( "qtwitter-app/res/loc" );
  }
  QStringList fileNames = qmDir.entryList(QStringList("qtwitter_*.qm"));
  fileNames.append( "qtwitter_en.qm" );
  fileNames.sort();
  for (int i = 0; i < fileNames.size(); ++i) {
    QString locale = fileNames[i];
    locale.remove(0, locale.indexOf('_') + 1);
    locale.chop(3);

    translator.load(fileNames[i], qmDir.absolutePath());
    //: Please put here your translation's language, e.g. "Deutsch", "Francais", "Suomi", etc.
    //: DON'T TRANSLATE "English" TO YOUR LANGUAGE
    QString language = translator.translate( "Settings", "English" );
    if ( language.isEmpty() )
      language = "English";
    ui.languageCombo->addItem( language, locale );
  }
  QString systemLocale = QLocale::system().name();
  ui.languageCombo->insertItem(0, tr( "Default" ), systemLocale );
//  systemLocale.chop(3);
  qDebug() << systemLocale << ui.languageCombo->findData( systemLocale );
//  ui.languageCombo->setCurrentIndex( ui.languageCombo->findData( systemLocale ) );
}

void Settings::switchLanguage( int index )
{
#if defined Q_WS_X11
  QDir qmDir( SHARE_DIR );
#else
  QDir qmDir( ":" );
#endif
  if ( !qmDir.cd( "loc" ) ) {
    qmDir.cd( QApplication::applicationDirPath() );
    qmDir.cd( "qtwitter-app/res/loc" );
  }
  QString qmPath( qmDir.absolutePath() );

  QString locale = ui.languageCombo->itemData( index ).toString();

  qDebug() << "switching language to" << locale << "from" << qmPath;
  translator.load( "qtwitter_" + locale, qmPath );
  qApp->installTranslator( &translator );

  retranslateUi();
  mainWindow->retranslateUi();
  core->retranslateUi();
  adjustSize();
}

void Settings::createUrlShortenerMenu()
{
  ui.urlShortenerCombo->addItem( "bit.ly", UrlShortener::SHORTENER_BITLY );
  ui.urlShortenerCombo->addItem( "Boooom!", UrlShortener::SHORTENER_BOOOOM );
  ui.urlShortenerCombo->addItem( "Digg", UrlShortener::SHORTENER_DIGG );
  ui.urlShortenerCombo->addItem( "is.gd", UrlShortener::SHORTENER_ISGD );
  ui.urlShortenerCombo->addItem( "MetaMark", UrlShortener::SHORTENER_METAMARK );
  ui.urlShortenerCombo->addItem( "Migre.me", UrlShortener::SHORTENER_MIGREME);
  ui.urlShortenerCombo->addItem( "tinyarro.ws", UrlShortener::SHORTENER_TINYARROWS );
  ui.urlShortenerCombo->addItem( "TinyURL", UrlShortener::SHORTENER_TINYURL );
  ui.urlShortenerCombo->addItem( "tr.im", UrlShortener::SHORTENER_TRIM );
  ui.urlShortenerCombo->addItem( "u.nu", UrlShortener::SHORTENER_UNU );
}
