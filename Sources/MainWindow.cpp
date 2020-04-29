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
//  local helpers

static bool verifyDir( const QString & dir, const QString & errorMessage )
{
	if (!QDir( dir ).exists()) {
		QMessageBox::warning( nullptr, "Directory no longer exists", errorMessage.arg( dir ) );
		return false;
	}
	return true;
}

static bool verifyFile( const QString & path, const QString & errorMessage )
{
	if (!QFileInfo( path ).exists()) {
		QMessageBox::warning( nullptr, "File no longer exists", errorMessage.arg( path ) );
		return false;
	}
	return true;
}

#define STORE_OPTION( structElem, value ) \
	if (optsStorage == STORE_GLOBALLY) { \
		opts.structElem = value; \
	} else if (optsStorage == STORE_TO_PRESET) { \
		int selectedPresetIdx = getSelectedItemIdx( ui->presetListView ); \
		if (selectedPresetIdx >= 0) \
			presetModel[ selectedPresetIdx ].opts.structElem = value; \
	}


//======================================================================================================================
//  MainWindow

MainWindow::MainWindow()
:
	QMainWindow( nullptr ),
	tickCount( 0 ),
	optionsCorrupted( false ),
	pathHelper( false, QDir::currentPath() ),
	engineModel(
		/*makeDisplayString*/ []( const Engine & engine ) { return engine.name; }
	),
	configModel(
		/*makeDisplayString*/ []( const ConfigFile & config ) { return config.fileName; }
	),
	saveModel(
		/*makeDisplayString*/ []( const SaveFile & save ) { return save.fileName; }
	),
	iwadModel(
		/*makeDisplayString*/ []( const IWAD & iwad ) { return iwad.name; }
	),
	iwadSettings {
		/*dir*/ "",
		/*loadFromDir*/ false,
		/*searchSubdirs*/ false
	},
	selectedIWAD(),
	mapModel(),
	mapSettings {
		""
	},
	modModel(
		/*makeDisplayString*/ []( const Mod & mod ) { return mod.fileName; }
	),
	modSettings {

	},
	optsStorage( DONT_STORE ),
	presetModel(
		/*makeDisplayString*/ []( const Preset & preset ) { return preset.name; }
	),
	opts {},
	compatOptsCmdArgs()
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
	connect( ui->launchMode_map, &QRadioButton::clicked, this, &thisClass::modeLaunchMap );
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
		iwadSettings,
		mapSettings,
		modSettings,
		optsStorage
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
		pathHelper.toggleAbsolutePaths( dialog.pathHelper.useAbsolutePaths() );
		engineModel.updateList( dialog.engineModel.list() );
		iwadModel.updateList( dialog.iwadModel.list() );
		iwadSettings = dialog.iwadSettings;
		mapSettings = dialog.mapSettings;
		modSettings = dialog.modSettings;
		optsStorage = dialog.optsStorage;
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
	GameOptsDialog dialog( this, opts.gameOpts );

	int code = dialog.exec();

	// update the data only if user clicked Ok
	if (code == QDialog::Accepted) {
		STORE_OPTION( gameOpts, dialog.gameOpts );
		updateLaunchCommand();
	}
}

void MainWindow::runCompatOptsDialog()
{
	CompatOptsDialog dialog( this, opts.compatOpts );

	int code = dialog.exec();

	// update the data only if user clicked Ok
	if (code == QDialog::Accepted) {
		STORE_OPTION( compatOpts, dialog.compatOpts );
		// cache the command line args string, so that it doesn't need to be regenerated on every command line update
		compatOptsCmdArgs = CompatOptsDialog::getCmdArgsFromOptions( opts.compatOpts );
		updateLaunchCommand();
	}
}

void MainWindow::runAboutDialog()
{
	AboutDialog dialog( this );

	dialog.exec();
}


//----------------------------------------------------------------------------------------------------------------------
//  preset loading

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
			verifyFile( preset.selectedEnginePath,
				"Engine selected for this preset ("%preset.selectedEnginePath%") no longer exists, please select another one." );
			ui->engineCmbBox->setCurrentIndex( engineIdx );
		} else {
			ui->engineCmbBox->setCurrentIndex( -1 );
			QMessageBox::warning( this, "Engine no longer exists",
				"Engine selected for this preset ("%preset.selectedEnginePath%") was removed from engine list, please select another one." );
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
				"Config file selected for this preset ("%preset.selectedConfig%") no longer exists, please select another one." );
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

	restoreLaunchOptions( preset.opts );

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


//----------------------------------------------------------------------------------------------------------------------
//  item selection

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
	QList< TreePosition > selectedMapPacks;
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
	QString path = QFileDialog::getOpenFileName( this, "Locate the mod file", modSettings.dir,
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
		presetModel[ selectedPresetIdx ].mods = modModel.list();  // not the most optimal way, but the size of the list will be always small
	}

	updateLaunchCommand();
}


