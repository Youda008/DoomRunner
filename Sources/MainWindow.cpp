//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  13.5.2019
// Description:
//======================================================================================================================

#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include "SetupDialog.hpp"
#include "DMFlagsDialog.hpp"
#include "CompatFlagsDialog.hpp"
#include "JsonHelper.hpp"
#include "Utils.hpp"

#include <cstdio>

#include <QString>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QMessageBox>
#include <QListWidgetItem>
#include <QHash>
//#include <QSet>
#include <QTimer>
#include <QProcess>
#include <QDesktopWidget>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>


//======================================================================================================================

// TODO: mac
#ifdef _WIN32
	static const QString scriptFileExt = "*.bat";
#else
	static const QString scriptFileExt = "*.sh";
#endif

static constexpr char defaultOptionsFile [] = "options.json";


//======================================================================================================================
//  fixed combo boxes values

enum Skill {
	TOO_YOUNG_TO_DIE,
	NOT_TOO_ROUGH,
	HURT_ME_PLENTY,
	ULTRA_VIOLENCE,
	NIGHTMARE,
	CUSTOM
};

enum MultRole {
	SERVER,
	CLIENT
};

enum NetMode {
	PEER_TO_PEER,
	PACKET_SERVER
};

enum GameMode {
	DEATHMATCH,
	TEAM_DEATHMATCH,
	ALT_DEATHMATCH,
	ALT_TEAM_DEATHMATCH,
	COOPERATIVE
};


//======================================================================================================================

MainWindow::MainWindow()

	: QMainWindow( nullptr )
	, shown( false )
	, width( -1 )
	, height( -1 )
	, tickCount( 0 )
	, pathHelper( false )
	, engineModel( engines )
	, iwadModel( iwads )
	, iwadListFromDir( false )
	, mapModel( maps )
	, selectedPackIdx( -1 )
	, presetModel( presets )
	, dmflags1( 0 )
	, dmflags2( 0 )
	, compatflags1( 0 )
	, compatflags2( 0 )
{
	ui = new Ui::MainWindow;
	ui->setupUi( this );

	// setup view models
	ui->engineCmbBox->setModel( &engineModel );
	ui->presetListView->setModel( &presetModel );
	ui->iwadListView->setModel( &iwadModel );
	ui->mapListView->setModel( &mapModel );

	// setup signals
	connect( ui->setupPathsAction, &QAction::triggered, this, &thisClass::runSetupDialog );
	connect( ui->exportPresetAction, &QAction::triggered, this, &thisClass::exportPreset );
	connect( ui->importPresetAction, &QAction::triggered, this, &thisClass::importPreset );
	connect( ui->exitAction, &QAction::triggered, this, &thisClass::close );

	connect( ui->engineCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectEngine );
	connect( ui->presetListView, &QListView::clicked, this, &thisClass::selectPreset );
	connect( ui->iwadListView, &QListView::clicked, this, &thisClass::toggleIWAD );
	connect( ui->mapListView, &QListView::clicked, this, &thisClass::toggleMapPack );
	connect( ui->modList, &QListWidget::itemChanged, this, &thisClass::toggleMod );

	connect( ui->presetBtnAdd, &QPushButton::clicked, this, &thisClass::presetAdd );
	connect( ui->presetBtnDel, &QPushButton::clicked, this, &thisClass::presetDelete );
	connect( ui->presetBtnUp, &QPushButton::clicked, this, &thisClass::presetMoveUp );
	connect( ui->presetBtnDown, &QPushButton::clicked, this, &thisClass::presetMoveDown );

	connect( ui->modBtnAdd, &QPushButton::clicked, this, &thisClass::modAdd );
	connect( ui->modBtnDel, &QPushButton::clicked, this, &thisClass::modDelete );
	connect( ui->modBtnUp, &QPushButton::clicked, this, &thisClass::modMoveUp );
	connect( ui->modBtnDown, &QPushButton::clicked, this, &thisClass::modMoveDown );

	connect( ui->launchMode_menu, &QRadioButton::clicked, this, &thisClass::modeGameMenu );
	connect( ui->launchMode_map, &QRadioButton::clicked, this, &thisClass::modeSelectedMap );
	connect( ui->launchMode_savefile, &QRadioButton::clicked, this, &thisClass::modeSavedGame );
	connect( ui->mapCmbBox, &QComboBox::currentTextChanged, this, &thisClass::selectMap );
	connect( ui->saveFileCmbBox, &QComboBox::currentTextChanged, this, &thisClass::selectSavedGame );
	connect( ui->skillCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectSkill );
	connect( ui->skillSpinBox, QOverload<int>::of( &QSpinBox::valueChanged ), this, &thisClass::changeSkillNum );
	connect( ui->noMonstersChkBox, &QCheckBox::toggled, this, &thisClass::toggleNoMonsters );
	connect( ui->fastMonstersChkBox, &QCheckBox::toggled, this, &thisClass::toggleFastMonsters );
	connect( ui->monstersRespawnChkBox, &QCheckBox::toggled, this, &thisClass::toggleMonstersRespawn );
	connect( ui->dmFlagsBtn, &QPushButton::clicked, this, &thisClass::runDMFlagsDialog );
	connect( ui->compatFlagsBtn, &QPushButton::clicked, this, &thisClass::runCompatFlagsDialog );

	connect( ui->multiplayerChkBox, &QCheckBox::toggled, this, &thisClass::toggleMultiplayer );
	connect( ui->multRoleCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectMultRole );
	connect( ui->hostnameLine, &QLineEdit::textChanged, this, &thisClass::changeHost );
	connect( ui->portSpinBox, QOverload<int>::of( &QSpinBox::valueChanged ), this, &thisClass::changePort );
	connect( ui->netModeCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectNetMode );
	connect( ui->gameModeCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectGameMode );
	connect( ui->playerCountSpinBox, QOverload<int>::of( &QSpinBox::valueChanged ), this, &thisClass::changePlayerCount );
	connect( ui->teamDmgSpinBox, QOverload<double>::of( &QDoubleSpinBox::valueChanged ), this, &thisClass::changeTeamDamage );
	connect( ui->timeLimitSpinBox, QOverload<int>::of( &QSpinBox::valueChanged ), this, &thisClass::changeTimeLimit );

	connect( ui->cmdArgsLine, &QLineEdit::textChanged, this, &thisClass::updateAdditionalArgs );
	connect( ui->launchBtn, &QPushButton::clicked, this, &thisClass::launch );

	// setup drag&drop
	ui->modList->setDragEnabled( true );
	ui->modList->setAcceptDrops( true );
	ui->modList->setDropIndicatorShown( true );
	connect( ui->modList, &FileSystemDnDListWidget::fileSystemPathDropped, this, &thisClass::addModByPath );

	// try to load last saved state
	if (QFileInfo( defaultOptionsFile ).exists())
		loadOptions( defaultOptionsFile );
	else  // this is a first run, perform an initial setup
		QTimer::singleShot( 1, this, &thisClass::runSetupDialog );

	// setup an update timer
	startTimer( 1000 );
}

