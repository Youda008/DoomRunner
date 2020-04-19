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
#include "GameOptsDialog.hpp"
#include "CompatOptsDialog.hpp"
#include "AboutDialog.hpp"

#include "LangUtils.hpp"
#include "JsonUtils.hpp"
#include "WidgetUtils.hpp"
#include "DoomUtils.hpp"

#include <QString>
#include <QStringBuilder>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QProcess>
#include <QDesktopWidget>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>

#include <QDebug>


//======================================================================================================================

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
//  MainWindow

MainWindow::MainWindow()

	: QMainWindow( nullptr )
	, tickCount( 0 )
	, optionsCorrupted( false )
	, pathHelper( false, QDir::currentPath() )
	, engineModel(
		/*makeDisplayString*/[]( const Engine & engine ) { return engine.name; }
	  )
	, configModel(
		/*makeDisplayString*/[]( const ConfigFile & config ) { return config.fileName; }
	  )
	, saveModel(
		/*makeDisplayString*/[]( const SaveFile & save ) { return save.fileName; }
	  )
	, iwadModel(
		/*makeDisplayString*/[]( const IWAD & iwad ) { return iwad.name; }
	  )
	, iwadListFromDir( false )
	, iwadSubdirs( false )
	, mapModel()
	, modModel(
		/*makeDisplayString*/[]( const Mod & mod ) { return mod.fileName; }
	  )
	, presetModel(
		/*makeDisplayString*/[]( const Preset & preset ) { return preset.name; }
	  )
{
	ui = new Ui::MainWindow;
	ui->setupUi( this );

	// setup view models
	ui->presetListView->setModel( &presetModel );
	// we use custom model for engines, because we want to display the same list differently in different window
	ui->engineCmbBox->setModel( &engineModel );
	// we use custom model for configs and saves, because calling 'clear' or 'add' on a combo box changes current index,
	// which causes the launch command to be regenerated back and forth on every update
	ui->configCmbBox->setModel( &configModel );
	ui->saveFileCmbBox->setModel( &saveModel );
	ui->iwadListView->setModel( &iwadModel );
	ui->mapDirView->setModel( &mapModel );
	ui->modListView->setModel( &modModel );

	// setup preset list view
	presetModel.setEditStringFunc( []( Preset & preset ) -> QString & { return preset.name; } );
	ui->presetListView->toggleNameEditing( true );
	ui->presetListView->toggleIntraWidgetDragAndDrop( true );
	ui->presetListView->toggleInterWidgetDragAndDrop( false );
	ui->presetListView->toggleExternalFileDragAndDrop( false );

	// setup map directory view
	ui->mapDirView->setDragEnabled( true );
	ui->mapDirView->setDragDropMode( QAbstractItemView::DragOnly );

	// setup mod list view
	modModel.setIsCheckedFunc( []( Mod & mod ) -> bool & { return mod.checked; } );
	modModel.setPathHelper( &pathHelper );  // model needs it for converting paths dropped from directory
	ui->modListView->toggleNameEditing( false );
	ui->modListView->toggleIntraWidgetDragAndDrop( true );
	ui->modListView->toggleInterWidgetDragAndDrop( true );
	ui->modListView->toggleExternalFileDragAndDrop( true );
	connect( ui->modListView, QOverload< int, int >::of( &EditableListView::itemsDropped ), this, &thisClass::modsDropped );

	// setup signals
	connect( ui->setupPathsAction, &QAction::triggered, this, &thisClass::runSetupDialog );
	connect( ui->exportPresetAction, &QAction::triggered, this, &thisClass::exportPreset );
	//connect( ui->importPresetAction, &QAction::triggered, this, &thisClass::importPreset );
	connect( ui->aboutAction, &QAction::triggered, this, &thisClass::runAboutDialog );
	connect( ui->exitAction, &QAction::triggered, this, &thisClass::close );

	connect( ui->engineCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectEngine );
	connect( ui->configCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectConfig );
	connect( ui->presetListView, &QListView::clicked, this, &thisClass::loadPreset );
	connect( ui->iwadListView, &QListView::clicked, this, &thisClass::toggleIWAD );
	connect( ui->mapDirView, &QListView::clicked, this, &thisClass::toggleMapPack );
	connect( ui->modListView, &QListView::clicked, this, &thisClass::toggleMod );

	connect( ui->presetBtnAdd, &QToolButton::clicked, this, &thisClass::presetAdd );
	connect( ui->presetBtnDel, &QToolButton::clicked, this, &thisClass::presetDelete );
	connect( ui->presetBtnClone, &QToolButton::clicked, this, &thisClass::presetClone );
	connect( ui->presetBtnUp, &QToolButton::clicked, this, &thisClass::presetMoveUp );
	connect( ui->presetBtnDown, &QToolButton::clicked, this, &thisClass::presetMoveDown );

	connect( ui->modBtnAdd, &QToolButton::clicked, this, &thisClass::modAdd );
	connect( ui->modBtnDel, &QToolButton::clicked, this, &thisClass::modDelete );
	connect( ui->modBtnUp, &QToolButton::clicked, this, &thisClass::modMoveUp );
	connect( ui->modBtnDown, &QToolButton::clicked, this, &thisClass::modMoveDown );

	connect( ui->launchMode_standard, &QRadioButton::clicked, this, &thisClass::modeStandard );
	connect( ui->launchMode_map, &QRadioButton::clicked, this, &thisClass::modeSelectedMap );
	connect( ui->launchMode_savefile, &QRadioButton::clicked, this, &thisClass::modeSavedGame );
	connect( ui->mapCmbBox, &QComboBox::currentTextChanged, this, &thisClass::selectMap );
	connect( ui->saveFileCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectSavedGame );
	connect( ui->skillCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectSkill );
	connect( ui->skillSpinBox, QOverload<int>::of( &QSpinBox::valueChanged ), this, &thisClass::changeSkillNum );
	connect( ui->noMonstersChkBox, &QCheckBox::toggled, this, &thisClass::toggleNoMonsters );
	connect( ui->fastMonstersChkBox, &QCheckBox::toggled, this, &thisClass::toggleFastMonsters );
	connect( ui->monstersRespawnChkBox, &QCheckBox::toggled, this, &thisClass::toggleMonstersRespawn );
	connect( ui->gameOptsBtn, &QPushButton::clicked, this, &thisClass::runGameOptsDialog );
	connect( ui->compatOptsBtn, &QPushButton::clicked, this, &thisClass::runCompatOptsDialog );

	connect( ui->multiplayerChkBox, &QCheckBox::toggled, this, &thisClass::toggleMultiplayer );
	connect( ui->multRoleCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectMultRole );
	connect( ui->hostnameLine, &QLineEdit::textChanged, this, &thisClass::changeHost );
	connect( ui->portSpinBox, QOverload<int>::of( &QSpinBox::valueChanged ), this, &thisClass::changePort );
	connect( ui->netModeCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectNetMode );
	connect( ui->gameModeCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectGameMode );
	connect( ui->playerCountSpinBox, QOverload<int>::of( &QSpinBox::valueChanged ), this, &thisClass::changePlayerCount );
	connect( ui->teamDmgSpinBox, QOverload<double>::of( &QDoubleSpinBox::valueChanged ), this, &thisClass::changeTeamDamage );
	connect( ui->timeLimitSpinBox, QOverload<int>::of( &QSpinBox::valueChanged ), this, &thisClass::changeTimeLimit );

	connect( ui->presetCmdArgsLine, &QLineEdit::textChanged, this, &thisClass::updatePresetCmdArgs );
	connect( ui->globalCmdArgsLine, &QLineEdit::textChanged, this, &thisClass::updateGlobalCmdArgs );
	connect( ui->launchBtn, &QPushButton::clicked, this, &thisClass::launch );

	// This will call the function when the window is fully initialized and displayed.
	// Not sure, which one of these 2 options is better.
	QMetaObject::invokeMethod( this, &thisClass::onWindowShown, Qt::ConnectionType::QueuedConnection );
	//QTimer::singleShot( 0, this, &thisClass::onWindowShown );
}