//----------------------------------------------------------------------------------------------------------------------
//  launch options

void MainWindow::modeStandard()
{
	STORE_OPTION( mode, STANDARD );

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

void MainWindow::modeLaunchMap()
{
	STORE_OPTION( mode, LAUNCH_MAP );

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
	STORE_OPTION( mode, LOAD_SAVE );

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

void MainWindow::selectMap( const QString & mapName )
{
	STORE_OPTION( mapName, mapName );

	updateLaunchCommand();
}

void MainWindow::selectSavedGame( int saveIdx )
{
	STORE_OPTION( saveFile, saveModel[ saveIdx ].fileName );

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
	STORE_OPTION( skillNum, uint( skill ) );

	if (skill < Skill::CUSTOM)
		ui->skillCmbBox->setCurrentIndex( skill );

	updateLaunchCommand();
}

void MainWindow::toggleNoMonsters( bool checked )
{
	STORE_OPTION( noMonsters, checked );

	updateLaunchCommand();
}

void MainWindow::toggleFastMonsters( bool checked )
{
	STORE_OPTION( fastMonsters, checked );

	updateLaunchCommand();
}

void MainWindow::toggleMonstersRespawn( bool checked )
{
	STORE_OPTION( monstersRespawn, checked );

	updateLaunchCommand();
}

void MainWindow::toggleMultiplayer( bool checked )
{
	STORE_OPTION( isMultiplayer, checked );

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
	STORE_OPTION( multRole, MultRole( role ) );

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

void MainWindow::changeHost( const QString & hostName )
{
	STORE_OPTION( hostName, hostName );

	updateLaunchCommand();
}

void MainWindow::changePort( int port )
{
	STORE_OPTION( port, uint16_t( port ) );

	updateLaunchCommand();
}

void MainWindow::selectNetMode( int netMode )
{
	STORE_OPTION( netMode, NetMode( netMode ) );

	updateLaunchCommand();
}

void MainWindow::selectGameMode( int gameMode )
{
	STORE_OPTION( gameMode, GameMode( gameMode ) );

	updateLaunchCommand();
}

void MainWindow::changePlayerCount( int count )
{
	STORE_OPTION( playerCount, uint( count ) );

	updateLaunchCommand();
}

void MainWindow::changeTeamDamage( double damage )
{
	STORE_OPTION( teamDamage, damage );

	updateLaunchCommand();
}

void MainWindow::changeTimeLimit( int timeLimit )
{
	STORE_OPTION( timeLimit, uint( timeLimit ) );

	updateLaunchCommand();
}

//----------------------------------------------------------------------------------------------------------------------
//  additional command line arguments

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
//  misc

void MainWindow::toggleAbsolutePaths( bool absolute )
{
	pathHelper.toggleAbsolutePaths( absolute );

	for (Engine & engine : engineModel) {
		engine.path = pathHelper.convertPath( engine.path );
		engine.configDir = pathHelper.convertPath( engine.configDir );
	}

	iwadSettings.dir = pathHelper.convertPath( iwadSettings.dir );
	for (IWAD & iwad : iwadModel)
		iwad.path = pathHelper.convertPath( iwad.path );

	mapSettings.dir = pathHelper.convertPath( mapSettings.dir );
	mapModel.setBaseDir( mapSettings.dir );

	modSettings.dir = pathHelper.convertPath( modSettings.dir );
	for (Mod & mod : modModel)
		mod.path = pathHelper.convertPath( mod.path );

	for (Preset & preset : presetModel) {
		for (Mod & mod : preset.mods)
			mod.path = pathHelper.convertPath( mod.path );
		preset.selectedEnginePath = pathHelper.convertPath( preset.selectedEnginePath );
	}

	updateLaunchCommand();
}


//----------------------------------------------------------------------------------------------------------------------
//  automatic list updates according to directory content

void MainWindow::updateListsFromDirs()
{
	if (iwadSettings.updateFromDir) {
		updateIWADsFromDir();
	}
	updateMapPacksFromDir();
	updateConfigFilesFromDir();
	updateSaveFilesFromDir();
}

void MainWindow::updateIWADsFromDir()
{
	updateListFromDir< IWAD >( iwadModel, ui->iwadListView, iwadSettings.dir, iwadSettings.searchSubdirs, pathHelper, isIWAD );

	// the previously selected item might have been removed
	if (!isSomethingSelected( ui->iwadListView ))
		selectedIWAD.clear();
}

void MainWindow::updateMapPacksFromDir()
{
	updateTreeFromDir( mapModel, ui->mapDirView, mapSettings.dir, pathHelper, isMapPack );
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


//----------------------------------------------------------------------------------------------------------------------
//  saving and loading user data entered into the launcher

void MainWindow::saveOptions( const QString & fileName )
{
	QFile file( fileName );
	if (!file.open( QIODevice::WriteOnly )) {
		QMessageBox::warning( this, "Can't open file", "Saving options failed. Could not open file "%fileName%" for writing." );
	}

	QJsonObject json;

	{
		QJsonObject jsGeometry;

		const QRect & geometry = this->geometry();
		jsGeometry["width"] = geometry.width();
		jsGeometry["height"] = geometry.height();

		json["geometry"] = jsGeometry;
	}

	json["use_absolute_paths"] = pathHelper.useAbsolutePaths();

	{
		QJsonObject jsEngines;

		jsEngines["engines"] = serializeList( engineModel.list() );

		json["engines"] = jsEngines;
	}

	{
		QJsonObject jsIWADs = serialize( iwadSettings );

		if (iwadSettings.updateFromDir) {
			jsIWADs["IWADs"] = serializeList( iwadModel.list() );
		}

		json["IWADs"] = jsIWADs;
    }

	json["maps"] = serialize( mapSettings );

	json["mods"] = serialize( modSettings );

	{
		QJsonArray jsPresetArray;
		for (const Preset & preset : presetModel) {
			QJsonObject jsPreset = serialize( preset, optsStorage == STORE_TO_PRESET );
			jsPresetArray.append( jsPreset );
		}
		json["presets"] = jsPresetArray;

		int presetIdx = getSelectedItemIdx( ui->presetListView );
		json["selected_preset"] = presetIdx >= 0 ? presetModel[ presetIdx ].name : "";
	}

	json["additional_args"] = ui->globalCmdArgsLine->text();

	json["options_storage"] = int( optsStorage );
	if (optsStorage == STORE_GLOBALLY) {
		json["options"] = serialize( opts );
	}

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

	QByteArray data = file.readAll();

	QJsonParseError error;
	QJsonDocument jsonDoc = QJsonDocument::fromJson( data, &error );
	if (jsonDoc.isNull()) {
		QMessageBox::warning( this, "Error loading options file", "Loading options failed: " + error.errorString() );
		optionsCorrupted = true;
		return;
	}

	// We use this contextual mechanism instead of standard JSON getters, because when something fails to load
	// we want to print a useful error message with information where in the whole JSON the problem is.
	JsonContext json( jsonDoc.object() );

	if (json.enterObject( "geometry" ))
	{
		int width = json.getInt( "width", -1 );
		int height = json.getInt( "height", -1 );
		if (width > 0 && height > 0) {
			const QRect & geometry = this->geometry();
			this->setGeometry( geometry.x(), geometry.y(), width, height );
		}

		json.exitObject();
	}

	// preset must be deselected first, so that the cleared selections doesn't save in the preset
	deselectSelectedItems( ui->presetListView );

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
				if (!json.enterObject( i ))  // wrong type on position i -> skip this entry
					continue;

				Engine engine;
				deserialize( json, engine );

				if (engine.path.isEmpty())  // element isn't present in JSON -> skip this entry
					continue;

				if (!verifyFile( engine.path, "An engine from the saved options (%1) no longer exists. It will be removed from the list." ))
					continue;

				engineModel.append( std::move( engine ) );

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

		deserialize( json, iwadSettings );

		if (iwadSettings.updateFromDir)
		{
			verifyDir( iwadSettings.dir, "IWAD directory from the saved options (%1) no longer exists. Please update it in Menu -> Setup." );
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
					deserialize( json, iwad );

					if (iwad.name.isEmpty() || iwad.path.isEmpty())  // element isn't present in JSON -> skip this entry
						continue;

					if (!verifyFile( iwad.path, "An IWAD from the saved options (%1) no longer exists. It will be removed from the list." ))
						continue;

					iwadModel.append( std::move( iwad ) );

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

		deserialize( json, mapSettings );

		verifyDir( mapSettings.dir, "Map directory from the saved options (%1) no longer exists. Please update it in Menu -> Setup." );

		json.exitObject();
	}

	if (json.enterObject( "mods" ))
	{
		deselectSelectedItems( ui->modListView );

		deserialize( json, modSettings );

		verifyDir( modSettings.dir, "Mod directory from the saved options (%1) no longer exists. Please update it in Menu -> Setup." );

		json.exitObject();
	}

	// this must be loaded before presets, because we need to know whether to attempt loading the opts from the presets
	optsStorage = json.getEnum< OptionsStorage >( "options_storage", STORE_GLOBALLY );

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
			deserialize( json, preset, optsStorage == STORE_TO_PRESET );

			presetModel.append( std::move( preset ) );

			json.exitObject();
		}

		presetModel.finishCompleteUpdate();

		json.exitArray();
	}

	ui->globalCmdArgsLine->setText( json.getString( "additional_args" ) );

	// launch options
	if (optsStorage == STORE_GLOBALLY && json.enterObject( "options" ))
	{
		deserialize( json, opts );

		json.exitObject();
	}

	// make sure all paths loaded from JSON are stored in correct format
	toggleAbsolutePaths( pathHelper.useAbsolutePaths() );

	optionsCorrupted = false;

	file.close();

	updateListsFromDirs();

	// this must be done after the lists are already updated because we want to select existing item in combo boxes
	restoreLaunchOptions( opts );

	updateLaunchCommand();
}

void MainWindow::restoreLaunchOptions( const LaunchOptions & opts )
{
	if (opts.mode == STANDARD)
		ui->launchMode_standard->click();
	else if (opts.mode == LAUNCH_MAP)
		ui->launchMode_map->click();
	else if (opts.mode == LOAD_SAVE)
		ui->launchMode_savefile->click();

	ui->mapCmbBox->setCurrentText( opts.mapName );

	if (!opts.saveFile.isEmpty()) {
		int saveFileIdx = findSuch( saveModel.list(), [ &opts ]( const SaveFile & save ) { return save.fileName == opts.saveFile; } );
		if (saveFileIdx >= 0) {
			ui->saveFileCmbBox->setCurrentIndex( saveFileIdx );
		} else {
			ui->saveFileCmbBox->setCurrentIndex( -1 );
			QMessageBox::warning( this, "Save file no longer exists",
				"Save file \""%opts.saveFile%"\" no longer exists, please select another one." );
		}
	}

	ui->skillSpinBox->setValue( int( opts.skillNum ) );

	ui->noMonstersChkBox->setChecked( opts.noMonsters );
	ui->fastMonstersChkBox->setChecked( opts.fastMonsters );
	ui->monstersRespawnChkBox->setChecked( opts.monstersRespawn );

	ui->multiplayerChkBox->setChecked( opts.isMultiplayer );
	ui->multRoleCmbBox->setCurrentIndex( int( opts.multRole ) );
	ui->hostnameLine->setText( opts.hostName );
	ui->portSpinBox->setValue( opts.port );
	ui->netModeCmbBox->setCurrentIndex( int( opts.netMode ) );
	ui->gameModeCmbBox->setCurrentIndex( int( opts.gameMode ) );
	ui->playerCountSpinBox->setValue( int( opts.playerCount ) );
	ui->teamDmgSpinBox->setValue( opts.teamDamage );
	ui->timeLimitSpinBox->setValue( int( opts.timeLimit ) );
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
		const Engine & selectedEngine = engineModel[ selectedEngineIdx ];

		//verifyFile( selectedEngine.path, "The selected engine (%1) no longer exists. Please update its path in Menu -> Setup." );
		cmdStream << "\"" << base.rebasePath( selectedEngine.path ) << "\"";

		const int configIdx = ui->configCmbBox->currentIndex();
		if (configIdx >= 0) {
			QDir configDir( selectedEngine.configDir );
			QString configPath = configDir.filePath( configModel[ configIdx ].fileName );

			//verifyFile( configPath, "The selected config (%1) no longer exists. Please update the config dir in Menu -> Setup" );
			cmdStream << " -config \"" << base.rebasePath( configPath ) << "\"";
		}
	}

	int selectedIwadIdx = getSelectedItemIdx( ui->iwadListView );
	if (selectedIwadIdx >= 0) {
		//verifyFile( iwadModel[ selectedIwadIdx ].path, "The selected IWAD (%1) no longer exists. Please update it in Menu -> Setup." );
		cmdStream << " -iwad \"" << base.rebasePath( iwadModel[ selectedIwadIdx ].path ) << "\"";
	}

	for (QModelIndex & selectedMapIdx : getSelectedItemIdxs( ui->mapDirView )) {
		if (selectedMapIdx.isValid()) {
			QString mapFilePath = mapModel.getFSPath( selectedMapIdx );
			//verifyFile( mapFilePath, "The selected map pack (%1) no longer exists. Please select another one." );
			cmdStream << " -file \"" << base.rebasePath( mapFilePath ) << "\"";
		}
	}

	for (const Mod & mod : modModel) {
		if (mod.checked) {
			//verifyFile( mod.path, "The selected mod (%1) no longer exists. Please update the mod list." );
			if (QFileInfo( mod.path ).suffix() == "deh")
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
		if (opts.gameOpts.flags1 != 0)
			cmdStream << " +dmflags " << QString::number( opts.gameOpts.flags1 );
		if (opts.gameOpts.flags2 != 0)
			cmdStream << " +dmflags2 " << QString::number( opts.gameOpts.flags2 );
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
					"The game mode index is out of range. This is a bug, please create an issue on Github page." );
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
				"The multiplayer role index is out of range. This is a bug, please create an issue on Github page." );
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
