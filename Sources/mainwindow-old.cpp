/* todo
 *   compatflags
 *
 *   import options
 *
 *   static linking, licence
 *   drag&drop
 */

//====================================================================================================

#include "mainwindow-old.hpp"
#include "ui_mainwindow.h"
#include "DMFlagsDialog.hpp"
#include "CompatFlagsDialog.hpp"
#include "Utils.hpp"

#include <cstdio>

#include <QDebug>
#include <QString>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QMessageBox>
#include <QListWidgetItem>
#include <QHash>
#include <QSet>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

#include <QStringBuilder>
#define QT_USE_QSTRINGBUILDER


//====================================================================================================

// !!todo!! MAC
#ifdef _WIN32
    const char * SCRIPT_EXT = "*.bat";
#else
    const char * SCRIPT_EXT = "*.sh";
#endif

const char * WINDOW_PROPS_FILENAME = "window.dml";
const char * CURRENT_CONFIG_FILENAME = "SavedOptions/current.json";


//====================================================================================================

MainWindow::MainWindow( QWidget * parent )

    : QMainWindow( parent )
    , ui( new Ui::MainWindow )
    , currentDir( QDir::current() )
    , dmflags1(0)
    , dmflags2(0)
    , compatflags1(0)
    , compatflags2(0)
{
    ui->setupUi( this );

    connect( ui->addPort_btn, SIGNAL( clicked() ), this, SLOT( addSourcePort() ) );
    connect( ui->delPort_btn, SIGNAL( clicked() ), this, SLOT( delSourcePort() ) );
    connect( ui->sourcePort_cmbbox, SIGNAL( currentTextChanged(QString) ), this, SLOT( selectSourcePort(QString) ) );

    connect( ui->autoUpdIWADs_chkbox, SIGNAL( toggled(bool) ), this, SLOT( toggleUpdateIWADs(bool) ) );
    connect( ui->IWADsDir_line, SIGNAL( textChanged(QString) ), this, SLOT( changeUpdateIWADsDir(QString) ) );
    connect( ui->browseIWADs_btn, SIGNAL( clicked() ), this, SLOT( browseIWADs() ) );
    connect( ui->addIWAD_btn, SIGNAL( clicked() ), this, SLOT( addIWAD() ) );
    connect( ui->delIWAD_btn, SIGNAL( clicked() ), this, SLOT( delIWAD() ) );
    connect( ui->upIWAD_btn, SIGNAL( clicked() ), this, SLOT( upIWAD() ) );
    connect( ui->downIWAD_btn, SIGNAL( clicked() ), this, SLOT( downIWAD() ) );
    connect( ui->IWADs_list, SIGNAL( currentTextChanged(QString) ), this, SLOT( selectIWAD(QString) ) );

    connect( ui->autoUpdPWADs_chkbox, SIGNAL( toggled(bool) ), this, SLOT( toggleUpdatePWADs(bool) ) );
    connect( ui->PWADsDir_line, SIGNAL( textChanged(QString) ), this, SLOT( changeUpdatePWADsDir(QString) ) );
    connect( ui->browsePWADs_btn, SIGNAL( clicked() ), this, SLOT( browsePWADs() ) );
    connect( ui->addPWAD_btn, SIGNAL( clicked() ), this, SLOT( addPWAD() ) );
    connect( ui->delPWAD_btn, SIGNAL( clicked() ), this, SLOT( delPWAD() ) );
    connect( ui->upPWAD_btn, SIGNAL( clicked() ), this, SLOT( upPWAD() ) );
    connect( ui->downPWAD_btn, SIGNAL( clicked() ), this, SLOT( downPWAD() ) );
    connect( ui->PWADs_list, SIGNAL( itemChanged(QListWidgetItem*) ), this, SLOT( selectPWAD(QListWidgetItem*) ) );

    connect( ui->autoUpdMods_chkbox, SIGNAL( toggled(bool) ), this, SLOT( toggleUpdateMods(bool) ) );
    connect( ui->modsDir_line, SIGNAL( textChanged(QString) ), this, SLOT( changeUpdateModsDir(QString) ) );
    connect( ui->browseMods_btn, SIGNAL( clicked() ), this, SLOT( browseMods() ) );
    connect( ui->addMod_btn, SIGNAL( clicked() ), this, SLOT( addMod() ) );
    connect( ui->delMod_btn, SIGNAL( clicked() ), this, SLOT( delMod() ) );
    connect( ui->upMod_btn, SIGNAL( clicked() ), this, SLOT( upMod() ) );
    connect( ui->downMod_btn, SIGNAL( clicked() ), this, SLOT( downMod() ) );
    connect( ui->mods_list, SIGNAL( itemChanged(QListWidgetItem*) ), this, SLOT( selectMod(QListWidgetItem*) ) );

    connect( ui->multRole_cmbbox, SIGNAL( currentIndexChanged(int) ), this, SLOT( selectMultRole(int) ) );
    connect( ui->playerCnt_spinbox, SIGNAL( valueChanged(int) ), this, SLOT( changePlayerCount(int) ) );
    connect( ui->gameMode_cmbbox, SIGNAL( currentIndexChanged(int) ), this, SLOT( selectGameMode(int) ) );
    connect( ui->teamDmg_spinbox, SIGNAL( valueChanged(double) ), this, SLOT( changeTeamDamage(double) ) );
    connect( ui->timeLimit_spinbox, SIGNAL( valueChanged(int) ), this, SLOT( changeTimeLimit(int) ) );
    connect( ui->netMode_cmbbox, SIGNAL( currentIndexChanged(int) ), this, SLOT( selectNetMode(int) ) );
    connect( ui->IPAddress_line, SIGNAL( textChanged(QString) ), this, SLOT( changeIP(QString) ) );
    connect( ui->port_spinbox, SIGNAL( valueChanged(int) ), this, SLOT( changePort(int) ) );

    connect( ui->directStart_chkbox, SIGNAL( toggled(bool) ), this, SLOT( toggleDirectStart(bool) ) );
    connect( ui->loadGame_chkbox, SIGNAL( toggled(bool) ), this, SLOT( toggleLoadGame(bool) ) );
    connect( ui->map_cmbbox, SIGNAL( currentTextChanged(QString) ), this, SLOT( changeMap(QString) ) );
    connect( ui->skill_cmbbox, SIGNAL( currentIndexChanged(int) ), this, SLOT( changeSkill(int) ) );
    connect( ui->skill_spinbox, SIGNAL( valueChanged(int) ), this, SLOT( changeSkillNum(int) ) );
    connect( ui->noMonsters_chkbox, SIGNAL( toggled(bool) ), this, SLOT( toggleNoMonsters(bool) ) );
    connect( ui->fastMonsters_chkbox, SIGNAL( toggled(bool) ), this, SLOT( toggleFastMonsters(bool) ) );
    connect( ui->monstersRespawn_chkbox, SIGNAL( toggled(bool) ), this, SLOT( toggleMonstersRespawn(bool) ) );
    connect( ui->dmflags_btn, SIGNAL( clicked() ), this, SLOT( showDMFlags() ) );
    connect( ui->compatflags_btn, SIGNAL( clicked(bool) ), this, SLOT( showCompatFlags() ) );

    connect( ui->relativePaths_chkbox, SIGNAL( toggled(bool) ), this, SLOT( toggleRelativePaths(bool) ) );
    connect( ui->cmdargs_line, SIGNAL( textChanged(QString) ), this, SLOT( changeCmdArgs(QString) ) );
    connect( ui->saveOptions_btn, SIGNAL( clicked() ), this, SLOT( saveOptions() ) );
    connect( ui->loadOptions_btn, SIGNAL( clicked() ), this, SLOT( loadOptions() ) );
    connect( ui->exportOptions_btn, SIGNAL( clicked() ), this, SLOT( exportOptions() ) );
    connect( ui->importOptions_btn, SIGNAL( clicked() ), this, SLOT( importOptions() ) );
    connect( ui->launch_btn, SIGNAL( clicked() ), this, SLOT( launch() ) );

    if (QFile( WINDOW_PROPS_FILENAME ).exists())
        loadWindowProperties();

    if (QFile( CURRENT_CONFIG_FILENAME ).exists())
        loadOptionsFromFile( CURRENT_CONFIG_FILENAME );
    updateSaveFiles();

    tickCount = 0;
    startTimer( 2000 );
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent( QCloseEvent * event )
{
    autoSaveOptions();
    QWidget::closeEvent( event );
}

void MainWindow::timerEvent( QTimerEvent * event )
{
    if (ui->autoUpdIWADs_chkbox->isChecked())
        updateListFromDir( ui->IWADs_list, prevIWADsDir, ui->IWADsDir_line->text(), IWADs, false );
    if (ui->autoUpdPWADs_chkbox->isChecked())
        updateListFromDir( ui->PWADs_list, prevPWADsDir, ui->PWADsDir_line->text(), PWADs, true );
    if (ui->autoUpdMods_chkbox->isChecked())
        updateListFromDir( ui->mods_list, prevModsDir, ui->modsDir_line->text(), mods, true );
    updateSaveFiles();

    tickCount++;
    //if (tickCount % 5 == 0)
    //    autoSaveOptions();
}

bool MainWindow::saveWindowProperties()
{
    QString fileName = WINDOW_PROPS_FILENAME;

    QFile file( fileName );
    if (!file.open( QIODevice::WriteOnly )) {
        QMessageBox::warning( this, "Can't open file", "Saving window properties failed. Could not open file "+fileName+" for writing." );
        return false;
    }

    QJsonObject json;

    const QRect & geometry = this->geometry();
    json["pos_x"] = geometry.x();
    json["pos_y"] = geometry.y();
    json["width"] = geometry.width();
    json["height"] = geometry.height();

    QJsonDocument jsonDoc( json );
    file.write( jsonDoc.toJson() );
    file.close();

    return file.error() == QFile::NoError;
}

bool MainWindow::loadWindowProperties()
{
    QString fileName = WINDOW_PROPS_FILENAME;

    QFile file( fileName );
    if (!file.open( QIODevice::ReadOnly )) {
        QMessageBox::warning( this, "Can't open file", "Loading window properties failed. Could not open file "+fileName+" for reading." );
        return false;
    }

    QJsonDocument jsonDoc = QJsonDocument::fromJson( file.readAll() );
    QJsonObject json = jsonDoc.object();

  try {

    int posX = getInt( json, "pos_x" );
    int posY = getInt( json, "pos_y" );
    int width = getInt( json, "width" );
    int height = getInt( json, "height" );
    this->setGeometry( posX, posY, width, height );

  } catch (const JsonKeyMissing & ex) {
    QMessageBox::warning( this, "Error reading config file", "Element "+ex.key()+" is missing in the config. The file "+fileName+" seems to be corrupt." );
    return false;
  } catch (const JsonInvalidTypeAtKey & ex) {
    QMessageBox::warning( this, "Error reading config file", "Element "+ex.key()+" has invalid type, "+ex.expectedType()+" expected. The file "+fileName+" seems to be corrupt." );
    return false;
  } catch (const JsonInvalidTypeAtIdx & ex) {
    QMessageBox::warning( this, "Error reading config file", "Element on index "+QString::number(ex.index())+" has invalid type, "+ex.expectedType()+" expected. The "+fileName+" file seems to be corrupt." );
    return false;
  }

    file.close();

    return true;
}

bool MainWindow::saveOptionsToFile( QString fileName )
{
    QFile file( fileName );
    if (!file.open( QIODevice::WriteOnly )) {
        QMessageBox::warning( this, "Can't open file", "Saving options failed. Could not open file "+fileName+" for writing." );
        return false;
    }

    QJsonObject json;

    // source ports
    {
        QJsonObject jsSourcePorts;

        QJsonArray jsPortArray;
        for (int i = 0; i < ui->sourcePort_cmbbox->count(); i++) {
            QString name = ui->sourcePort_cmbbox->itemText(i);
            QJsonObject jsPort;
            jsPort["name"] = name;
            jsPort["path"] = ports[ name ];
            jsPortArray.append( jsPort );
        }
        jsSourcePorts["ports"] = jsPortArray;
        jsSourcePorts["selected"] = ui->sourcePort_cmbbox->currentIndex();

        json["source_ports"] = jsSourcePorts;
    }

    // IWADs
    {
        QJsonObject jsIWADs;

        jsIWADs["auto_update"] = ui->autoUpdIWADs_chkbox->checkState();
        jsIWADs["directory"] = ui->IWADsDir_line->text();
        QJsonArray jsIWADArray;
        for (int i = 0; i < ui->IWADs_list->count(); i++) {
            QString name = ui->IWADs_list->item(i)->text();
            QJsonObject jsIWAD;
            jsIWAD["name"] = name;
            jsIWAD["path"] = IWADs[ name ];
            jsIWADArray.append( jsIWAD );
        }
        jsIWADs["IWADs"] = jsIWADArray;
        jsIWADs["selected"] = ui->IWADs_list->currentRow();

        json["IWADs"] = jsIWADs;
    }

    // PWADs
    {
        QJsonObject jsPWADs;

        jsPWADs["auto_update"] = ui->autoUpdPWADs_chkbox->checkState();
        jsPWADs["directory"] = ui->PWADsDir_line->text();
        QJsonArray jsPWADArray;
        for (int i = 0; i < ui->PWADs_list->count(); i++) {
            QString name = ui->PWADs_list->item(i)->text();
            QJsonObject jsPWAD;
            jsPWAD["name"] = name;
            jsPWAD["path"] = PWADs[ name ];
            jsPWAD["selected"] = ui->PWADs_list->item(i)->checkState();
            jsPWADArray.append( jsPWAD );
        }
        jsPWADs["PWADs"] = jsPWADArray;

        json["PWADs"] = jsPWADs;
    }

    // mods
    {
        QJsonObject jsMods;

        jsMods["auto_update"] = ui->autoUpdMods_chkbox->checkState();
        jsMods["directory"] = ui->modsDir_line->text();
        QJsonArray jsModArray;
        for (int i = 0; i < ui->mods_list->count(); i++) {
            QString name = ui->mods_list->item(i)->text();
            QJsonObject jsMod;
            jsMod["name"] = name;
            jsMod["path"] = mods[ name ];
            jsMod["selected"] = ui->mods_list->item(i)->checkState();
            jsModArray.append( jsMod );
        }
        jsMods["mods"] = jsModArray;

        json["mods"] = jsMods;
    }

    // multiplayer
    {
        QJsonObject jsMult;

        jsMult["mult_role"] = ui->multRole_cmbbox->currentIndex();
        jsMult["player_count"] = ui->playerCnt_spinbox->value();
        jsMult["game_mode"] = ui->gameMode_cmbbox->currentIndex();
        jsMult["time_limit"] = ui->timeLimit_spinbox->value();
        jsMult["net_mode"] = ui->netMode_cmbbox->currentIndex();
        jsMult["team_dmg"] = ui->teamDmg_spinbox->value();
        jsMult["hostname"] = ui->IPAddress_line->text();
        jsMult["port"] = ui->port_spinbox->value();

        json["multiplayer"] = jsMult;
    }

    // general
    {
        QJsonObject jsGeneral;

        jsGeneral["direct_start"] = ui->directStart_chkbox->checkState();
        jsGeneral["load_game"] = ui->loadGame_chkbox->checkState();
        jsGeneral["load_filename"] = ui->loadGame_cmbbox->currentText();
        jsGeneral["map"] = ui->map_cmbbox->currentIndex();
        jsGeneral["skill"] = ui->skill_cmbbox->currentIndex();
        jsGeneral["skill_custom"] = ui->skill_spinbox->value();
        jsGeneral["no_monsters"] = ui->noMonsters_chkbox->checkState();
        jsGeneral["fast_monsters"] = ui->fastMonsters_chkbox->checkState();
        jsGeneral["monsters_respawn"] = ui->monstersRespawn_chkbox->checkState();
        jsGeneral["dmflags1"] = (int)dmflags1;
        jsGeneral["dmflags2"] = (int)dmflags2;
        jsGeneral["compatflags1"] = (int)compatflags1;
        jsGeneral["compatflags2"] = (int)compatflags2;

        json["general"] = jsGeneral;
    }

    // other
    json["use_relative_paths"] = ui->relativePaths_chkbox->checkState();
    json["additional_args"] = ui->cmdargs_line->text();

    QJsonDocument jsonDoc( json );
    file.write( jsonDoc.toJson() );

    file.close();

    return file.error() == QFile::NoError;
}

bool MainWindow::loadOptionsFromFile( QString fileName )
{
    QFile file( fileName );
    if (!file.open( QIODevice::ReadOnly )) {
        QMessageBox::warning( this, "Can't open file", "Loading options failed. Could not open file "+fileName+" for reading." );
        return false;
    }

    QJsonDocument jsonDoc = QJsonDocument::fromJson( file.readAll() );
    QJsonObject json = jsonDoc.object();

    int state, autoupd;
    QString name, path, upddir;

  try {

    // source ports
    {
        QJsonObject jsSourcePorts = getObject( json, "source_ports" );

        ui->sourcePort_cmbbox->clear();
        ports.clear();
        QJsonArray jsPortArray = getArray( jsSourcePorts, "ports" );
        for (int i = 0; i < jsPortArray.size(); i++) {
            QJsonObject jsPort = getObject( jsPortArray, i );
            name = getString( jsPort, "name" );
            path = getString( jsPort, "path" );
            if (!QFile( path ).exists())
                continue;
            ports[ name ] = path;
            ui->sourcePort_cmbbox->addItem( name );
        }
        ui->sourcePort_cmbbox->setCurrentIndex( getInt( jsSourcePorts, "selected" ) );
    }

    // IWADs
    {
        QJsonObject jsIWADs = getObject( json, "IWADs" );

        autoupd = getInt( jsIWADs, "auto_update" );
        upddir = getString( jsIWADs, "directory" );
        ui->IWADs_list->clear();
        IWADs.clear();
        QJsonArray jsIWADArray = getArray( jsIWADs, "IWADs" );
        for (int i = 0; i < jsIWADArray.size(); i++) {
            QJsonObject jsIWAD = getObject( jsIWADArray, i );
            name = getString( jsIWAD, "name" );
            path = getString( jsIWAD, "path" );
            if (!QFile( path ).exists())
                continue;
            IWADs[ name ] = path;
            addListItem( ui->IWADs_list, name, false );
        }
        ui->IWADs_list->setCurrentRow( getInt( jsIWADs, "selected" ) );
        prevIWADsDir = upddir;
        ui->IWADsDir_line->setText( upddir );
        ui->autoUpdIWADs_chkbox->setCheckState( (Qt::CheckState)autoupd );
    }

    // PWADs
    {
        QJsonObject jsPWADs = getObject( json, "PWADs" );

        autoupd = getInt( jsPWADs, "auto_update" );
        upddir = getString( jsPWADs, "directory" );
        ui->PWADs_list->clear();
        PWADs.clear();
        QJsonArray jsPWADArray = getArray( jsPWADs, "PWADs" );
        for (int i = 0; i < jsPWADArray.size(); i++) {
            QJsonObject jsPWAD = getObject( jsPWADArray, i );
            name = getString( jsPWAD, "name" );
            path = getString( jsPWAD, "path" );
            state = getInt( jsPWAD, "selected" );
            if (!QFile( path ).exists())
                continue;
            PWADs[ name ] = path;
            QListWidgetItem * item = addListItem( ui->PWADs_list, name, true );
            item->setCheckState( (Qt::CheckState)state );
        }
        prevPWADsDir = upddir;
        ui->PWADsDir_line->setText( upddir );
        ui->autoUpdPWADs_chkbox->setCheckState( (Qt::CheckState)autoupd );
    }

    // mods
    {
        QJsonObject jsMods = getObject( json, "mods" );

        autoupd = getInt( jsMods, "auto_update" );
        upddir = getString( jsMods, "directory" );
        ui->mods_list->clear();
        mods.clear();
        QJsonArray jsModArray = getArray( jsMods, "mods" );
        for (int i = 0; i < jsModArray.size(); i++) {
            QJsonObject jsMod = getObject( jsModArray, i );
            name = getString( jsMod, "name" );
            path = getString( jsMod, "path" );
            state = getInt( jsMod, "selected" );
            if (!QFile( path ).exists())
                continue;
            mods[ name ] = path;
            QListWidgetItem * item = addListItem( ui->mods_list, name, true );
            item->setCheckState( (Qt::CheckState)state );
        }
        prevModsDir = upddir;
        ui->modsDir_line->setText( upddir );
        ui->autoUpdMods_chkbox->setCheckState( (Qt::CheckState)autoupd );
    }

    // multiplayer
    {
        QJsonObject jsMult = getObject( json, "multiplayer" );

        ui->multRole_cmbbox->setCurrentIndex( getInt( jsMult, "mult_role" ) );
        ui->playerCnt_spinbox->setValue( getInt( jsMult, "player_count" ) );
        ui->gameMode_cmbbox->setCurrentIndex( getInt( jsMult, "game_mode" ) );
        ui->teamDmg_spinbox->setValue( getDouble( jsMult, "team_dmg" ) );
        ui->timeLimit_spinbox->setValue( getInt( jsMult, "time_limit" ) );
        ui->netMode_cmbbox->setCurrentIndex( getInt( jsMult, "net_mode" ) );
        ui->IPAddress_line->setText( getString( jsMult, "hostname" ) );
        ui->port_spinbox->setValue( getInt( jsMult, "port" ) );
    }

    // general
    {
        QJsonObject jsGeneral = getObject( json, "general" );

        ui->directStart_chkbox->setCheckState( (Qt::CheckState)getInt( jsGeneral, "direct_start" ) );
        ui->loadGame_chkbox->setCheckState( (Qt::CheckState)getInt( jsGeneral, "load_game" ) );
        ui->loadGame_cmbbox->setCurrentText( getString( jsGeneral, "load_filename" ) );
        ui->map_cmbbox->setCurrentIndex( getInt( jsGeneral, "map" ) );
        ui->skill_cmbbox->setCurrentIndex( getInt( jsGeneral, "skill" ) );
        ui->skill_spinbox->setValue( getInt( jsGeneral, "skill_custom" ) );
        ui->noMonsters_chkbox->setCheckState( (Qt::CheckState)getInt( jsGeneral, "no_monsters" ) );
        ui->fastMonsters_chkbox->setCheckState( (Qt::CheckState)getInt( jsGeneral, "fast_monsters" ) );
        ui->monstersRespawn_chkbox->setCheckState( (Qt::CheckState)getInt( jsGeneral, "monsters_respawn" ) );
        dmflags1 = getInt( jsGeneral, "dmflags1" );
        dmflags2 = getInt( jsGeneral, "dmflags2" );
        compatflags1 = getInt( jsGeneral, "compatflags1" );
        compatflags2 = getInt( jsGeneral, "compatflags2" );
    }

    // other
    ui->relativePaths_chkbox->setCheckState( (Qt::CheckState)getInt( json, "use_relative_paths" ) );
    ui->cmdargs_line->setText( getString( json, "additional_args" ) );

  } catch (const JsonKeyMissing & ex) {
    QMessageBox::warning( this, "Error reading config file", "Element "+ex.key()+" is missing in the config. The file "+fileName+" seems to be corrupt." );
    return false;
  } catch (const JsonInvalidTypeAtKey & ex) {
    QMessageBox::warning( this, "Error reading config file", "Element "+ex.key()+" has invalid type, "+ex.expectedType()+" expected. The file "+fileName+" seems to be corrupt." );
    return false;
  } catch (const JsonInvalidTypeAtIdx & ex) {
    QMessageBox::warning( this, "Error reading config file", "Element on index "+QString::number(ex.index())+" has invalid type, "+ex.expectedType()+" expected. The file "+fileName+" seems to be corrupt." );
    return false;
  }

    file.close();

    genLaunchCommand();

    return true;
}

void MainWindow::autoSaveOptions()
{
    saveWindowProperties();
    saveOptionsToFile( CURRENT_CONFIG_FILENAME );
}


//====================================================================================================
//  source ports

void MainWindow::addSourcePort()
{
    QFileInfo file( QFileDialog::getOpenFileName( this, tr("Locate source port executable") ) );
    if (!file.exists())
        return;

    QString portName = file.baseName();
    QString portPath = getPath( file.filePath() );

    if (ports.contains( portName )) {
        if (ports[ portName ] == portPath) {
            QMessageBox::warning( this, "Error adding source port", "This port is already there." );
            return;
        }

        QString newPortName;
        int portNum = 2;
        do {
            newPortName = portName + QString::number( portNum );
            portNum++;
        } while (ports.contains( newPortName ));
        portName = newPortName;
    }

    ports[ portName ] = portPath;
    ui->sourcePort_cmbbox->addItem( portName );
}

void MainWindow::delSourcePort()
{
    ports.remove( ui->sourcePort_cmbbox->currentText() );
    ui->sourcePort_cmbbox->removeItem( ui->sourcePort_cmbbox->currentIndex() );
}

void MainWindow::selectSourcePort( QString port )
{
    updateSaveFiles();

    genLaunchCommand();
}


//====================================================================================================
//  IWADs

void MainWindow::toggleUpdateIWADs( bool enabled )
{
    ui->IWADsDir_line->setEnabled( enabled );
    ui->browseIWADs_btn->setEnabled( enabled );
    ui->addIWAD_btn->setEnabled( !enabled );
    ui->delIWAD_btn->setEnabled( !enabled );

    if (enabled)
        updateListFromDir( ui->IWADs_list, prevIWADsDir, ui->IWADsDir_line->text(), IWADs, false );
}

void MainWindow::changeUpdateIWADsDir( QString dir )
{
    if (ui->autoUpdIWADs_chkbox->isChecked())
        updateListFromDir( ui->IWADs_list, prevIWADsDir, ui->IWADsDir_line->text(), IWADs, false );
}

void MainWindow::browseIWADs()
{
    QString path = QFileDialog::getExistingDirectory( this, tr("Locate the directory with IWADs") );
    if (path.length() == 0)
        return;

    ui->IWADsDir_line->setText( getPath( path ) );
}

void MainWindow::addIWAD()
{
    QFileInfo file( QFileDialog::getOpenFileName( this, tr("Locate the IWAD to be added") ) );
    if (!file.exists() || IWADs.contains( file.fileName() ))
        return;

    IWADs[ file.fileName() ] = getPath( file.filePath() );
    addListItem( ui->IWADs_list, file.fileName(), false );
}

void MainWindow::delIWAD()
{
    IWADs.remove( ui->IWADs_list->currentItem()->text() );
    delete ui->IWADs_list->takeItem( ui->IWADs_list->currentRow() );
}

void MainWindow::upIWAD()
{
    moveUpListItem( ui->IWADs_list );
}

void MainWindow::downIWAD()
{
    moveDownListItem( ui->IWADs_list );
}

void MainWindow::selectIWAD( QString text )
{
    if (ui->IWADs_list->currentItem())
        updateMaps();

    genLaunchCommand();
}


//====================================================================================================
//  PWADs

void MainWindow::toggleUpdatePWADs( bool enabled )
{
    ui->PWADsDir_line->setEnabled( enabled );
    ui->browsePWADs_btn->setEnabled( enabled );
    ui->addPWAD_btn->setEnabled( !enabled );
    ui->delPWAD_btn->setEnabled( !enabled );

    if (enabled)
        updateListFromDir( ui->PWADs_list, prevPWADsDir, ui->PWADsDir_line->text(), PWADs, true );
}

void MainWindow::changeUpdatePWADsDir( QString dir )
{
    if (ui->autoUpdPWADs_chkbox->isChecked())
        updateListFromDir( ui->PWADs_list, prevPWADsDir, ui->PWADsDir_line->text(), PWADs, true );
}

void MainWindow::browsePWADs()
{
    QString path = QFileDialog::getExistingDirectory( this, tr("Locate the directory with Maps/PWADs") );
    if (path.length() == 0)
        return;

    ui->PWADsDir_line->setText( getPath( path ) );
}

void MainWindow::addPWAD()
{
    QFileInfo file( QFileDialog::getOpenFileName( this, tr("Locate the PWAD to be added") ) );
    if (!file.exists() || PWADs.contains( file.fileName() ))
        return;

    PWADs[ file.fileName() ] = getPath( file.filePath() );
    addListItem( ui->PWADs_list, file.fileName(), true );
}

void MainWindow::delPWAD()
{
    PWADs.remove( ui->PWADs_list->currentItem()->text() );
    delete ui->PWADs_list->takeItem( ui->PWADs_list->currentRow() );
}

void MainWindow::upPWAD()
{
    moveUpListItem( ui->PWADs_list );
}

void MainWindow::downPWAD()
{
    moveDownListItem( ui->PWADs_list );
}

void MainWindow::selectPWAD( QListWidgetItem * item )
{
    genLaunchCommand();
}


//====================================================================================================
//  Mods

void MainWindow::toggleUpdateMods( bool enabled )
{
    ui->modsDir_line->setEnabled( enabled );
    ui->browseMods_btn->setEnabled( enabled );
    ui->addMod_btn->setEnabled( !enabled );
    ui->delMod_btn->setEnabled( !enabled );

    if (enabled)
        updateListFromDir( ui->mods_list, prevModsDir, ui->modsDir_line->text(), mods, true );
}

void MainWindow::changeUpdateModsDir( QString dir )
{
    if (ui->autoUpdMods_chkbox->isChecked())
        updateListFromDir( ui->mods_list, prevModsDir, ui->modsDir_line->text(), mods, true );
}

void MainWindow::browseMods()
{
    QString path = QFileDialog::getExistingDirectory( this, tr("Locate the directory with Mods") );
    if (path.length() == 0)
        return;

    ui->modsDir_line->setText( getPath( path ) );
}

void MainWindow::addMod()
{
    QFileInfo file( QFileDialog::getOpenFileName( this, tr("Locate the PWAD to be added") ) );
    if (!file.exists() || mods.contains( file.fileName() ))
        return;

    mods[ file.fileName() ] = getPath( file.filePath() );
    addListItem( ui->mods_list, file.fileName(), true );
}

void MainWindow::delMod()
{
    mods.remove( ui->mods_list->currentItem()->text() );
    delete ui->mods_list->takeItem( ui->mods_list->currentRow() );
}

void MainWindow::upMod()
{
    moveUpListItem( ui->mods_list );
}

void MainWindow::downMod()
{
    moveDownListItem( ui->mods_list );
}

void MainWindow::selectMod( QListWidgetItem * item )
{
    genLaunchCommand();
}


//====================================================================================================
//  multiplayer options

void MainWindow::selectMultRole( int role )
{
    switch (role) {
     case 0: // single-player
        ui->playerCnt_spinbox->setEnabled( false );
        ui->gameMode_cmbbox->setEnabled( false );
        ui->teamDmg_spinbox->setEnabled( false );
        ui->timeLimit_spinbox->setEnabled( false );
        ui->netMode_cmbbox->setEnabled( false );
        ui->IPAddress_line->setEnabled( false );
        ui->port_spinbox->setEnabled( false );
        ui->directStart_chkbox->setEnabled( true );
        ui->directStart_chkbox->setChecked( false );
        break;
     case 1: // server
        ui->playerCnt_spinbox->setEnabled( true );
        ui->gameMode_cmbbox->setEnabled( true );
        ui->teamDmg_spinbox->setEnabled( true );
        ui->timeLimit_spinbox->setEnabled( true );
        ui->netMode_cmbbox->setEnabled( true );
        ui->IPAddress_line->setEnabled( false );
        ui->port_spinbox->setEnabled( true );
        ui->directStart_chkbox->setEnabled( true );
        ui->directStart_chkbox->setChecked( true );
        break;
     case 2: // client
        ui->playerCnt_spinbox->setEnabled( false );
        ui->gameMode_cmbbox->setEnabled( false );
        ui->teamDmg_spinbox->setEnabled( false );
        ui->timeLimit_spinbox->setEnabled( false );
        ui->netMode_cmbbox->setEnabled( false );
        ui->IPAddress_line->setEnabled( true );
        ui->port_spinbox->setEnabled( true );
        ui->directStart_chkbox->setEnabled( false );
        ui->directStart_chkbox->setChecked( false );
        break;
     default:
        break;
    }

    genLaunchCommand();
}

void MainWindow::changePlayerCount( int number )
{
    genLaunchCommand();
}

void MainWindow::selectGameMode( int index )
{
    genLaunchCommand();
}

void MainWindow::changeTeamDamage( double number )
{
    genLaunchCommand();
}

void MainWindow::changeTimeLimit( int number )
{
    genLaunchCommand();
}

void MainWindow::selectNetMode( int number )
{
    genLaunchCommand();
}

void MainWindow::changeIP( QString text )
{
    genLaunchCommand();
}

void MainWindow::changePort( int number )
{
    genLaunchCommand();
}

void MainWindow::toggleDirectStart( bool enabled )
{
    if (enabled && ui->loadGame_chkbox->isChecked())
        ui->loadGame_chkbox->setChecked( false );
    else if (!enabled && !ui->loadGame_chkbox->isChecked() && ui->multRole_cmbbox->currentIndex() == 1)
        ui->loadGame_chkbox->setChecked( true );
    ui->map_cmbbox->setEnabled( enabled );
    ui->skill_cmbbox->setEnabled( enabled );
    ui->skill_spinbox->setEnabled( enabled && ui->skill_cmbbox->currentIndex() == 5 );
    ui->noMonsters_chkbox->setEnabled( enabled );
    ui->fastMonsters_chkbox->setEnabled( enabled );
    ui->monstersRespawn_chkbox->setEnabled( enabled );
    ui->dmflags_btn->setEnabled( enabled );
    ui->compatflags_btn->setEnabled( enabled );

    genLaunchCommand();
}

void MainWindow::toggleLoadGame( bool enabled )
{
    if (enabled && ui->directStart_chkbox->isChecked())
        ui->directStart_chkbox->setChecked( false );
    else if (!enabled && !ui->directStart_chkbox->isChecked() && ui->multRole_cmbbox->currentIndex() == 1)
        ui->directStart_chkbox->setChecked( true );
    ui->loadGame_cmbbox->setEnabled( enabled );

    genLaunchCommand();
}

void MainWindow::changeSaveFile( QString text )
{
    genLaunchCommand();
}

void MainWindow::changeMap( QString text )
{
    genLaunchCommand();
}

void MainWindow::changeSkill( int number )
{
    ui->skill_spinbox->setValue( number );
    ui->skill_spinbox->setEnabled( number == 5 );

    //genLaunchCommand();
}

void MainWindow::changeSkillNum( int number )
{
    genLaunchCommand();
}

void MainWindow::toggleNoMonsters( bool enabled )
{
    genLaunchCommand();
}

void MainWindow::toggleFastMonsters( bool enabled )
{
    genLaunchCommand();
}

void MainWindow::toggleMonstersRespawn( bool enabled )
{
    genLaunchCommand();
}

void MainWindow::showDMFlags()
{
    DMFlags window( this, dmflags1, dmflags2 );
    window.setModal( true );
    connect( &window, SIGNAL( dmflagsConfirmed(uint32_t,uint32_t) ), this, SLOT( setDMFLags(uint32_t,uint32_t) ) );
    window.exec();

    genLaunchCommand();
}

void MainWindow::showCompatFlags()
{
/*    CompatFlags window( this );
    window.setModal( true );
    connect( &window, SIGNAL( compatflagsConfirmed(uint32_t) ), this, SLOT( setCompatFlags(uint32_t) ) );
    window.exec();

    genLaunchCommand();
*/
     QMessageBox::warning( this, "Not implemented", "Sorry, this feature is not finished yet. Use additional param +compatflags" );
}

void MainWindow::setDMFLags( uint32_t flags1 , uint32_t flags2 )
{
    dmflags1 = flags1;
    dmflags2 = flags2;
}

void MainWindow::setCompatFlags( uint32_t flags1 , uint32_t flags2 )
{
    compatflags1 = flags1;
    compatflags2 = flags2;
}


//====================================================================================================
//  other

void MainWindow::toggleRelativePaths( bool relative )
{
    if (relative)
        convertPathsToRelative();
    else
        convertPathsToAbsolute();

    genLaunchCommand();
}

void MainWindow::convertPathsToRelative()
{
    QString newDir;

    QHashIterator<QString,QString> it( ports );
    while (it.hasNext()) {
        it.next();
        ports[ it.key() ] = getRelPath( it.value() );
    }

    newDir = getRelPath( ui->IWADsDir_line->text() );
    prevIWADsDir = newDir;
    ui->IWADsDir_line->setText( newDir );
    it = IWADs;
    while (it.hasNext()) {
        it.next();
        IWADs[ it.key() ] = getRelPath( it.value() );
    }

    newDir = getRelPath( ui->PWADsDir_line->text() );
    prevPWADsDir = newDir;
    ui->PWADsDir_line->setText( newDir );
    it = PWADs;
    while (it.hasNext()) {
        it.next();
        PWADs[ it.key() ] = getRelPath( it.value() );
    }

    newDir = getRelPath( ui->modsDir_line->text() );
    prevModsDir = newDir;
    ui->modsDir_line->setText( newDir );
    it = mods;
    while (it.hasNext()) {
        it.next();
        mods[ it.key() ] = getRelPath( it.value() );
    }
}

void MainWindow::convertPathsToAbsolute()
{
    QString newDir;

    QHashIterator<QString,QString> it ( ports );
    while (it.hasNext()) {
        it.next();
        ports[ it.key() ] = getAbsPath( it.value() );
    }

    newDir = getAbsPath( ui->IWADsDir_line->text() );
    prevIWADsDir = newDir;
    ui->IWADsDir_line->setText( newDir );
    it = IWADs;
    while (it.hasNext()) {
        it.next();
        IWADs[ it.key() ] = getAbsPath( it.value() );
    }

    newDir = getAbsPath( ui->PWADsDir_line->text() );
    prevPWADsDir = newDir;
    ui->PWADsDir_line->setText( newDir );
    it = PWADs;
    while (it.hasNext()) {
        it.next();
        PWADs[ it.key() ] = getAbsPath( it.value() );
    }

    newDir = getAbsPath( ui->modsDir_line->text() );
    prevModsDir = newDir;
    ui->modsDir_line->setText( newDir );
    it = mods;
    while (it.hasNext()) {
        it.next();
        mods[ it.key() ] = getAbsPath( it.value() );
    }
}

void MainWindow::changeCmdArgs( QString text )
{
    genLaunchCommand();
}

void MainWindow::saveOptions()
{
    QString file = QFileDialog::getSaveFileName( this, tr("Specify a file to save options to"), "SavedOptions", "*.json" );
    if (file.length() == 0)
        return;

    saveOptionsToFile( file );
}

void MainWindow::loadOptions()
{
    QString file = QFileDialog::getOpenFileName( this, tr("Specify a file to load options from"), "SavedOptions", "*.json" );
    if (file.length() == 0)
        return;

    loadOptionsFromFile( file );
}

void MainWindow::exportOptions()
{
    QString fileName = QFileDialog::getSaveFileName( this, tr("Specify a script file to export options to"), ".", SCRIPT_EXT );
    if (fileName.length() == 0)
        return;

    QFile file( fileName );
    if (!file.open( QIODevice::WriteOnly )) {
        QMessageBox::warning( this, "Can't open file", "Exporting script failed. Could not open file "+fileName+" for writing." );
        return;
    }
    QTextStream out( &file );

    out << "start " << ui->command_line->text() << endl;

    file.close();
}

void MainWindow::importOptions()
{
    QMessageBox::warning( this, tr("Not implemented"), tr("This feature is not finished yet.") );
}

void MainWindow::launch()
{
    // !!todo!! check paths

    bool success = QProcess::startDetached( ui->command_line->text() );
    if (!success)
        QMessageBox::warning( this, tr("Launch error"), tr("Failed to execute launch command.") );
}


//====================================================================================================
//  common methods

void MainWindow::updateListFromDir( QListWidget * list, QString & prevDir, QString newDir, QHash<QString,QString> & paths, bool checkable )
{
    if (newDir.length() == 0)
        return;

    QDir dir( newDir );
    if (!dir.exists())
        return;

    if (newDir != prevDir) {
        prevDir = newDir;
        reinsertListFromDir( list, dir, paths, checkable );
    } else {
        correctListFromDir( list, dir, paths, checkable );
    }

    genLaunchCommand();
}

void MainWindow::reinsertListFromDir( QListWidget * list, QDir newDir, QHash<QString,QString> & paths, bool checkable )
{
    paths.clear();
    list->clear();

    QDirIterator dirIt( newDir );
    while (dirIt.hasNext()) {
        dirIt.next();
        if (!dirIt.fileInfo().isDir()) {
            paths[ dirIt.fileName() ] = getPath( dirIt.filePath() );
            addListItem( list, dirIt.fileName(), checkable );
        }
    }
}

void MainWindow::correctListFromDir( QListWidget * list, QDir newDir, QHash<QString,QString> & paths, bool checkable )
{
    QSet<QString> dirItems;

    QDirIterator dirIt( newDir );
    while (dirIt.hasNext()) {
        dirIt.next();
        if (!dirIt.fileInfo().isDir()) {
            dirItems.insert( dirIt.fileName() );
            if (!paths.contains( dirIt.fileName() )) {
                paths[ dirIt.fileName() ] = getPath( dirIt.filePath() );
                addListItem( list, dirIt.fileName(), checkable );
            }
        }
    }

    for (int i = 0; i < list->count();) {
        QString listItem = list->item(i)->text();
        if (!dirItems.contains( listItem )) {
            delete list->takeItem(i);
            paths.remove( listItem );
            continue;
        }
        i++;
    }
}

QListWidgetItem * MainWindow::addListItem( QListWidget * list, QString text, bool checkable )
{
    QListWidgetItem * item = new QListWidgetItem();
    item->setData( Qt::DisplayRole, text );
    if (checkable) {
        item->setFlags( item->flags() | Qt::ItemFlag::ItemIsUserCheckable );
        item->setCheckState( Qt::Unchecked );
    }
    list->addItem( item );
    return item;
}

void MainWindow::moveUpListItem( QListWidget * list )
{
    int row = list->currentRow();
    if (row > 0) {
        QListWidgetItem * item = list->takeItem( row );
        list->insertItem( row-1, item );
        list->setCurrentRow( row-1 );
    }

    genLaunchCommand();
}

void MainWindow::moveDownListItem( QListWidget * list )
{
    int row = list->currentRow();
    if (row < list->count() - 1) {
        QListWidgetItem * item = list->takeItem( row );
        list->insertItem( row+1, item );
        list->setCurrentRow( row+1 );
    }

    genLaunchCommand();
}

void MainWindow::updateMaps()
{
    QString text = ui->IWADs_list->currentItem()->text();

    if ((text.compare("doom.wad",Qt::CaseInsensitive)==0 || text.startsWith("doom1",Qt::CaseInsensitive))
    && !ui->map_cmbbox->itemText(0).startsWith('E')) {
        ui->map_cmbbox->clear();
        for (int i = 1; i <= 9; i++)
            ui->map_cmbbox->addItem( QStringLiteral("E1M%1").arg(i) );
        for (int i = 1; i <= 9; i++)
            ui->map_cmbbox->addItem( QStringLiteral("E1M%1").arg(i) );
        for (int i = 1; i <= 9; i++)
            ui->map_cmbbox->addItem( QStringLiteral("E1M%1").arg(i) );
    } else if (!ui->map_cmbbox->itemText(0).startsWith('M')) {
        ui->map_cmbbox->clear();
        for (int i = 1; i <= 32; i++)
            ui->map_cmbbox->addItem( QStringLiteral("MAP%1").arg( i, 2, 10, QChar('0') ) );
    }
}

void MainWindow::updateSaveFiles()
{
    if (ui->sourcePort_cmbbox->count() == 0)
        return;

    QString curText = ui->loadGame_cmbbox->currentText();

    ui->loadGame_cmbbox->clear();

    QFileInfo info( ports[ ui->sourcePort_cmbbox->currentText() ] );
    QDir dir( info.absolutePath() );
    QDirIterator dirIt( dir );
    while (dirIt.hasNext()) {
        QFileInfo entry( dirIt.next() );
        if (!entry.isDir() && entry.completeSuffix() == "zds")
            ui->loadGame_cmbbox->addItem( entry.fileName() );
    }

    ui->loadGame_cmbbox->setCurrentText( curText );
}

void MainWindow::genLaunchCommand()
{
    QString command;

    if (ui->sourcePort_cmbbox->count() > 0) {
        command += "\"" + ports[ ui->sourcePort_cmbbox->currentText() ] + "\"";
    }

    if (ui->IWADs_list->currentItem()) {
        command += " -iwad \"" + IWADs[ ui->IWADs_list->currentItem()->text() ] + "\"";
    }

    for (int i = 0; i < ui->PWADs_list->count(); i++) {
        QListWidgetItem * item = ui->PWADs_list->item(i);
        if (item->checkState() == Qt::Checked) {
            command += " -file \"" + PWADs[ item->text() ] + "\"";
        }
    }

    for (int i = 0; i < ui->mods_list->count(); i++) {
        QListWidgetItem * item = ui->mods_list->item(i);
        if (item->checkState() == Qt::Checked) {
            command += " -file \"" + mods[ item->text() ] + "\"";
        }
    }

    switch (ui->multRole_cmbbox->currentIndex()) {
     case 0: // single-player
        break;
     case 1: // server
        command += " -host " + ui->playerCnt_spinbox->text();
        if (ui->port_spinbox->value() != 5029)
            command += " -port " + ui->port_spinbox->text();
        switch (ui->gameMode_cmbbox->currentIndex()) {
         case 0: // deathmatch
            command += " -deathmatch";
            break;
         case 1: // team deathmatch
            command += " -deathmatch +teamplay";
            break;
         case 2: // alternative deathmatch
            command += " -altdeath";
            break;
         case 3: // alternative team deathmatch
            command += " -altdeath +teamplay";
            break;
         default: // cooperative - default mode, which is started without any param
            break;
        }
        if (ui->teamDmg_spinbox->value() != 0.0)
            command += " +teamdamage " + QString::number( ui->teamDmg_spinbox->value(), 'f', 2 );
        if (ui->timeLimit_spinbox->value() != 0)
            command += " -timer " + ui->timeLimit_spinbox->text();
        command += " -netmode " + QString::number( ui->netMode_cmbbox->currentIndex() );
        break;
     case 2: // client
        command += " -join " + ui->IPAddress_line->text() + ":" + ui->port_spinbox->text();
        break;
     default:
        break;
    }

    if (ui->directStart_chkbox->isChecked()) {
        command += " -warp " + getMapNumber( ui->map_cmbbox->currentText() );
        command += " -skill " + ui->skill_spinbox->text();
        if (ui->noMonsters_chkbox->isChecked())
            command += " -nomonsters";
        if (ui->fastMonsters_chkbox->isChecked())
            command += " -fast";
        if (ui->monstersRespawn_chkbox->isChecked())
            command += " -respawn";
        if (dmflags1 != 0)
            command += " +dmflags " + QString::number( dmflags1 );
        if (dmflags2 != 0)
            command += " +dmflags2 " + QString::number( dmflags2 );
        if (compatflags1 != 0)
            command += " +compatflags " + QString::number( compatflags1 );
        if (compatflags2 != 0)
            command += " +compatflags2 " + QString::number( compatflags2 );
    } else if (ui->loadGame_chkbox->isChecked()) {
        command += " -loadgame " + ui->loadGame_cmbbox->currentText();
    }

    if (ui->cmdargs_line->text().length()) {
        command += " " + ui->cmdargs_line->text();
    }

    ui->command_line->setText( command );
}

QString MainWindow::getPath( QString path )
{
    if (ui->relativePaths_chkbox->isChecked())
        return getRelPath( path );
    else
        return getAbsPath( path );
}

QString MainWindow::getAbsPath( QString path )
{
    if (QDir::isAbsolutePath( path ))
        return path;

    QFileInfo info( path );
    return info.absoluteFilePath();
}

QString MainWindow::getRelPath( QString path )
{
    if (QDir::isRelativePath( path ))
        return path;

    return currentDir.relativeFilePath( path );
}

QString MainWindow::getMapNumber( QString mapName )
{
    if (mapName.startsWith('E')) {
        return mapName[1]+QString(' ')+mapName[3];
    } else {
        return mapName.mid(3,2);
    }
}