void MainWindow::onWindowShown()
{
	// In the constructor, some properties of the window are not yet initialized, like window dimensions,
	// so we have to do this here, when the window is already fully loaded.

	// try to load last saved state
	if (QFileInfo( defaultOptionsFile ).exists())
		loadOptions( defaultOptionsFile );
	else  // this is a first run, perform an initial setup
		runSetupDialog();

	// setup an update timer
	startTimer( 1000 );
}

void MainWindow::timerEvent( QTimerEvent * event )  // called once per second
{
	QMainWindow::timerEvent( event );

	tickCount++;

 #ifdef QT_DEBUG
	constexpr uint dirUpdateDelay = 8;
 #else
	constexpr uint dirUpdateDelay = 2;
 #endif
	if (tickCount % dirUpdateDelay == 0) {
		updateListsFromDirs();
	}

	if (tickCount % 60 == 0) {
		if (!optionsCorrupted)  // don't overwrite existing file with empty data, when there was just one small syntax error
			saveOptions( defaultOptionsFile );
	}
}

void MainWindow::closeEvent( QCloseEvent * event )
{
	if (!optionsCorrupted)  // don't overwrite existing file with empty data, when there was just one small syntax error
		saveOptions( defaultOptionsFile );

	QMainWindow::closeEvent( event );
}

MainWindow::~MainWindow()
{
	delete ui;
}


//----------------------------------------------------------------------------------------------------------------------
//  dialogs

void MainWindow::runSetupDialog()
{
	// The dialog gets a copy of all the required data and when it's confirmed, we copy them back.
	// This allows the user to cancel all the changes he made without having to manually revert them.
	// Secondly, this removes all the problems with data synchronization like dangling pointers
	// or that previously selected items in the view no longer exist.
	// And thirdly, we no longer have to split the data itself from the logic of displaying them.

	SetupDialog dialog( this,
		pathHelper.useAbsolutePaths(),
		pathHelper.baseDir(),
		engineModel.list(),
		iwadModel.list(),
		iwadListFromDir,
		iwadDir,
		iwadSubdirs,
		mapDir,
		modDir
	);

	int code = dialog.exec();

	// update the data only if user clicked Ok
	if (code == QDialog::Accepted)
	{
		// write down the previously selected items
		auto selectedEngine = getSelectedItemID( ui->engineCmbBox, engineModel );
		auto selectedIWAD = getSelectedItemID( ui->iwadListView, iwadModel );

		// deselect the items
		ui->engineCmbBox->setCurrentIndex( -1 );
		deselectSelectedItems( ui->iwadListView );

		// make sure all data and indexes are invalidated and no longer used
		engineModel.startCompleteUpdate();
		iwadModel.startCompleteUpdate();

		// update our data from the dialog
		pathHelper = dialog.pathHelper;
		engineModel.updateList( dialog.engineModel.list() );
		iwadModel.updateList( dialog.iwadModel.list() );
		iwadListFromDir = dialog.iwadListFromDir;
		iwadDir = dialog.iwadDir;
		iwadSubdirs = dialog.iwadSubdirs;
		mapDir = dialog.mapDir;
		modDir = dialog.modDir;
		// update all stored paths
		toggleAbsolutePaths( pathHelper.useAbsolutePaths() );
		selectedEngine = pathHelper.convertPath( selectedEngine );
		selectedIWAD = pathHelper.convertPath( selectedIWAD );

		// notify the widgets to re-draw their content
		engineModel.finishCompleteUpdate();
		iwadModel.finishCompleteUpdate();

		// select back the previously selected items
		selectItemByID( ui->engineCmbBox, engineModel, selectedEngine );
		selectItemByID( ui->iwadListView, iwadModel, selectedIWAD );

		updateMapPacksFromDir();

		updateLaunchCommand();
	}
}

void MainWindow::runGameOptsDialog()
{
	GameOptsDialog dialog( this, gameOpts );

	int code = dialog.exec();

	// update the data only if user clicked Ok
	if (code == QDialog::Accepted) {
		gameOpts = dialog.gameOpts;
		updateLaunchCommand();
	}
}