void MainWindow::showEvent( QShowEvent * )
{
	// the window doesn't have the geometry.x and geometry.y set yet in the constructor, so we must do it here
	if (width > 0 && height > 0) {
		const QRect & geometry = this->geometry();
		this->setGeometry( geometry.x(), geometry.y(), width, height );
	}

	shown = true;
}

MainWindow::~MainWindow()
{
	delete ui;
}


//----------------------------------------------------------------------------------------------------------------------
//  slots

void MainWindow::runSetupDialog()
{
	SetupDialog dialog( this, pathHelper, engines, iwads, iwadListFromDir, iwadDir, mapDir, modDir );
	connect( &dialog, &SetupDialog::iwadListNeedsUpdate, this, &thisClass::updateIWADsFromDir );
	connect( &dialog, &SetupDialog::absolutePathsToggled, this, &thisClass::toggleAbsolutePaths );
	dialog.exec();

	// update the views in this window, because the dialog may have changed the underlying data
	engineModel.updateUI( 0 );
	iwadModel.updateUI( 0 );

	generateLaunchCommand();
}

void MainWindow::runDMFlagsDialog()
{
	DMFlagsDialog dialog( this, dmflags1, dmflags2 );
	dialog.exec();

	generateLaunchCommand();
}

void MainWindow::runCompatFlagsDialog()
{
	QMessageBox::warning( this, "Not implemented", "Sorry, this feature is not implemented yet" );

	//CompatFlagsDialog dialog( this, compatflags1, compatflags2 );
	//dialog.exec();

	//generateLaunchCommand();
}

void MainWindow::selectEngine( int index )
{
	// update the current preset
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedPresetIdx >= 0) {
		presets[ selectedPresetIdx ].selectedEnginePath = engines[ index ].path;
	}

	updateSaveFilesFromDir();

	generateLaunchCommand();
}

void MainWindow::selectPreset( const QModelIndex & index )
{
	Preset & preset = presets[ index.row() ];

	// restore selected engine
	if (!preset.selectedEnginePath.isEmpty()) {  // the engine combo box might have been empty when creating this preset
		int engineIdx = findSuch<Engine>( engines, [ &preset ]( const Engine & engine )
		                                  { return engine.path == preset.selectedEnginePath; } );
		if (engineIdx >= 0) {
			ui->engineCmbBox->setCurrentIndex( engineIdx );
		} else {
			QMessageBox::warning( this, "Engine no longer exists",
				"Engine selected for this preset ("%preset.selectedEnginePath%") no longer exists, please select another one." );
		}
	}

	// restore selected IWAD
	deselectSelectedItem( ui->iwadListView );
	selectedIWADIdx = -1;
	if (!preset.selectedIWAD.isEmpty()) {  // the IWAD may have not been selected when creating this preset
		int iwadIdx = findSuch<IWAD>( iwads, [ &preset ]( const IWAD & iwad ) { return iwad.name == preset.selectedIWAD; } );
		if (iwadIdx >= 0) {
			selectItemByIdx( ui->iwadListView, iwadIdx );
			selectedIWADIdx = iwadIdx;
			updateMapsFromIWAD();
		} else {
			QMessageBox::warning( this, "IWAD no longer exists",
				"IWAD selected for this preset ("%preset.selectedIWAD%") no longer exists, please select another one." );
		}
	}
/*
	// restore selected MapPack
	deselectSelectedItem( ui->mapListView );
	if (!preset.selectedMapPack.isEmpty()) {  // the map pack may have not been selected when creating this preset
		int mapIdx = findSuch<MapPack>( maps, [ &preset ]( const MapPack & pack ) { return pack.name == preset.selectedMapPack; } );
		if (mapIdx >= 0) {
			selectItemByIdx( ui->mapListView, mapIdx );
			selectedPackIdx = mapIdx;
		} else {
			QMessageBox::warning( this, "IWAD no longer exists",
				"Map pack selected for this preset ("%preset.selectedMapPack%") no longer exists, please select another one." );
		}
	}
*/
	// restore list of mods
	ui->modList->clear();
	modInfo.clear();
	for (auto modIt = preset.mods.begin(); modIt != preset.mods.end(); )
	{
		const Preset::ModEntry & mod = *modIt;

		if (!QFileInfo( mod.info.path ).exists()) {
			QMessageBox::warning( this, "Mod no longer exists",
				"A mod from the preset ("%mod.info.path%") no longer exists. It will be removed from the list." );
			modIt = preset.mods.erase( modIt );  // keep the list widget in sync with the preset list
			continue;
		}

		modInfo[ mod.name ] = { mod.info.path };
		QListWidgetItem * item = new QListWidgetItem();
		item->setData( Qt::DisplayRole, mod.name );
		item->setFlags( item->flags() | Qt::ItemFlag::ItemIsUserCheckable );
		item->setCheckState( mod.checked ? Qt::Checked : Qt::Unchecked );
		ui->modList->addItem( item );

		modIt++;
	}

	generateLaunchCommand();
}