void MainWindow::runCompatOptsDialog()
{
	CompatOptsDialog dialog( this, compatOpts );

	int code = dialog.exec();

	// update the data only if user clicked Ok
	if (code == QDialog::Accepted) {
		compatOpts = dialog.compatOpts;
		// cache the command line args string, so that it doesn't need to be regenerated on every command line update
		compatOptsCmdArgs = CompatOptsDialog::getCmdArgsFromOptions( compatOpts );
		updateLaunchCommand();
	}
}

void MainWindow::runAboutDialog()
{
	AboutDialog dialog( this );

	dialog.exec();
}


//----------------------------------------------------------------------------------------------------------------------
//  list item selection

void MainWindow::loadPreset( const QModelIndex & index )
{
	Preset & preset = presetModel[ index.row() ];

	// enable all widgets that contain preset settings
	togglePresetSubWidgets( true );

	// restore selected engine
	if (!preset.selectedEnginePath.isEmpty()) {  // the engine combo box might have been empty when creating this preset
		int engineIdx = findSuch( engineModel.list(), [ &preset ]( const Engine & engine )
		                                                         { return engine.path == preset.selectedEnginePath; } );
		if (engineIdx >= 0) {
			ui->engineCmbBox->setCurrentIndex( engineIdx );
		} else {
			ui->engineCmbBox->setCurrentIndex( -1 );
			QMessageBox::warning( this, "Engine no longer exists",
				"Engine selected for this preset ("%preset.selectedEnginePath%") no longer exists, please select another one." );
		}
	} else {
		ui->engineCmbBox->setCurrentIndex( -1 );
	}

	// restore selected config
	if (!preset.selectedConfig.isEmpty()) {  // the preset combo box might have been empty when creating this preset
		int configIdx = findSuch( configModel.list(), [ &preset ]( const ConfigFile & config )
		                                                         { return config.fileName == preset.selectedConfig; } );
		if (configIdx >= 0) {
			ui->configCmbBox->setCurrentIndex( configIdx );
		} else {
			ui->configCmbBox->setCurrentIndex( -1 );
			QMessageBox::warning( this, "Config no longer exists",
				"Config selected for this preset ("%preset.selectedConfig%") no longer exists, please select another one." );
		}
	} else {
		ui->configCmbBox->setCurrentIndex( -1 );
	}

	// restore selected IWAD
	deselectSelectedItems( ui->iwadListView );
	selectedIWAD.clear();
	if (!preset.selectedIWAD.isEmpty()) {  // the IWAD may have not been selected when creating this preset
		int iwadIdx = findSuch( iwadModel.list(), [ &preset ]( const IWAD & iwad )
		                                                     { return iwad.name == preset.selectedIWAD; } );
		if (iwadIdx >= 0) {
			selectItemByIdx( ui->iwadListView, iwadIdx );
			selectedIWAD = preset.selectedIWAD;
			updateMapsFromIWAD();
		} else {
			QMessageBox::warning( this, "IWAD no longer exists",
				"IWAD selected for this preset ("%preset.selectedIWAD%") no longer exists, please select another one." );
		}
	}

	// restore selected MapPack
	deselectSelectedItems( ui->mapDirView );
	if (!preset.selectedMapPacks.isEmpty()) {
		for (const TreePosition & pos : preset.selectedMapPacks) {
			QModelIndex mapIdx = mapModel.getNodeByPosition( pos );
			if (mapIdx.isValid()) {
				selectItemByIdx( ui->mapDirView, mapIdx );
			} else {
				QMessageBox::warning( this, "Map file no longer exists",
					"Map file selected for this preset ("%pos.toString()%") no longer exists." );
			}
		}
	}

	// restore list of mods
	deselectSelectedItems( ui->modListView );
	modModel.startCompleteUpdate();
	modModel.clear();
	for (auto modIt = preset.mods.begin(); modIt != preset.mods.end(); )  // need iterator, so that we can erase non-existing
	{
		const Mod & mod = *modIt;

		if (!QFileInfo( mod.path ).exists()) {
			QMessageBox::warning( this, "Mod no longer exists",
				"A mod from the preset ("%mod.path%") no longer exists. It will be removed from the list." );
			modIt = preset.mods.erase( modIt );  // keep the list widget in sync with the preset list
			continue;
		}

		modModel.append( mod );
		modIt++;
	}
	modModel.finishCompleteUpdate();

	// restore additional command line arguments
	ui->presetCmdArgsLine->setText( preset.cmdArgs );

	updateLaunchCommand();
}

void MainWindow::togglePresetSubWidgets( bool enabled )
{
	ui->engineCmbBox->setEnabled( enabled );
	ui->configCmbBox->setEnabled( enabled );
	ui->iwadListView->setEnabled( enabled );
	ui->mapDirView->setEnabled( enabled );
	ui->modListView->setEnabled( enabled );
	ui->modBtnAdd->setEnabled( enabled );
	ui->modBtnDel->setEnabled( enabled );
	ui->modBtnUp->setEnabled( enabled );
	ui->modBtnDown->setEnabled( enabled );
	ui->presetCmdArgsLine->setEnabled( enabled );
}

void MainWindow::selectEngine( int index )
{
	// update the current preset
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedPresetIdx >= 0) {
		if (index < 0)  // engine combo box was reset to "no engine selected" state
			presetModel[ selectedPresetIdx ].selectedEnginePath.clear();
		else
			presetModel[ selectedPresetIdx ].selectedEnginePath = engineModel[ index ].path;
	}

	updateSaveFilesFromDir();
	updateConfigFilesFromDir();

	updateLaunchCommand();
}

void MainWindow::selectConfig( int index )
{
	// update the current preset
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedPresetIdx >= 0) {
		if (index < 0)  // engine combo box was reset to "no engine selected" state
			presetModel[ selectedPresetIdx ].selectedConfig.clear();
		else
			presetModel[ selectedPresetIdx ].selectedConfig = configModel[ index ].fileName;
	}

	updateLaunchCommand();
}

void MainWindow::toggleIWAD( const QModelIndex & index )
{
	QString clickedIWAD = iwadModel[ index.row() ].name;

	// allow the user to deselect the IWAD by clicking on it again
	if (clickedIWAD == selectedIWAD) {
		selectedIWAD.clear();
		ui->iwadListView->selectionModel()->select( index, QItemSelectionModel::Deselect );
	} else {
		selectedIWAD = clickedIWAD;
	}

	// update the current preset
	int clickedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (clickedPresetIdx >= 0) {
		presetModel[ clickedPresetIdx ].selectedIWAD = selectedIWAD;
	}

	updateMapsFromIWAD();

	updateLaunchCommand();
}

void MainWindow::toggleMapPack( const QModelIndex & )
{
	QVector< TreePosition > selectedMapPacks;
	for (const QModelIndex & index : getSelectedItemIdxs( ui->mapDirView ))
		selectedMapPacks.append( mapModel.getNodePosition( index ) );

	// update the current preset
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedPresetIdx >= 0) {
		presetModel[ selectedPresetIdx ].selectedMapPacks = selectedMapPacks;
	}

	updateLaunchCommand();
}

void MainWindow::toggleMod( const QModelIndex & modIndex )
{
	// update the current preset
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedPresetIdx >= 0) {
		presetModel[ selectedPresetIdx ].mods[ modIndex.row() ].checked = modModel[ modIndex.row() ].checked;
	}

	updateLaunchCommand();
}


//----------------------------------------------------------------------------------------------------------------------
//  preset list manipulation

void MainWindow::presetAdd()
{
  static uint presetNum = 1;

	appendItem( presetModel, { "Preset"+QString::number( presetNum++ ) } );

	// clear the widgets to represent an empty preset
	// the widgets must be cleared AFTER the new preset is added and selected, otherwise it will clear the prev preset
	ui->engineCmbBox->setCurrentIndex( -1 );
	ui->configCmbBox->setCurrentIndex( -1 );
	deselectSelectedItems( ui->iwadListView );
	selectedIWAD.clear();
	deselectSelectedItems( ui->modListView );
	modModel.startCompleteUpdate();
	modModel.clear();
	modModel.finishCompleteUpdate();

	// open edit mode so that user can name the preset
	ui->presetListView->edit( presetModel.index( presetModel.count() - 1, 0 ) );
}

void MainWindow::presetDelete()
{
	deleteSelectedItem( ui->presetListView, presetModel );

	if (presetModel.isEmpty())  // no preset selected -> nowhere to save the IWAD/map/engine/config selection
		togglePresetSubWidgets( false );
}

void MainWindow::presetClone()
{
	int origIdx = cloneSelectedItem( ui->presetListView, presetModel );
	if (origIdx >= 0) {
		// open edit mode so that user can name the preset
		ui->presetListView->edit( presetModel.index( presetModel.count() - 1, 0 ) );
	}
}

void MainWindow::presetMoveUp()
{
	moveUpSelectedItem( ui->presetListView, presetModel );
}

void MainWindow::presetMoveDown()
{
	moveDownSelectedItem( ui->presetListView, presetModel );
}


//----------------------------------------------------------------------------------------------------------------------
//  mod list manipulation

void MainWindow::modAdd()
{
	QString path = QFileDialog::getOpenFileName( this, "Locate the mod file", modDir,
	                                             "Doom mod files (*.wad *.WAD *.pk3 *.PK3 *.pk7 *.PK7 *.zip *.ZIP *.7z *.7Z);;"
	                                             "All files (*)" );
	if (path.isEmpty())  // user probably clicked cancel
		return;

	// the path comming out of the file dialog is always absolute
	if (pathHelper.useRelativePaths())
		path = pathHelper.getRelativePath( path );

	Mod mod( QFileInfo( path ), true );

	appendItem( modModel, mod );

	// add it also to the current preset
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedPresetIdx >= 0) {
		presetModel[ selectedPresetIdx ].mods.append( mod );
	}

	updateLaunchCommand();
}

void MainWindow::modDelete()
{
	QVector<int> deletedIndexes = deleteSelectedItems( ui->modListView, modModel );
	if (deletedIndexes.isEmpty())  // no item was selected
		return;

	// remove it also from the current preset
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedPresetIdx >= 0) {
		int deletedCnt = 0;
		for (int deletedIdx : deletedIndexes) {
			presetModel[ selectedPresetIdx ].mods.removeAt( deletedIdx - deletedCnt );
			deletedCnt++;
		}
	}

	updateLaunchCommand();
}

void MainWindow::modMoveUp()
{
	QVector<int> movedIndexes = moveUpSelectedItems( ui->modListView, modModel );
	if (movedIndexes.isEmpty())  // no item was selected or they were already at the top
		return;

	// move it up also in the preset
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedPresetIdx >= 0) {
		for (int movedIdx : movedIndexes) {
			presetModel[ selectedPresetIdx ].mods.move( movedIdx, movedIdx - 1 );
		}
	}

	updateLaunchCommand();
}

void MainWindow::modMoveDown()
{
	QVector<int> movedIndexes = moveDownSelectedItems( ui->modListView, modModel );
	if (movedIndexes.isEmpty())  // no item was selected or they were already at the bottom
		return;

	// move it down also in the preset
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedPresetIdx >= 0) {
		for (int movedIdx : movedIndexes) {
			presetModel[ selectedPresetIdx ].mods.move( movedIdx, movedIdx + 1 );
		}
	}

	updateLaunchCommand();
}

void MainWindow::modsDropped( int /*row*/, int /*count*/ )
{
	// update the preset
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedPresetIdx >= 0) {
		presetModel[ selectedPresetIdx ].mods = modModel.list().toVector();  // not the most optimal way, but the size of the list will be always small
	}

	updateLaunchCommand();
}

//----------------------------------------------------------------------------------------------------------------------
//  automatic list updates according to directory content

void MainWindow::updateIWADsFromDir()
{
	updateListFromDir< IWAD >( iwadModel, ui->iwadListView, iwadDir, iwadSubdirs, pathHelper, isIWAD );

	// the previously selected item might have been removed
	if (!isSomethingSelected( ui->iwadListView ))
		selectedIWAD.clear();
}

void MainWindow::updateMapPacksFromDir()
{
	updateTreeFromDir( mapModel, ui->mapDirView, mapDir, pathHelper, isMapPack );
}

void MainWindow::updateConfigFilesFromDir()
{
	int currentEngineIdx = ui->engineCmbBox->currentIndex();
	if (currentEngineIdx < 0) {  // no engine is selected
		return;
	}

	QString configDir = engineModel[ currentEngineIdx ].configDir;

	updateComboBoxFromDir( configModel, ui->configCmbBox, configDir, false, pathHelper,
		/*isDesiredFile*/[]( const QFileInfo & file ) { return file.suffix().toLower() == configFileExt; }
	);
}