void MainWindow::toggleIWAD( const QModelIndex & index )
{
	// allow the user to deselect the IWAD by clicking on it again
	int clickedIWADIdx = index.row();
	if (clickedIWADIdx == selectedIWADIdx) {
		selectedIWADIdx = -1;
		ui->iwadListView->selectionModel()->select( index, QItemSelectionModel::Deselect );
	} else {
		selectedIWADIdx = clickedIWADIdx;
	}

	// update the current preset
	int clickedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (clickedPresetIdx >= 0) {
		if (selectedIWADIdx >= 0)
			presets[ clickedPresetIdx ].selectedIWAD = iwads[ index.row() ].name;
		else
			presets[ clickedPresetIdx ].selectedIWAD.clear();
	}

	updateMapsFromIWAD();

	generateLaunchCommand();
}

void MainWindow::toggleMapPack( const QModelIndex & index )
{
	// allow the user to deselect the map pack by clicking on it again
	int clickedPackIdx = index.row();
	if (clickedPackIdx == selectedPackIdx) {
		selectedPackIdx = -1;
		ui->mapListView->selectionModel()->select( index, QItemSelectionModel::Deselect );
	} else {
		selectedPackIdx = clickedPackIdx;
	}
/*
	// update the current preset
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedPresetIdx >= 0) {
		if (selectedPackIdx >= 0)
			presets[ selectedPresetIdx ].selectedMapPack = maps[ clickedPackIdx ].name;
		else
			presets[ selectedPresetIdx ].selectedMapPack.clear();  // deselect it also from preset
	}
*/
	generateLaunchCommand();
}

void MainWindow::toggleMod( QListWidgetItem * item )
{
	// update the current preset
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedPresetIdx >= 0) {
		// the item doesn't know its index in the list widget
		for (int i = 0; i < ui->modList->count(); i++)
			if (ui->modList->item(i) == item)
				presets[ selectedPresetIdx ].mods[i].checked = item->checkState() == Qt::Checked;
	}

	generateLaunchCommand();
}

void MainWindow::presetAdd()
{
  static uint presetNum = 1;

	// clear the widgets displaying preset content
	deselectSelectedItem( ui->iwadListView );
	selectedIWADIdx = -1;
	//deselectSelectedItem( ui->mapListView );
	//selectedPackIdx = -1;
	ui->modList->clear();

	// create it with the currently selected engine
	QString currentEngine = "";
	int engineIdx = ui->engineCmbBox->currentIndex();
	if (engineIdx >= 0)  // engine might not have been selected yet
		currentEngine = engines[ engineIdx ].path;

	appendItem( ui->presetListView, presetModel, { "Preset"+QString::number( presetNum ), currentEngine, "", {} } );

	ui->presetListView->edit( presetModel.index( presets.count() - 1, 0 ) );

	presetNum++;
}

void MainWindow::presetDelete()
{
	deleteSelectedItem( ui->presetListView, presetModel );
}

void MainWindow::presetMoveUp()
{
	moveUpSelectedItem( ui->presetListView, presetModel );
}

void MainWindow::presetMoveDown()
{
	moveDownSelectedItem( ui->presetListView, presetModel );
}

void MainWindow::modAdd()
{
	QString path = QFileDialog::getOpenFileName( this, "Locate the mod file", modDir );
	if (path.isEmpty())  // user probably clicked cancel
		return;

	addModByPath( path );
}

void MainWindow::modDelete()
{
	int selectedModIdx = getSelectedItemIdx( ui->modList );
	if (selectedModIdx < 0) {  // if no item is selected
		if (ui->modList->count() > 0)
			QMessageBox::warning( this, "No item selected", "No item is selected." );
		return;
	}

	// update selection
	if (selectedModIdx == ui->modList->count() - 1) {            // if item is the last one
		deselectItemByIdx( ui->modList, selectedModIdx );        // deselect it
		if (selectedModIdx > 0) {                                // and if it's not the only one
			selectItemByIdx( ui->modList, selectedModIdx - 1 );  // select the previous
		}
	}

	// remove it from the list widget
	modInfo.remove( ui->modList->item( selectedModIdx )->text() );
	delete ui->modList->takeItem( selectedModIdx );

	// remove it also from the current preset
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedPresetIdx >= 0) {
		presets[ selectedPresetIdx ].mods.removeAt( selectedModIdx );
	}

	generateLaunchCommand();
}