void MainWindow::updateSaveFilesFromDir()
{
	int currentEngineIdx = ui->engineCmbBox->currentIndex();
	if (currentEngineIdx < 0) {  // no engine is selected
		return;
	}

	// i don't know about a case, where the save dir would be different from config dir, it's not even configurable in zdoom
	QString saveDir = engineModel[ currentEngineIdx ].configDir;

	updateComboBoxFromDir( saveModel, ui->saveFileCmbBox, saveDir, false, pathHelper,
		/*isDesiredFile*/[]( const QFileInfo & file ) { return file.suffix().toLower() == saveFileExt; }
	);
}

void MainWindow::updateMapsFromIWAD()
{
	int iwadIdx = getSelectedItemIdx( ui->iwadListView );
	if (iwadIdx < 0)
		return;

	QString selectedIwadName = iwadModel[ iwadIdx ].name;

	if (isDoom1( selectedIwadName ) && !ui->mapCmbBox->itemText(0).startsWith('E')) {
		ui->mapCmbBox->clear();
		for (int i = 1; i <= 9; i++)
			ui->mapCmbBox->addItem( QStringLiteral("E1M%1").arg(i) );
		for (int i = 1; i <= 9; i++)
			ui->mapCmbBox->addItem( QStringLiteral("E2M%1").arg(i) );
		for (int i = 1; i <= 9; i++)
			ui->mapCmbBox->addItem( QStringLiteral("E3M%1").arg(i) );
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
	}
	updateMapPacksFromDir();
	updateConfigFilesFromDir();
	updateSaveFilesFromDir();
}


//----------------------------------------------------------------------------------------------------------------------
//  other options

void MainWindow::toggleAbsolutePaths( bool absolute )
{
	pathHelper.toggleAbsolutePaths( absolute );

	for (Engine & engine : engineModel) {
		engine.path = pathHelper.convertPath( engine.path );
		engine.configDir = pathHelper.convertPath( engine.configDir );
	}

	if (iwadListFromDir && !iwadDir.isEmpty())
		iwadDir = pathHelper.convertPath( iwadDir );
	for (IWAD & iwad : iwadModel)
		iwad.path = pathHelper.convertPath( iwad.path );

	mapDir = pathHelper.convertPath( mapDir );
	mapModel.setBaseDir( mapDir );

	modDir = pathHelper.convertPath( modDir );
	for (Mod & mod : modModel)
		mod.path = pathHelper.convertPath( mod.path );

	for (Preset & preset : presetModel) {
		for (Mod & mod : preset.mods)
			mod.path = pathHelper.convertPath( mod.path );
		preset.selectedEnginePath = pathHelper.convertPath( preset.selectedEnginePath );
	}

	updateLaunchCommand();
}

void MainWindow::modeStandard()
{
	ui->mapCmbBox->setEnabled( false );
	ui->saveFileCmbBox->setEnabled( false );
	ui->skillCmbBox->setEnabled( false );
	ui->skillSpinBox->setEnabled( false );
	ui->noMonstersChkBox->setEnabled( false );
	ui->fastMonstersChkBox->setEnabled( false );
	ui->monstersRespawnChkBox->setEnabled( false );
	ui->gameOptsBtn->setEnabled( false );
	ui->compatOptsBtn->setEnabled( false );

	if (ui->multiplayerChkBox->isChecked() && ui->multRoleCmbBox->currentIndex() == SERVER)
		ui->multRoleCmbBox->setCurrentIndex( CLIENT );   // only client can use standard launch mode

	updateLaunchCommand();
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
	ui->gameOptsBtn->setEnabled( true );
	ui->compatOptsBtn->setEnabled( true );

	if (ui->multiplayerChkBox->isChecked() && ui->multRoleCmbBox->currentIndex() == CLIENT)
		ui->multRoleCmbBox->setCurrentIndex( SERVER );   // only server can select a map

	updateLaunchCommand();
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
	ui->gameOptsBtn->setEnabled( false );
	ui->compatOptsBtn->setEnabled( false );

	updateLaunchCommand();
}

void MainWindow::selectMap( const QString & )
{
	updateLaunchCommand();
}

void MainWindow::selectSavedGame( int )
{
	updateLaunchCommand();
}

void MainWindow::selectSkill( int skill )
{
	ui->skillSpinBox->setValue( skill );
	ui->skillSpinBox->setEnabled( skill == Skill::CUSTOM );

	updateLaunchCommand();
}

void MainWindow::changeSkillNum( int skill )
{
	if (skill < Skill::CUSTOM)
		ui->skillCmbBox->setCurrentIndex( skill );

	updateLaunchCommand();
}

void MainWindow::toggleNoMonsters( bool )
{
	updateLaunchCommand();
}

void MainWindow::toggleFastMonsters( bool )
{
	updateLaunchCommand();
}

void MainWindow::toggleMonstersRespawn( bool )
{
	updateLaunchCommand();
}

void MainWindow::toggleMultiplayer( bool checked )
{
	int multRole = ui->multRoleCmbBox->currentIndex();

	ui->multRoleCmbBox->setEnabled( checked );
	ui->hostnameLine->setEnabled( checked && multRole == CLIENT );
	ui->portSpinBox->setEnabled( checked );
	ui->netModeCmbBox->setEnabled( checked );
	ui->gameModeCmbBox->setEnabled( checked && multRole == SERVER );
	ui->playerCountSpinBox->setEnabled( checked && multRole == SERVER );
	ui->teamDmgSpinBox->setEnabled( checked && multRole == SERVER );
	ui->timeLimitSpinBox->setEnabled( checked && multRole == SERVER );

	if (checked) {
		if (multRole == CLIENT && ui->launchMode_map->isChecked())  // client doesn't select map, server does
			ui->launchMode_standard->click();
		if (multRole == SERVER && ui->launchMode_standard->isChecked())  // server MUST choose a map
			ui->launchMode_map->click();
	}

	updateLaunchCommand();
}

void MainWindow::selectMultRole( int role )
{
	bool multEnabled = ui->multiplayerChkBox->isChecked();

	ui->hostnameLine->setEnabled( multEnabled && role == CLIENT );
	ui->gameModeCmbBox->setEnabled( multEnabled && role == SERVER );
	ui->playerCountSpinBox->setEnabled( multEnabled && role == SERVER );
	ui->teamDmgSpinBox->setEnabled( multEnabled && role == SERVER );
	ui->timeLimitSpinBox->setEnabled( multEnabled && role == SERVER );

	if (multEnabled) {
		if (role == CLIENT && ui->launchMode_map->isChecked())  // client doesn't select map, server does
			ui->launchMode_standard->click();
		if (role == SERVER && ui->launchMode_standard->isChecked())  // server MUST choose a map
			ui->launchMode_map->click();
	}

	updateLaunchCommand();
}

void MainWindow::changeHost( const QString & )
{
	updateLaunchCommand();
}

void MainWindow::changePort( int )
{
	updateLaunchCommand();
}

void MainWindow::selectNetMode( int )
{
	updateLaunchCommand();
}

void MainWindow::selectGameMode( int )
{
	updateLaunchCommand();
}

void MainWindow::changePlayerCount( int )
{
	updateLaunchCommand();
}

void MainWindow::changeTeamDamage( double )
{
	updateLaunchCommand();
}

void MainWindow::changeTimeLimit( int )
{
	updateLaunchCommand();
}

void MainWindow::updatePresetCmdArgs( const QString & text )
{
	// update the current preset
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedPresetIdx >= 0) {
		presetModel[ selectedPresetIdx ].cmdArgs = text;
	}

	updateLaunchCommand();
}

void MainWindow::updateGlobalCmdArgs( const QString & )
{
	updateLaunchCommand();
}


//----------------------------------------------------------------------------------------------------------------------
//  saving & loading current options

void MainWindow::saveOptions( const QString & fileName )
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

	json["use_absolute_paths"] = pathHelper.useAbsolutePaths();

	// engines
	{
		QJsonObject jsEngines;

		QJsonArray jsEngineArray;
		for (const Engine & engine : engineModel) {
			QJsonObject jsEngine;
			jsEngine["name"] = engine.name;
			jsEngine["path"] = engine.path;
			jsEngine["config_dir"] = engine.configDir;
			jsEngineArray.append( jsEngine );
		}
		jsEngines["engines"] = jsEngineArray;

		json["engines"] = jsEngines;
	}

    // IWADs
	{
		QJsonObject jsIWADs;

		jsIWADs["auto_update"] = iwadListFromDir;
		jsIWADs["directory"] = iwadDir;
		jsIWADs["subdirs"] = iwadSubdirs;
		QJsonArray jsIWADArray;
		for (const IWAD & iwad : iwadModel) {
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

		for (const Preset & preset : presetModel) {
			QJsonObject jsPreset;

			jsPreset["name"] = preset.name;
			jsPreset["selected_engine"] = preset.selectedEnginePath;
			jsPreset["selected_config"] = preset.selectedConfig;
			jsPreset["selected_IWAD"] = preset.selectedIWAD;
			QJsonArray jsMapArray;
			for (const TreePosition & pos : preset.selectedMapPacks) {
				jsMapArray.append( pos.toString() );
			}
			jsPreset["selected_mappacks"] = jsMapArray;
			jsPreset["additional_args"] = preset.cmdArgs;
			QJsonArray jsModArray;
			for (const Mod & mod : preset.mods) {
				QJsonObject jsMod;
				jsMod["path"] = mod.path;
				jsMod["checked"] = mod.checked;
				jsModArray.append( jsMod );
			}
			jsPreset["mods"] = jsModArray;

			jsPresetArray.append( jsPreset );
		}

		json["presets"] = jsPresetArray;
	}
	int presetIdx = getSelectedItemIdx( ui->presetListView );
	json["selected_preset"] = presetIdx >= 0 ? presetModel[ presetIdx ].name : "";

	// additional command line arguments
	json["additional_args"] = ui->globalCmdArgsLine->text();

	// launch options
	json["dmflags1"] = qint64( gameOpts.flags1 );
	json["dmflags2"] = qint64( gameOpts.flags2 );
	json["compatflags1"] = qint64( compatOpts.flags1 );
	json["compatflags2"] = qint64( compatOpts.flags2 );

	// write the json to file
	QJsonDocument jsonDoc( json );
	file.write( jsonDoc.toJson() );
	file.close();

	//return file.error() == QFile::NoError;
}