void MainWindow::modMoveUp()
{
	int selectedModIdx = getSelectedItemIdx( ui->modList );
	if (selectedModIdx < 0) {  // if no item is selected
		QMessageBox::warning( ui->modList->parentWidget(), "No item selected", "No item is selected." );
		return;
	}
	if (selectedModIdx == 0) {  // if the selected item is the first one, do nothing
		return;
	}

	// move it up in the list widget
	QListWidgetItem * item = ui->modList->takeItem( selectedModIdx );
	ui->modList->insertItem( selectedModIdx - 1, item );

	// update selection
	deselectSelectedItem( ui->modList );  // the list widget, when item is removed, automatically selects some other one
	selectItemByIdx( ui->modList, selectedModIdx - 1 );

	// move it up also in the preset
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedPresetIdx >= 0) {
		presets[ selectedPresetIdx ].mods.move( selectedModIdx, selectedModIdx - 1 );
	}

	generateLaunchCommand();
}

void MainWindow::modMoveDown()
{
	int selectedModIdx = getSelectedItemIdx( ui->modList );
	if (selectedModIdx < 0) {  // if no item is selected
		QMessageBox::warning( ui->modList->parentWidget(), "No item selected", "No item is selected." );
		return;
	}
	if (selectedModIdx == ui->modList->count() - 1) {  // if the selected item is the last one, do nothing
		return;
	}

	// move it down in the list widget
	QListWidgetItem * item = ui->modList->takeItem( selectedModIdx );
	ui->modList->insertItem( selectedModIdx + 1, item );

	// update selection
	deselectSelectedItem( ui->modList );  // the list widget, when item is removed, automatically selects some other one
	selectItemByIdx( ui->modList, selectedModIdx + 1 );

	// move it down also in the preset
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedPresetIdx >= 0) {
		presets[ selectedPresetIdx ].mods.move( selectedModIdx, selectedModIdx + 1 );
	}

	generateLaunchCommand();
}

void MainWindow::addModByPath( QString path )
{
	QString name = QFileInfo( path ).fileName();

	// the path comming out of the file dialog and drops is always absolute
	if (pathHelper.useRelativePaths())
		path = pathHelper.getRelativePath( path );

	modInfo[ name ] = { path };

	// add it to the list widget
	QListWidgetItem * item = new QListWidgetItem();
	item->setData( Qt::DisplayRole, name );
	item->setFlags( item->flags() | Qt::ItemFlag::ItemIsUserCheckable );
	item->setCheckState( Qt::Checked );
	ui->modList->addItem( item );

	// add it also to the current preset
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedPresetIdx >= 0) {
		presets[ selectedPresetIdx ].mods.append({ name, {path}, true });
	}

	generateLaunchCommand();
}

void MainWindow::updateIWADsFromDir()
{
	iwads.clear();  // TODO: optimize to not reinsert everything when only 1 file was added
	if (!iwadDir.isEmpty()) {
		QDir dir( iwadDir );
		if (dir.exists()) {
			QDirIterator dirIt( dir );
			while (dirIt.hasNext()) {
				dirIt.next();
				if (!dirIt.fileInfo().isDir()) {
					iwads.append({ dirIt.fileName(), pathHelper.convertPath( dirIt.filePath() ) });
				}
			}
		}
	}
}

void MainWindow::updateMapPacksFromDir()
{
	maps.clear();  // TODO: optimize to not reinsert everything when only 1 file was added
	if (!mapDir.isEmpty()) {
		QDir dir( mapDir );
		if (dir.exists()) {
			QDirIterator dirIt( mapDir );
			while (dirIt.hasNext()) {
				dirIt.next();
				if (!dirIt.fileInfo().isDir()) {
					maps.append({ dirIt.fileName() });
				}
			}
		}
	}
}

void MainWindow::updateSaveFilesFromDir()
{
	if (ui->engineCmbBox->currentIndex() < 0)
		return;

	QString curText = ui->saveFileCmbBox->currentText();

	ui->saveFileCmbBox->clear();

	QFileInfo info( engines[ ui->engineCmbBox->currentIndex() ].path );
	QDir dir = info.dir();
	QDirIterator dirIt( dir );
	while (dirIt.hasNext()) {
		QFileInfo entry( dirIt.next() );
		if (!entry.isDir() && entry.completeSuffix() == "zds")
			ui->saveFileCmbBox->addItem( entry.fileName() );
	}

	ui->saveFileCmbBox->setCurrentText( curText );
}