void MainWindow::loadOptions( const QString & fileName )
{
	QFile file( fileName );
	if (!file.open( QIODevice::ReadOnly )) {
		QMessageBox::warning( this, "Can't open file", "Loading options failed. Could not open file "+fileName+" for reading." );
		optionsCorrupted = true;
		return;
	}

	QJsonParseError error;
	QJsonDocument jsonDoc = QJsonDocument::fromJson( file.readAll(), &error );
	if (jsonDoc.isNull()) {
		QMessageBox::warning( this, "Error loading options file", "Loading options failed: " + error.errorString() );
		optionsCorrupted = true;
		return;
	}

	JsonContext json( jsonDoc.object() );

	// window geometry
	{
		int width = json.getInt( "width", -1 );
		int height = json.getInt( "height", -1 );
		if (width > 0 && height > 0) {
			const QRect & geometry = this->geometry();
			this->setGeometry( geometry.x(), geometry.y(), width, height );
		}
	}

	// preset must be deselected first, so that the cleared selections doesn't save in the preset
	deselectSelectedItems( ui->presetListView );

	// path helper must be set before others, because we want to convert the loaded paths accordingly
	pathHelper.toggleAbsolutePaths( json.getBool( "use_absolute_paths", false ) );

	if (json.enterObject( "engines" ))
	{
		ui->engineCmbBox->setCurrentIndex( -1 );

		engineModel.startCompleteUpdate();

		engineModel.clear();

		if (json.enterArray( "engines" ))
		{
			for (int i = 0; i < json.arraySize(); i++)
			{
				if (!json.enterObject( i ))  // wrong type on position i - skip this entry
					continue;

				Engine engine;
				engine.name = json.getString( "name", "<missing name>" );
				engine.path = pathHelper.convertPath( json.getString( "path" ) );
				engine.configDir = pathHelper.convertPath( json.getString( "config_dir", QFileInfo( engine.path ).dir().path() ) );
				if (engine.path.isEmpty())  // path doesn't exist - skip this entry
					continue;

				if (QFileInfo( engine.path ).exists()) {
					engineModel.append( engine );
				} else {
					QMessageBox::warning( this, "Engine no longer exists",
						"An engine from the saved options ("%engine.path%") no longer exists. It will be removed from the list." );
				}

				json.exitObject();
			}
			json.exitArray();
		}

		engineModel.finishCompleteUpdate();

		json.exitObject();
	}

	if (json.enterObject( "IWADs" ))
	{
		deselectSelectedItems( ui->iwadListView );
		selectedIWAD.clear();

		iwadModel.startCompleteUpdate();

		iwadModel.clear();

		iwadListFromDir = json.getBool( "auto_update", false );

		if (iwadListFromDir)
		{
			iwadSubdirs = json.getBool( "subdirs", false );
			QString dir = json.getString( "directory" );
			if (!dir.isEmpty()) {  // non-existing element directory -> skip completely
				if (QDir( dir ).exists()) {
					iwadDir = pathHelper.convertPath( dir );
				} else {
					QMessageBox::warning( this, "IWAD dir no longer exists",
						"IWAD directory from the saved options ("%dir%") no longer exists. Please update it in Menu -> Setup." );
				}
			}
		}
		else
		{
			if (json.enterArray( "IWADs" ))
			{
				for (int i = 0; i < json.arraySize(); i++)
				{
					if (!json.enterObject( i ))  // wrong type on position i - skip this entry
						continue;

					IWAD iwad;
					iwad.path = pathHelper.convertPath( json.getString( "path" ) );
					iwad.name = json.getString( "name", QFileInfo( iwad.path ).fileName() );
					if (iwad.name.isEmpty() || iwad.path.isEmpty())  // name or path doesn't exist - skip this entry
						continue;

					if (QFileInfo( iwad.path ).exists()) {
						iwadModel.append( iwad );
					} else {
						QMessageBox::warning( this, "IWAD no longer exists",
							"An IWAD from the saved options ("%iwad.path%") no longer exists. It will be removed from the list." );
					}

					json.exitObject();
				}
				json.exitArray();
			}
		}

		iwadModel.finishCompleteUpdate();

		json.exitObject();
	}

	if (json.enterObject( "maps" ))
	{
		deselectSelectedItems( ui->mapDirView );

		QString dir = json.getString( "directory" );
		if (!dir.isEmpty()) {  // non-existing element directory - skip completely
			if (QDir( dir ).exists()) {
				mapDir = pathHelper.convertPath( dir );
			} else {
				QMessageBox::warning( this, "Map dir no longer exists",
					"Map directory from the saved options ("%dir%") no longer exists. Please update it in Menu -> Setup." );
			}
		}

		json.exitObject();
	}

	if (json.enterObject( "mods" ))
	{
		deselectSelectedItems( ui->modListView );

		QString dir = json.getString( "directory" );
		if (!dir.isEmpty()) {  // non-existing element directory - skip completely
			if (QDir( dir ).exists()) {
				modDir = pathHelper.convertPath( dir );
			} else {
				QMessageBox::warning( this, "Mod dir no longer exists",
					"Mod directory from the saved options ("%dir%") no longer exists. Please update it in Menu -> Setup." );
			}
		}

		json.exitObject();
	}

	if (json.enterArray( "presets" ))
	{
		deselectSelectedItems( ui->presetListView );
		togglePresetSubWidgets( false );

		presetModel.startCompleteUpdate();

		presetModel.clear();

		for (int i = 0; i < json.arraySize(); i++)
		{
			if (!json.enterObject( i ))  // wrong type on position i - skip this entry
				continue;

			Preset preset;
			preset.name = json.getString( "name", "<missing name>" );
			preset.selectedEnginePath = pathHelper.convertPath( json.getString( "selected_engine" ) );
			preset.selectedConfig = json.getString( "selected_config" );
			preset.selectedIWAD = json.getString( "selected_IWAD" );
			preset.cmdArgs = json.getString( "additional_args" );

			if (json.enterArray( "selected_mappacks" ))
			{
				for (int i = 0; i < json.arraySize(); i++)
				{
					QString selectedMapPack = json.getString( i );
					if (!selectedMapPack.isEmpty()) {
						preset.selectedMapPacks.append( TreePosition( selectedMapPack ) );
					}
				}
				json.exitArray();
			}

			if (json.enterArray( "mods" ))
			{
				for (int i = 0; i < json.arraySize(); i++)
				{
					if (!json.enterObject( i ))  // wrong type on position i - skip this entry
						continue;

					QString path = json.getString( "path" );
					bool checked = json.getBool( "checked", false );
					if (!path.isEmpty()) {
						QFileInfo file( pathHelper.convertPath( path ) );
						preset.mods.append( Mod( file, checked ) );
					}

					json.exitObject();
				}
				json.exitArray();
			}

			presetModel.append( preset );

			json.exitObject();
		}

		presetModel.finishCompleteUpdate();

		json.exitArray();
	}

	ui->globalCmdArgsLine->setText( json.getString( "additional_args" ) );

	// launch options
	gameOpts.flags1 = json.getInt( "dmflags1", 0 );
	gameOpts.flags2 = json.getInt( "dmflags2", 0 );
	compatOpts.flags1 = json.getInt( "compatflags1", 0 );
	compatOpts.flags2 = json.getInt( "compatflags2", 0 );

	file.close();

	optionsCorrupted = false;

	updateListsFromDirs();

	updateLaunchCommand();
}

void MainWindow::exportPreset()
{
	int selectedIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedIdx < 0) {
		QMessageBox::warning( this, "No preset selected", "Select a preset from the preset list." );
		return;
	}

	QString filePath = QFileDialog::getSaveFileName( this, "Export preset", QString(), scriptFileExt );
	if (filePath.isEmpty()) {  // user probably clicked cancel
		return;
	}

	QFileInfo fileInfo( filePath );

	QFile file( filePath );
	if (!file.open( QIODevice::WriteOnly | QIODevice::Text )) {
		QMessageBox::warning( this, "Cannot open file", "Cannot open file for writing. Check directory permissions" );
		return;
	}

	QTextStream stream( &file );

	stream << generateLaunchCommand( fileInfo.path() ) << endl;

	file.close();
}