void MainWindow::updateMapsFromIWAD()
{
	int iwadIdx = getSelectedItemIdx( ui->iwadListView );
	if (iwadIdx < 0)
		return;

	QString text = iwads[ iwadIdx ].name;

	if ((text.compare( "doom.wad", Qt::CaseInsensitive ) == 0 || text.startsWith( "doom1" , Qt::CaseInsensitive ))
	&& !ui->mapCmbBox->itemText(0).startsWith('E')) {
		ui->mapCmbBox->clear();
		for (int i = 1; i <= 9; i++)
			ui->mapCmbBox->addItem( QStringLiteral("E1M%1").arg(i) );
		for (int i = 1; i <= 9; i++)
			ui->mapCmbBox->addItem( QStringLiteral("E1M%1").arg(i) );
		for (int i = 1; i <= 9; i++)
			ui->mapCmbBox->addItem( QStringLiteral("E1M%1").arg(i) );
	} else if (!ui->mapCmbBox->itemText(0).startsWith('M')) {
		ui->mapCmbBox->clear();
		for (int i = 1; i <= 32; i++)
			ui->mapCmbBox->addItem( QStringLiteral("MAP%1").arg( i, 2, 10, QChar('0') ) );
	}
}

void MainWindow::updateListsFromDirs()
{
	if (iwadListFromDir) {
		updateIWADsFromDir();
		iwadModel.updateUI(0);
	}
	updateMapPacksFromDir();
	mapModel.updateUI(0);
	updateSaveFilesFromDir();
}

void MainWindow::toggleAbsolutePaths( bool absolute )
{
	pathHelper.toggleAbsolutePaths( absolute );

	for (Engine & engine : engines)
		engine.path = pathHelper.convertPath( engine.path );

	if (iwadListFromDir && !iwadDir.isEmpty()) {
		iwadDir = pathHelper.convertPath( iwadDir );
	} else {
		for (IWAD & iwad : iwads)
			iwad.path = pathHelper.convertPath( iwad.path );
	}

	mapDir = pathHelper.convertPath( mapDir );

	modDir = pathHelper.convertPath( modDir );
	for (ModInfo & info : modInfo)
		info.path = pathHelper.convertPath( info.path );

	for (Preset & preset : presets)
		for (Preset::ModEntry & entry : preset.mods)
			entry.info.path = pathHelper.convertPath( entry.info.path );

	generateLaunchCommand();
}

void MainWindow::modeGameMenu()
{
	ui->mapCmbBox->setEnabled( false );
	ui->saveFileCmbBox->setEnabled( false );
	ui->skillCmbBox->setEnabled( false );
	ui->skillSpinBox->setEnabled( false );
	ui->noMonstersChkBox->setEnabled( false );
	ui->fastMonstersChkBox->setEnabled( false );
	ui->monstersRespawnChkBox->setEnabled( false );
	ui->dmFlagsBtn->setEnabled( false );
	ui->compatFlagsBtn->setEnabled( false );

	ui->multiplayerChkBox->setChecked( false );

	generateLaunchCommand();
}

void MainWindow::modeSelectedMap()
{
	ui->mapCmbBox->setEnabled( true );
	ui->saveFileCmbBox->setEnabled( false );
	ui->skillCmbBox->setEnabled( true );
	ui->skillSpinBox->setEnabled( ui->skillCmbBox->currentIndex() == Skill::CUSTOM );
	ui->noMonstersChkBox->setEnabled( true );
	ui->fastMonstersChkBox->setEnabled( true );
	ui->monstersRespawnChkBox->setEnabled( true );
	ui->dmFlagsBtn->setEnabled( true );
	ui->compatFlagsBtn->setEnabled( true );

	generateLaunchCommand();
}

void MainWindow::modeSavedGame()
{
	ui->mapCmbBox->setEnabled( false );
	ui->saveFileCmbBox->setEnabled( true );
	ui->skillCmbBox->setEnabled( false );
	ui->skillSpinBox->setEnabled( false );
	ui->noMonstersChkBox->setEnabled( false );
	ui->fastMonstersChkBox->setEnabled( false );
	ui->monstersRespawnChkBox->setEnabled( false );
	ui->dmFlagsBtn->setEnabled( false );
	ui->compatFlagsBtn->setEnabled( false );

	generateLaunchCommand();
}

void MainWindow::selectMap( QString )
{
	generateLaunchCommand();
}

void MainWindow::selectSavedGame( QString )
{
	generateLaunchCommand();
}

void MainWindow::selectSkill( int skill )
{
	ui->skillSpinBox->setValue( skill );
	ui->skillSpinBox->setEnabled( skill == Skill::CUSTOM );

	generateLaunchCommand();
}

void MainWindow::changeSkillNum( int skill )
{
	if (skill < Skill::CUSTOM)
		ui->skillCmbBox->setCurrentIndex( skill );

	generateLaunchCommand();
}

void MainWindow::toggleNoMonsters( bool )
{
	generateLaunchCommand();
}

void MainWindow::toggleFastMonsters( bool )
{
	generateLaunchCommand();
}

void MainWindow::toggleMonstersRespawn( bool )
{
	generateLaunchCommand();
}

void MainWindow::toggleMultiplayer( bool checked )
{
	ui->multRoleCmbBox->setEnabled( checked );
	ui->hostnameLine->setEnabled( checked );
	ui->portSpinBox->setEnabled( checked );
	ui->netModeCmbBox->setEnabled( checked );
	ui->gameModeCmbBox->setEnabled( checked );
	ui->playerCountSpinBox->setEnabled( checked );
	ui->teamDmgSpinBox->setEnabled( checked );
	ui->timeLimitSpinBox->setEnabled( checked );

	if (checked && ui->launchMode_menu->isChecked())
		ui->launchMode_map->click();

	generateLaunchCommand();
}

void MainWindow::selectMultRole( int )
{
	generateLaunchCommand();
}

void MainWindow::changeHost( QString )
{
	generateLaunchCommand();
}

void MainWindow::changePort( int )
{
	generateLaunchCommand();
}

void MainWindow::selectNetMode( int )
{
	generateLaunchCommand();
}

void MainWindow::selectGameMode( int )
{
	generateLaunchCommand();
}

void MainWindow::changePlayerCount( int )
{
	generateLaunchCommand();
}

void MainWindow::changeTeamDamage( double )
{
	generateLaunchCommand();
}

void MainWindow::changeTimeLimit( int )
{
	generateLaunchCommand();
}

void MainWindow::updateAdditionalArgs( QString )
{
	generateLaunchCommand();
}


//----------------------------------------------------------------------------------------------------------------------
//  options

void MainWindow::saveOptions( QString fileName )
{
	QFile file( fileName );
	if (!file.open( QIODevice::WriteOnly )) {
		QMessageBox::warning( this, "Can't open file", "Saving options failed. Could not open file "%fileName%" for writing." );
	}

	QJsonObject json;

	// window geometry
	{
		const QRect & geometry = this->geometry();
		json["width"] = geometry.width();
		json["height"] = geometry.height();
	}

	// engines
	{
		QJsonObject jsEngines;

		QJsonArray jsPortArray;
		for (const Engine & engine : engines) {
			QJsonObject jsPort;
			jsPort["name"] = engine.name;
			jsPort["path"] = engine.path;
			jsPortArray.append( jsPort );
		}
		jsEngines["engines"] = jsPortArray;
		jsEngines["selected"] = ui->engineCmbBox->currentIndex();

		json["engines"] = jsEngines;
	}

    // IWADs
	{
		QJsonObject jsIWADs;

		jsIWADs["auto_update"] = iwadListFromDir;
		jsIWADs["directory"] = iwadDir;
		QJsonArray jsIWADArray;
		for (const IWAD & iwad : iwads) {
			QJsonObject jsIWAD;
			jsIWAD["name"] = iwad.name;
			jsIWAD["path"] = iwad.path;
			jsIWADArray.append( jsIWAD );
		}
		jsIWADs["IWADs"] = jsIWADArray;

		json["IWADs"] = jsIWADs;
    }

    // map packs
	{
		QJsonObject jsMaps;
		jsMaps["directory"] = mapDir;
		json["maps"] = jsMaps;
	}

    // mods
	{
		QJsonObject jsMods;
		jsMods["directory"] = modDir;
		json["mods"] = jsMods;
	}

	// presets
	{
		QJsonArray jsPresetArray;

		for (const Preset & preset : presets) {
			QJsonObject jsPreset;

			jsPreset["name"] = preset.name;
			jsPreset["selectedEngine"] = preset.selectedEnginePath;
			jsPreset["selectedIWAD"] = preset.selectedIWAD;
			//jsPreset["selectedMapPack"] = preset.selectedMapPack;
			QJsonArray jsModArray;
			for (const Preset::ModEntry & entry : preset.mods) {
				QJsonObject jsMod;
				jsMod["name"] = entry.name;
				jsMod["path"] = entry.info.path;
				jsMod["checked"] = entry.checked;
				jsModArray.append( jsMod );
			}
			jsPreset["mods"] = jsModArray;

			jsPresetArray.append( jsPreset );
		}

		json["presets"] = jsPresetArray;
	}

	// additional command line arguments
	json["additional_args"] = ui->cmdArgsLine->text();

	// launch options
	json["dmflags1"] = qint64( dmflags1 );
	json["dmflags2"] = qint64( dmflags2 );
	json["compatflags1"] = qint64( compatflags1 );
	json["compatflags2"] = qint64( compatflags2 );

	// write the json to file
	QJsonDocument jsonDoc( json );
	file.write( jsonDoc.toJson() );
    file.close();

    //return file.error() == QFile::NoError;
}