#include "QDebug"

void MainWindow::importPreset()
{
	QMessageBox::warning( this, "Not implemented", "Sorry, this feature is not implemented yet" );
/*
	QString filePath = QFileDialog::getOpenFileName( this, "Import preset", QString(), scriptFileExt );
	if (filePath.isEmpty()) {  // user probably clicked cancel
		return;
	}

	QFileInfo fileInfo( filePath );
	QString fileDir = fileInfo.path();

	QFile file( filePath );
	if (!file.open( QIODevice::ReadOnly | QIODevice::Text )) {
		QMessageBox::warning( this, "Cannot open file", "Cannot open file for reading. Check file permissions" );
		return;
	}

	QTextStream stream( &file );
	QString command = stream.readLine( 10 * 1024 );
	if (stream.status() == QTextStream::ReadCorruptData) {
		QMessageBox::warning( this, "Error reading file", "An error occured while reading the file. Check if the disk isn't disconnected." );
		return;
	}

	if (!stream.atEnd() && !stream.readLine( 1 ).isEmpty()) {
		QMessageBox::warning( this, "Problem with parsing file", "Only single line shorter than 10kB is supported, the rest will be ignored" );
	}

	QStringList tokens = command.split( ' ', QString::SkipEmptyParts );

	// verify that first token is existing path and that it's added in DoomRunner

	// while not at end
	//    read option
	//    read argument
*/
}


//----------------------------------------------------------------------------------------------------------------------
//  launch command generation

void MainWindow::updateLaunchCommand()
{
	QString curCommand = ui->commandLine->text();

	QString newCommand = generateLaunchCommand();

	// Don't replace the line widget's content if there is no change. It would just annoy a user who is trying to select
	// and copy part of the line, by constantly reseting his selection.
	if (newCommand != curCommand) {
		ui->commandLine->setText( newCommand );
	}
}

QString MainWindow::generateLaunchCommand( QString baseDir )
{
	// Allow the caller to specify which base directory he wants to derive the relative paths from.
	// This is required because of the "Export preset" function that needs to write the correct paths to the bat.
	// If no argument is given (empty string), fallback to system current directory.
	if (baseDir.isEmpty())
		baseDir = QDir::currentPath();

	// All stored paths are relative to pathHelper.baseDir(), but we need them relative to baseDir
	PathHelper base( pathHelper.useAbsolutePaths(), baseDir, pathHelper.baseDir() );

	QString newCommand;
	QTextStream cmdStream( &newCommand );

	int selectedEngineIdx = ui->engineCmbBox->currentIndex();
	if (selectedEngineIdx >= 0) {
		cmdStream << "\"" << base.rebasePath( engineModel[ selectedEngineIdx ].path ) << "\"";

		const int configIdx = ui->configCmbBox->currentIndex();
		if (configIdx >= 0) {
			QDir configDir( engineModel[ selectedEngineIdx ].configDir );
			QString configPath = configDir.filePath( configModel[ configIdx ].fileName );
			cmdStream << " -config \"" << base.rebasePath( configPath ) << "\"";
		}
	}

	int selectedIwadIdx = getSelectedItemIdx( ui->iwadListView );
	if (selectedIwadIdx >= 0) {
		cmdStream << " -iwad \"" << base.rebasePath( iwadModel[ selectedIwadIdx ].path ) << "\"";
	}

	for (QModelIndex & selectedMapIdx : getSelectedItemIdxs( ui->mapDirView )) {
		if (selectedMapIdx.isValid()) {
			cmdStream << " -file \"" << base.rebasePath( mapModel.getFSPath( selectedMapIdx ) ) << "\"";
		}
	}

	for (const Mod & mod : modModel) {
		if (mod.checked) {
			if (QFileInfo(mod.path).suffix() == "deh")
				cmdStream << " -deh \"" << base.rebasePath( mod.path ) << "\"";
			else
				cmdStream << " -file \"" << base.rebasePath( mod.path ) << "\"";
		}
	}

	if (ui->launchMode_map->isChecked()) {
		cmdStream << " +map " << ui->mapCmbBox->currentText();
		cmdStream << " -skill " << ui->skillSpinBox->text();
		if (ui->noMonstersChkBox->isChecked())
			cmdStream << " -nomonsters";
		if (ui->fastMonstersChkBox->isChecked())
			cmdStream << " -fast";
		if (ui->monstersRespawnChkBox->isChecked())
			cmdStream << " -respawn";
		if (gameOpts.flags1 != 0)
			cmdStream << " +dmflags " << QString::number( gameOpts.flags1 );
		if (gameOpts.flags2 != 0)
			cmdStream << " +dmflags2 " << QString::number( gameOpts.flags2 );
		if (!compatOptsCmdArgs.isEmpty()) {
			cmdStream << " " << compatOptsCmdArgs;
		}
	} else if (ui->launchMode_savefile->isChecked() && selectedEngineIdx >= 0 && ui->saveFileCmbBox->currentIndex() >= 0) {
		QDir saveDir( engineModel[ selectedEngineIdx ].configDir );
		QString savePath = saveDir.filePath( saveModel[ ui->saveFileCmbBox->currentIndex() ].fileName );
		cmdStream << " -loadgame \"" << base.rebasePath( savePath ) << "\"";
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

	if (!ui->presetCmdArgsLine->text().isEmpty())
		cmdStream << " " << ui->presetCmdArgsLine->text();

	if (!ui->globalCmdArgsLine->text().isEmpty())
		cmdStream << " " << ui->globalCmdArgsLine->text();

	cmdStream.flush();

	return newCommand;
}

void MainWindow::launch()
{
	if (ui->engineCmbBox->currentIndex() < 0) {
		QMessageBox::warning( this, "No engine selected", "No Doom engine is selected." );
		return;
	}

	bool success = QProcess::startDetached( ui->commandLine->text() );
	if (!success) {
		QMessageBox::warning( this, tr("Launch error"), tr("Failed to execute launch command.") );
	}
}