void MainWindow::loadOptions( QString fileName )
{
	QFile file( fileName );
	if (!file.open( QIODevice::ReadOnly )) {
		QMessageBox::warning( this, "Can't open file", "Loading options failed. Could not open file "+fileName+" for reading." );
		return;
	}

	QJsonParseError error;
	QJsonDocument jsonDoc = QJsonDocument::fromJson( file.readAll(), &error );
	QJsonObject json = jsonDoc.object();

	// window geometry
	{
		width = getInt( json, "width", -1 );
		height = getInt( json, "height", -1 );
		if (shown && width > 0 && height > 0) {  // the window has been already shown and has the final position, so we can change the dimensions
			const QRect & geometry = this->geometry();
			this->setGeometry( geometry.x(), geometry.y(), width, height );
		} // otherwise we need to do this in showEvent callback
	}

	// engines
	{
		QJsonObject jsEngines = getObject( json, "engines" );

		engines.clear();
		QJsonArray jsEngineArray = getArray( jsEngines, "engines" );
		for (int i = 0; i < jsEngineArray.size(); i++)
		{
			QJsonObject jsEngine = getObject( jsEngineArray, i );
			if (jsEngine.isEmpty())  // wrong type on position i - skip this entry
				continue;

			QString name = getString( jsEngine, "name" );
			QString path = getString( jsEngine, "path" );
			if (name.isEmpty() || path.isEmpty())  // name or path doesn't exist - skip this entry
				continue;

			if (QFileInfo( path ).exists())
				engines.append({ name, path });
			else
				QMessageBox::warning( this, "Engine no longer exists",
					"An engine from the saved options ("%path%") no longer exists. It will be removed from the list." );
		}
		engineModel.updateUI(0);
		ui->engineCmbBox->setCurrentIndex( getInt( jsEngines, "selected", 0 ) );
	}

	// IWADS
	{
		QJsonObject jsIWADs = getObject( json, "IWADs" );

		iwadListFromDir = getBool( jsIWADs, "auto_update", false );

		if (iwadListFromDir) {
			QString dir = getString( jsIWADs, "directory" );
			if (!dir.isEmpty()) {  // non-existing element directory - skip completely
				if (QDir( dir ).exists())
					iwadDir = dir;
				else
					QMessageBox::warning( this, "IWAD dir no longer exists",
						"IWAD directory from the saved options ("%dir%") no longer exists. Please update it in Menu -> Setup." );
				updateIWADsFromDir();
			}
		} else {
			QJsonArray jsIWADArray = getArray( jsIWADs, "IWADs" );
			deselectSelectedItem( ui->iwadListView );
			selectedIWADIdx = -1;
			iwads.clear();
			for (int i = 0; i < jsIWADArray.size(); i++)
			{
				QJsonObject jsIWAD = getObject( jsIWADArray, i );
				if (jsIWAD.isEmpty())  // wrong type on position i - skip this entry
					continue;

				QString name = getString( jsIWAD, "name" );
				QString path = getString( jsIWAD, "path" );
				if (name.isEmpty() || path.isEmpty())  // name or path doesn't exist - skip this entry
					continue;

				if (QFileInfo( path ).exists())
					iwads.append({ name, path });
				else
					QMessageBox::warning( this, "IWAD no longer exists",
						"An IWAD from the saved options ("%path%") no longer exists. It will be removed from the list." );
			}
		}

		iwadModel.updateUI(0);
	}

	// map packs
	{
		QJsonObject jsMaps = getObject( json, "maps" );
		QString dir = getString( jsMaps, "directory" );
		if (!dir.isEmpty()) {  // non-existing element directory - skip completely
			if (QDir( dir ).exists())
				mapDir = dir;
			else
				QMessageBox::warning( this, "Map dir no longer exists",
					"Map directory from the saved options ("%dir%") no longer exists. Please update it in Menu -> Setup." );
		}
	}

	// mods
	{
		QJsonObject jsMods = getObject( json, "mods" );
		QString dir = getString( jsMods, "directory" );
		if (!dir.isEmpty()) {  // non-existing element directory - skip completely
			if (QDir( dir ).exists())
				modDir = dir;
			else
				QMessageBox::warning( this, "Mod dir no longer exists",
					"Mod directory from the saved options ("%dir%") no longer exists. Please update it in Menu -> Setup." );
		}
	}

	// presets
	{
		QJsonArray jsPresetArray = getArray( json, "presets" );
		deselectSelectedItem( ui->presetListView );
		presets.clear();
		for (int i = 0; i < jsPresetArray.size(); i++)
		{
			QJsonObject jsPreset = getObject( jsPresetArray, i );
			if (jsPreset.isEmpty())  // wrong type on position i - skip this entry
				continue;

			Preset preset;
			preset.name = getString( jsPreset, "name", "<missing name>" );
			preset.selectedEnginePath = getString( jsPreset, "selectedEngine" );
			preset.selectedIWAD = getString( jsPreset, "selectedIWAD" );
			//preset.selectedMapPack = getString( jsPreset, "selectedMapPack" );
			QJsonArray jsModArray = getArray( jsPreset, "mods" );
			for (int i = 0; i < jsModArray.size(); i++)
			{
				QJsonObject jsMod = getObject( jsModArray, i );
				if (jsMod.isEmpty())  // wrong type on position i - skip this entry
					continue;

				Preset::ModEntry entry;
				entry.name = getString( jsMod, "name" );
				entry.info.path = getString( jsMod, "path" );
				entry.checked = getBool( jsMod, "checked", false );
				if (!entry.name.isEmpty() && !entry.info.path.isEmpty())
					preset.mods.append( entry );
			}
			presets.append( preset );
		}
		presetModel.updateUI(0);
	}

	// launch options
	dmflags1 = getUInt( json, "dmflags1", 0 );
	dmflags2 = getUInt( json, "dmflags2", 0 );
	compatflags1 = getUInt( json, "compatflags1", 0 );
	compatflags2 = getUInt( json, "compatflags2", 0 );

    file.close();

	updateListsFromDirs();

	generateLaunchCommand();
}

void MainWindow::exportPreset()
{
	int selectedIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedIdx < 0) {
		QMessageBox::warning( this, "No preset selected", "Select a preset from the preset list." );
		return;
	}

	QString fileName = QFileDialog::getSaveFileName( this, "Save preset", QString(), scriptFileExt );
	if (fileName.isEmpty()) {  // user probably clicked cancel
		return;
	}

	QFile file( fileName );
	if (!file.open( QIODevice::ReadWrite )) {
		QMessageBox::warning( this, "Cannot open file", "Cannot open file for writing. Check directory permissions" );
		return;
	}

	QTextStream stream( &file );

	// TODO: convert paths according to the target directory
	stream << ui->commandLine->text() << endl;

	file.close();
}

void MainWindow::importPreset()
{
	QMessageBox::warning( this, "Not implemented", "Sorry, this feature is not implemented yet" );
}


//----------------------------------------------------------------------------------------------------------------------
//  launch command generation

void MainWindow::generateLaunchCommand()
{
	QString command;
	QTextStream cmdStream( &command );
	int index;

	index = ui->engineCmbBox->currentIndex();
	if (index >= 0) {
		cmdStream << "\"" << engines[ index ].path << "\"";
	}

	index = getSelectedItemIdx( ui->iwadListView );
	if (index >= 0) {
		cmdStream << " -iwad \"" << iwads[ index ].path << "\"";
	}

	index = getSelectedItemIdx( ui->mapListView );
	if (index >= 0) {
		cmdStream << " -file \"" << QDir( mapDir ).filePath( maps[ index ].name ) << "\"";
	}

	for (int i = 0; i < ui->modList->count(); i++) {
		QListWidgetItem * item = ui->modList->item(i);
		if (item->checkState() == Qt::Checked) {
			cmdStream << " -file \"" << modInfo[ item->text() ].path << "\"";
		}
	}

	if (ui->launchMode_map->isChecked()) {
		cmdStream << " -warp " << getMapNumber( ui->mapCmbBox->currentText() );
		cmdStream << " -skill " << ui->skillSpinBox->text();
		if (ui->noMonstersChkBox->isChecked())
			cmdStream << " -nomonsters";
		if (ui->fastMonstersChkBox->isChecked())
			cmdStream << " -fast";
		if (ui->monstersRespawnChkBox->isChecked())
			cmdStream << " -respawn";
		if (dmflags1 != 0)
			cmdStream << " +dmflags " << QString::number( dmflags1 );
		if (dmflags2 != 0)
			cmdStream << " +dmflags2 " << QString::number( dmflags2 );
		if (compatflags1 != 0)
			cmdStream << " +compatflags " << QString::number( compatflags1 );
		if (compatflags2 != 0)
			cmdStream << " +compatflags2 " << QString::number( compatflags2 );
	} else if (ui->launchMode_savefile->isChecked()) {
		cmdStream << " -loadgame " << ui->saveFileCmbBox->currentText();
	}

	if (ui->multiplayerChkBox->isChecked()) {
		switch (ui->multRoleCmbBox->currentIndex()) {
		 case MultRole::SERVER:
			cmdStream << " -host " << ui->playerCountSpinBox->text();
			if (ui->portSpinBox->value() != 5029)
				cmdStream << " -port " << ui->portSpinBox->text();
			switch (ui->gameModeCmbBox->currentIndex()) {
			 case DEATHMATCH:
				cmdStream << " -deathmatch";
				break;
			 case TEAM_DEATHMATCH:
				cmdStream << " -deathmatch +teamplay";
				break;
			 case ALT_DEATHMATCH:
				cmdStream << " -altdeath";
				break;
			 case ALT_TEAM_DEATHMATCH:
				cmdStream << " -altdeath +teamplay";
				break;
			 case COOPERATIVE: // default mode, which is started without any param
				break;
			 default:
				QMessageBox::critical( this, "Invalid game mode index",
					"The game mode index is out of range. This shouldn't be happening and it is a bug. Please create an issue on Github page." );
			}
			if (ui->teamDmgSpinBox->value() != 0.0)
				cmdStream << " +teamdamage " << QString::number( ui->teamDmgSpinBox->value(), 'f', 2 );
			if (ui->timeLimitSpinBox->value() != 0)
				cmdStream << " -timer " << ui->timeLimitSpinBox->text();
			cmdStream << " -netmode " << QString::number( ui->netModeCmbBox->currentIndex() );
			break;
		 case MultRole::CLIENT:
			cmdStream << " -join " << ui->hostnameLine->text() << ":" << ui->portSpinBox->text();
			break;
		 default:
			QMessageBox::critical( this, "Invalid multiplayer role index",
				"The multiplayer role index is out of range. This shouldn't be happening and it is a bug. Please create an issue on Github page." );
		}
	}

	if (!ui->cmdArgsLine->text().isEmpty()) {
		cmdStream << " " << ui->cmdArgsLine->text();
	}

	cmdStream.flush();
	ui->commandLine->setText( command );
}

void MainWindow::launch()
{
	if (ui->engineCmbBox->currentIndex() < 0) {
		QMessageBox::warning( this, "No engine selected", "No Doom engine is selected." );
		return;
	}

	bool success = QProcess::startDetached( ui->commandLine->text() );
	if (!success)
		QMessageBox::warning( this, tr("Launch error"), tr("Failed to execute launch command.") );
}


//----------------------------------------------------------------------------------------------------------------------
//  events

void MainWindow::closeEvent( QCloseEvent * event )
{
	saveOptions( defaultOptionsFile );

	QWidget::closeEvent( event );
}

void MainWindow::timerEvent( QTimerEvent * )  // called once per second
{
	tickCount++;

	if (tickCount % 2 == 0) {
		updateListsFromDirs();
	}

	if (tickCount % 60 == 0) {
		saveOptions( defaultOptionsFile );
	}
}
