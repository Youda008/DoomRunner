//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  13.5.2019
// Description:
//======================================================================================================================

#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include "AboutDialog.hpp"
#include "SetupDialog.hpp"
#include "ConfigDialog.hpp"
#include "GameOptsDialog.hpp"
#include "CompatOptsDialog.hpp"

#include "EventFilters.hpp"  // ConfirmationFilter
#include "LangUtils.hpp"
#include "JsonUtils.hpp"
#include "WidgetUtils.hpp"
#include "FileSystemUtils.hpp"
#include "OSUtils.hpp"
#include "DoomUtils.hpp"
#include "UpdateChecker.hpp"
#include "Version.hpp"

#include <QString>
#include <QStringBuilder>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QProcess>
#include <QJsonDocument>
#include <QDebug>

#include <QVBoxLayout>
#include <QPlainTextEdit>
#include <QFontDatabase>


//======================================================================================================================

#ifdef _WIN32
	static const QString scriptFileSuffix = "*.bat";
#else
	static const QString scriptFileSuffix = "*.sh";
#endif

static constexpr char defaultOptionsFile [] = "options.json";


//======================================================================================================================
//  local helpers

static QString getOptionsFilePath()
{
	return QDir( getAppDataDir() ).filePath( defaultOptionsFile );
}

static bool verifyDir( const QString & dir, const QString & errorMessage )
{
	if (!QDir( dir ).exists())
	{
		QMessageBox::warning( nullptr, "Directory no longer exists", errorMessage.arg( dir ) );
		return false;
	}
	return true;
}

static bool verifyFile( const QString & path, const QString & errorMessage )
{
	if (!QFileInfo::exists( path ))
	{
		QMessageBox::warning( nullptr, "File no longer exists", errorMessage.arg( path ) );
		return false;
	}
	return true;
}

#define STORE_OPTION( structElem, value ) \
	if (optsStorage == STORE_GLOBALLY) \
	{\
		opts.structElem = value; \
	}\
	else if (optsStorage == STORE_TO_PRESET) \
	{\
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
	disableSelectionCallbacks( false ),
	compatOptsCmdArgs(),
	pathContext( false, QApplication::applicationDirPath() ), // all paths will internally be stored relative to the application's dir
	checkForUpdates( true ),
	optsStorage( STORE_GLOBALLY ),
	closeOnLaunch( false ),
	engineModel(
		/*makeDisplayString*/ []( const Engine & engine ) { return engine.name; }
	),
	configModel(
		/*makeDisplayString*/ []( const ConfigFile & config ) { return config.fileName; }
	),
	saveModel(
		/*makeDisplayString*/ []( const SaveFile & save ) { return save.fileName; }
	),
	demoModel(
		/*makeDisplayString*/ []( const DemoFile & demo ) { return demo.fileName; }
	),
	iwadModel(
		/*makeDisplayString*/ []( const IWAD & iwad ) { return iwad.name; }
	),
	iwadSettings {
		/*dir*/ "",
		/*loadFromDir*/ false,
		/*searchSubdirs*/ false
	},
	mapModel(),
	mapSettings {
		""
	},
	modModel(
		/*makeDisplayString*/ []( const Mod & mod ) { return mod.fileName; }
	),
	modSettings {

	},
	presetModel(
		/*makeDisplayString*/ []( const Preset & preset ) { return preset.name; }
	),
	opts {}
{
	ui = new Ui::MainWindow;
	ui->setupUi( this );

	this->setWindowTitle( windowTitle() + ' ' + appVersion );

	// setup main menu actions

	connect( ui->setupPathsAction, &QAction::triggered, this, &thisClass::runSetupDialog );
	connect( ui->exportPresetAction, &QAction::triggered, this, &thisClass::exportPreset );
	//connect( ui->importPresetAction, &QAction::triggered, this, &thisClass::importPreset );
	connect( ui->aboutAction, &QAction::triggered, this, &thisClass::runAboutDialog );
	connect( ui->exitAction, &QAction::triggered, this, &thisClass::close );

	// setup main list views

	setupPresetView();
	setupIWADView();
	setupMapPackView();
	setupModView();

	// setup combo-boxes

	// we use custom model for engines, because we want to display the same list differently in different window
	ui->engineCmbBox->setModel( &engineModel );
	// we use custom model for configs and saves, because calling 'clear' or 'add' on a combo box changes current index,
	// which causes the launch command to be regenerated back and forth on every update
	ui->configCmbBox->setModel( &configModel );
	ui->saveFileCmbBox->setModel( &saveModel );
	ui->demoFileCmbBox_replay->setModel( &demoModel );
	connect( ui->engineCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectEngine );
	connect( ui->configCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectConfig );

	// setup buttons

	connect( ui->configAddBtn, &QToolButton::clicked, this, &thisClass::cloneConfig );

	connect( ui->presetBtnAdd, &QToolButton::clicked, this, &thisClass::presetAdd );
	connect( ui->presetBtnDel, &QToolButton::clicked, this, &thisClass::presetDelete );
	connect( ui->presetBtnClone, &QToolButton::clicked, this, &thisClass::presetClone );
	connect( ui->presetBtnUp, &QToolButton::clicked, this, &thisClass::presetMoveUp );
	connect( ui->presetBtnDown, &QToolButton::clicked, this, &thisClass::presetMoveDown );

	connect( ui->modBtnAdd, &QToolButton::clicked, this, &thisClass::modAdd );
	connect( ui->modBtnAddDir, &QToolButton::clicked, this, &thisClass::modAddDir );
	connect( ui->modBtnDel, &QToolButton::clicked, this, &thisClass::modDelete );
	connect( ui->modBtnUp, &QToolButton::clicked, this, &thisClass::modMoveUp );
	connect( ui->modBtnDown, &QToolButton::clicked, this, &thisClass::modMoveDown );

	// setup launch options callbacks

	// launch mode
	connect( ui->launchMode_standard, &QRadioButton::clicked, this, &thisClass::modeStandard );
	connect( ui->launchMode_map, &QRadioButton::clicked, this, &thisClass::modeLaunchMap );
	connect( ui->launchMode_savefile, &QRadioButton::clicked, this, &thisClass::modeSavedGame );
	connect( ui->launchMode_recordDemo, &QRadioButton::clicked, this, &thisClass::modeRecordDemo );
	connect( ui->launchMode_replayDemo, &QRadioButton::clicked, this, &thisClass::modeReplayDemo );
	connect( ui->mapCmbBox, &QComboBox::currentTextChanged, this, &thisClass::selectMap );
	connect( ui->saveFileCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectSavedGame );
	connect( ui->mapCmbBox_demo, &QComboBox::currentTextChanged, this, &thisClass::selectMap_demo );
	connect( ui->demoFileLine_record, &QLineEdit::textChanged, this, &thisClass::changeDemoFile_record );
	connect( ui->demoFileCmbBox_replay, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectDemoFile_replay );

	// gameplay
	connect( ui->skillCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectSkill );
	connect( ui->skillSpinBox, QOverload<int>::of( &QSpinBox::valueChanged ), this, &thisClass::changeSkillNum );
	connect( ui->noMonstersChkBox, &QCheckBox::toggled, this, &thisClass::toggleNoMonsters );
	connect( ui->fastMonstersChkBox, &QCheckBox::toggled, this, &thisClass::toggleFastMonsters );
	connect( ui->monstersRespawnChkBox, &QCheckBox::toggled, this, &thisClass::toggleMonstersRespawn );
	connect( ui->gameOptsBtn, &QPushButton::clicked, this, &thisClass::runGameOptsDialog );
	connect( ui->compatOptsBtn, &QPushButton::clicked, this, &thisClass::runCompatOptsDialog );
	connect( ui->allowCheatsChkBox, &QCheckBox::toggled, this, &thisClass::toggleAllowCheats );

	// video
	loadMonitorInfo( ui->monitorCmbBox );
	connect( ui->monitorCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectMonitor );
	connect( ui->resolutionXLine, &QLineEdit::textChanged, this, &thisClass::changeResolutionX );
	connect( ui->resolutionYLine, &QLineEdit::textChanged, this, &thisClass::changeResolutionY );

	// audio
	connect( ui->noSoundChkBox, &QCheckBox::toggled, this, &thisClass::toggleNoSound );
	connect( ui->noSfxChkBox, &QCheckBox::toggled, this, &thisClass::toggleNoSFX);
	connect( ui->noMusicChkBox, &QCheckBox::toggled, this, &thisClass::toggleNoMusic );

	// alternative paths
	connect( ui->saveDirLine, &QLineEdit::textChanged, this, &thisClass::changeSaveDir );
	connect( ui->screenshotDirLine, &QLineEdit::textChanged, this, &thisClass::changeScreenshotDir );
	connect( ui->saveDirBtn, &QPushButton::clicked, this, &thisClass::browseSaveDir );
	connect( ui->screenshotDirBtn, &QPushButton::clicked, this, &thisClass::browseScreenshotDir );

	// mutiplayer
	connect( ui->multiplayerChkBox, &QCheckBox::toggled, this, &thisClass::toggleMultiplayer );
	connect( ui->multRoleCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectMultRole );
	connect( ui->hostnameLine, &QLineEdit::textChanged, this, &thisClass::changeHost );
	connect( ui->portSpinBox, QOverload<int>::of( &QSpinBox::valueChanged ), this, &thisClass::changePort );
	connect( ui->netModeCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectNetMode );
	connect( ui->gameModeCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectGameMode );
	connect( ui->playerCountSpinBox, QOverload<int>::of( &QSpinBox::valueChanged ), this, &thisClass::changePlayerCount );
	connect( ui->teamDmgSpinBox, QOverload<double>::of( &QDoubleSpinBox::valueChanged ), this, &thisClass::changeTeamDamage );
	connect( ui->timeLimitSpinBox, QOverload<int>::of( &QSpinBox::valueChanged ), this, &thisClass::changeTimeLimit );
	connect( ui->fragLimitSpinBox, QOverload<int>::of( &QSpinBox::valueChanged ), this, &thisClass::changeFragLimit );

	// the rest

	connect( ui->presetCmdArgsLine, &QLineEdit::textChanged, this, &thisClass::updatePresetCmdArgs );
	connect( ui->globalCmdArgsLine, &QLineEdit::textChanged, this, &thisClass::updateGlobalCmdArgs );
	connect( ui->launchBtn, &QPushButton::clicked, this, &thisClass::launch );

	// this will call the function when the window is fully initialized and displayed
	// not sure, which one of these 2 options is better
	//QMetaObject::invokeMethod( this, &thisClass::onWindowShown, Qt::ConnectionType::QueuedConnection ); // this doesn't work in Qt 5.9
	QTimer::singleShot( 0, this, &thisClass::onWindowShown );
}

void MainWindow::setupPresetView()
{
	// specify where to get the string for edit mode
	presetModel.setEditStringFunc( []( Preset & preset ) -> QString & { return preset.name; } );

	// set data source for the view
	ui->presetListView->setModel( &presetModel );

	// set drag&drop behaviour
	ui->presetListView->toggleNameEditing( true );
	ui->presetListView->toggleIntraWidgetDragAndDrop( true );
	ui->presetListView->toggleInterWidgetDragAndDrop( false );
	ui->presetListView->toggleExternalFileDragAndDrop( false );

	// set reaction when an item is selected
	connect( ui->presetListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &thisClass::togglePreset );

	// setup reaction to key shortcuts and right click
	ui->presetListView->toggleContextMenu( true );
	ui->presetListView->enableItemCloning();
	connect( ui->presetListView->addAction, &QAction::triggered, this, &thisClass::presetAdd );
	connect( ui->presetListView->deleteAction, &QAction::triggered, this, &thisClass::presetDelete );
	connect( ui->presetListView->cloneAction, &QAction::triggered, this, &thisClass::presetClone );
	connect( ui->presetListView->moveUpAction, &QAction::triggered, this, &thisClass::presetMoveUp );
	connect( ui->presetListView->moveDownAction, &QAction::triggered, this, &thisClass::presetMoveDown );
}

void MainWindow::setupIWADView()
{
	// set data source for the view
	ui->iwadListView->setModel( &iwadModel );

	// set reaction when an item is selected
	connect( ui->iwadListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &thisClass::toggleIWAD );
}

void MainWindow::setupMapPackView()
{
	// set data source for the view
	ui->mapDirView->setModel( &mapModel );

	// set drag&drop behaviour
	ui->mapDirView->setDragEnabled( true );
	ui->mapDirView->setDragDropMode( QAbstractItemView::DragOnly );

	// set reaction when an item is selected
	connect( ui->mapDirView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &thisClass::toggleMapPack );
	connect( ui->mapDirView, &QTreeView::doubleClicked, this, &thisClass::showMapPackDesc );
}

void MainWindow::setupModView()
{
	// specify where to get the bool flag for the checkbox
	modModel.setIsCheckedFunc( []( Mod & mod ) -> bool & { return mod.checked; } );

	// give the model our path convertor, it will need it for converting paths dropped from directory
	modModel.setPathContext( &pathContext );

	// set data source for the view
	ui->modListView->setModel( &modModel );

	// set drag&drop behaviour
	ui->modListView->toggleNameEditing( false );
	ui->modListView->toggleIntraWidgetDragAndDrop( true );
	ui->modListView->toggleInterWidgetDragAndDrop( true );
	ui->modListView->toggleExternalFileDragAndDrop( true );
	connect( ui->modListView, QOverload< int, int >::of( &EditableListView::itemsDropped ), this, &thisClass::modsDropped );

	// set reaction when an item is checked or unchecked
	connect( &modModel, &QAbstractListModel::dataChanged, this, &thisClass::modDataChanged );

	// setup reaction to key shortcuts and right click
	ui->modListView->toggleContextMenu( true );
	connect( ui->modListView->addAction, &QAction::triggered, this, &thisClass::modAdd );
	connect( ui->modListView->deleteAction, &QAction::triggered, this, &thisClass::modDelete );
	connect( ui->modListView->moveUpAction, &QAction::triggered, this, &thisClass::modMoveUp );
	connect( ui->modListView->moveDownAction, &QAction::triggered, this, &thisClass::modMoveDown );
}

void MainWindow::loadMonitorInfo( QComboBox * box )
{
	const auto monitors = listMonitors();
	for (const MonitorInfo & monitor : monitors)
	{
		QString monitorDesc;
		QTextStream descStream( &monitorDesc, QIODevice::WriteOnly );

		descStream << monitor.name << " - " << QString::number( monitor.width ) << 'x' << QString::number( monitor.height );
		if (monitor.isPrimary)
			descStream << " (primary)";

		descStream.flush();
		box->addItem( monitorDesc );
	}
}

void MainWindow::onWindowShown()
{
	// In the constructor, some properties of the window are not yet initialized, like window dimensions,
	// so we have to do this here, when the window is already fully loaded.

	// create a directory for application data, if it doesn't exist already
	QDir appDataDir( getAppDataDir() );
	if (!appDataDir.exists())
	{
		appDataDir.mkpath(".");
	}

	// try to load last saved state
	QString optionsFilePath = appDataDir.filePath( defaultOptionsFile );
	if (QFileInfo::exists( optionsFilePath ))
		loadOptions( optionsFilePath );
	else  // this is a first run, perform an initial setup
		runSetupDialog();

	// if the presets are empty or none is selected, add a default one so that users don't complain that they can't enter anything
	if (!isSomethingSelected( ui->presetListView ))
	{
		int defaultPresetIdx = findSuch( presetModel, []( const Preset & preset ) { return preset.name == "Default"; } );
		if (defaultPresetIdx < 0)
		{
			prependItem( ui->presetListView, presetModel, { "Default" } );
			defaultPresetIdx = 0;
		}
		selectItemByIdx( ui->presetListView, defaultPresetIdx );  // this invokes the callback, which enables the dependent widgets
		                                                          // and calls restorePreset(...);
	}

	if (checkForUpdates)
	{
		updateChecker.checkForUpdates(
			/* result callback */[ this ]( UpdateChecker::Result result, const QString & detail )
			{
				if (result == UpdateChecker::UPDATE_AVAILABLE)
				{
					checkForUpdates = showUpdateNotification( this, detail, true );
				}
				// silently ignore the rest of the results, since nobody asked for anything
			}
		);
	}

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

	if (tickCount % dirUpdateDelay == 0)
	{
		updateListsFromDirs();
	}

	if (tickCount % 60 == 0)
	{
		if (!optionsCorrupted)  // don't overwrite existing file with empty data, when there was just one small syntax error
			saveOptions( getOptionsFilePath() );
	}
}

void MainWindow::closeEvent( QCloseEvent * event )
{
	if (!optionsCorrupted)  // don't overwrite existing file with empty data, when there was just one small syntax error
		saveOptions( getOptionsFilePath() );

	QMainWindow::closeEvent( event );
}

MainWindow::~MainWindow()
{
	delete ui;
}


//----------------------------------------------------------------------------------------------------------------------
//  dialogs

void MainWindow::runAboutDialog()
{
	AboutDialog dialog( this, checkForUpdates );

	dialog.exec();

	checkForUpdates = dialog.checkForUpdates;
}

void MainWindow::runSetupDialog()
{
	// The dialog gets a copy of all the required data and when it's confirmed, we copy them back.
	// This allows the user to cancel all the changes he made without having to manually revert them.
	// Secondly, this removes all the problems with data synchronization like dangling pointers
	// or that previously selected items in the view no longer exist.
	// And thirdly, we no longer have to split the data itself from the logic of displaying them.

	SetupDialog dialog(
		this,
		pathContext.useAbsolutePaths(),
		pathContext.baseDir(),
		engineModel.list(),
		iwadModel.list(),
		iwadSettings,
		mapSettings,
		modSettings,
		optsStorage,
		closeOnLaunch
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
		pathContext.toggleAbsolutePaths( dialog.pathContext.useAbsolutePaths() );
		engineModel.updateList( dialog.engineModel.list() );
		iwadModel.updateList( dialog.iwadModel.list() );
		iwadSettings = dialog.iwadSettings;
		mapSettings = dialog.mapSettings;
		modSettings = dialog.modSettings;
		optsStorage = dialog.optsStorage;
		closeOnLaunch = dialog.closeOnLaunch;
		// update all stored paths
		toggleAbsolutePaths( pathContext.useAbsolutePaths() );
		selectedEngine = pathContext.convertPath( selectedEngine );
		selectedIWAD = pathContext.convertPath( selectedIWAD );

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
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	GameplayOptions * gameOpts = (optsStorage == STORE_TO_PRESET && selectedPresetIdx >= 0)
	                               ? &presetModel[ selectedPresetIdx ].opts.gameOpts
	                               : &opts.gameOpts;

	GameOptsDialog dialog( this, *gameOpts );

	int code = dialog.exec();

	// update the data only if user clicked Ok
	if (code == QDialog::Accepted)
	{
		STORE_OPTION( gameOpts, dialog.gameOpts )
		updateLaunchCommand();
	}
}

void MainWindow::runCompatOptsDialog()
{
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	CompatibilityOptions * compatOpts = (optsStorage == STORE_TO_PRESET && selectedPresetIdx >= 0)
	                                      ? &presetModel[ selectedPresetIdx ].opts.compatOpts
	                                      : &opts.compatOpts;

	CompatOptsDialog dialog( this, *compatOpts );

	int code = dialog.exec();

	// update the data only if user clicked Ok
	if (code == QDialog::Accepted)
	{
		STORE_OPTION( compatOpts, dialog.compatOpts )
		// cache the command line args string, so that it doesn't need to be regenerated on every command line update
		compatOptsCmdArgs = CompatOptsDialog::getCmdArgsFromOptions( opts.compatOpts );
		updateLaunchCommand();
	}
}

void MainWindow::cloneConfig()
{
	QDir configDir( engineModel[ ui->engineCmbBox->currentIndex() ].configDir );  // if config was selected, engine selection must be valid too
	QFileInfo oldConfig( configDir.filePath( ui->configCmbBox->currentText() ) );

	ConfigDialog dialog( this, oldConfig.completeBaseName() );

	int code = dialog.exec();

	// perform the action only if user clicked Ok
	if (code == QDialog::Accepted)
	{
		QString oldConfigPath = oldConfig.filePath();
		QString newConfigPath = configDir.filePath( dialog.newConfigName + '.' + oldConfig.suffix() );
		bool copied = QFile::copy( oldConfigPath, newConfigPath );
		if (!copied)
		{
			QMessageBox::warning( this, "Error copying file",
				"Couldn't create file "%newConfigPath%". Check permissions." );
		}
	}
}


//----------------------------------------------------------------------------------------------------------------------
//  preset loading

// loads the content of a preset into the other widgets
void MainWindow::restorePreset( int presetIdx )
{
	Preset & preset = presetModel[ presetIdx ];

	// NOTE: Every change of a selection, like cmbBox->setCurrentIndex( idx ) or selectItemByIdx( idx )
	//       causes the corresponding callbacks to be called which in turn changes the value stored in the preset.
	//       You have to update the selection as the last operation after the preset value is not needed anymore,
	//       or make a copy of the value prior to updating the selection.

	// restore selected engine
	if (!preset.selectedEnginePath.isEmpty())  // the engine combo box might have been empty when creating this preset
	{
		int engineIdx = findSuch( engineModel, [ &preset ]( const Engine & engine )
		                                                  { return engine.path == preset.selectedEnginePath; } );
		if (engineIdx >= 0)
		{
			if (QFileInfo::exists( preset.selectedEnginePath ))
			{
				ui->engineCmbBox->setCurrentIndex( engineIdx );  // this invokes the callback, which updates the preset and the dependent combo-boxes
			}
			else
			{
				QMessageBox::warning( this, "Engine no longer exists",
					"Engine selected for this preset ("%preset.selectedEnginePath%") no longer exists, please update the engines at Menu -> Initial Setup." );
				ui->engineCmbBox->setCurrentIndex( -1 );  // this invokes the callback, which updates the preset and the dependent combo-boxes
			}
		}
		else
		{
			QMessageBox::warning( this, "Engine no longer exists",
				"Engine selected for this preset ("%preset.selectedEnginePath%") was removed from engine list, please select another one." );
			ui->engineCmbBox->setCurrentIndex( -1 );  // this invokes the callback, which updates the preset and the dependent combo-boxes
		}
	}
	else
	{
		ui->engineCmbBox->setCurrentIndex( -1 );  // this invokes the callback, which updates the preset and the dependent combo-boxes
	}

	// restore selected config
	if (!configModel.isEmpty())  // the engine might have not been selected yet so the configs have not been loaded
	{
		int configIdx = findSuch( configModel, [ &preset ]( const ConfigFile & config )
		                                                  { return config.fileName == preset.selectedConfig; } );
		if (configIdx >= 0)
		{
			// No sense to verify if this config file exists, the configModel has just been updated during
			// engineCmbBox->setCurrentIndex(...) so there are only existing files.
			ui->configCmbBox->setCurrentIndex( configIdx );  // this invokes the callback, which updates the preset
		}
		else
		{
			QMessageBox::warning( this, "Config no longer exists",
				"Config file selected for this preset ("%preset.selectedConfig%") no longer exists, please select another one." );
			ui->configCmbBox->setCurrentIndex( -1 );  // this invokes the callback, which updates the preset
		}
	}
	else
	{
		ui->configCmbBox->setCurrentIndex( -1 );  // this invokes the callback, which updates the preset
	}

	// restore selected IWAD
	if (!preset.selectedIWAD.isEmpty())  // the IWAD may have not been selected when creating this preset
	{
		int iwadIdx = findSuch( iwadModel, [ &preset ]( const IWAD & iwad )
		                                              { return iwad.path == preset.selectedIWAD; } );
		if (iwadIdx >= 0)
		{
			if (QFileInfo::exists( preset.selectedIWAD ))
			{
				deselectSelectedItems( ui->iwadListView );     // This invokes the callback, which updates the preset and calls updateMapsFromIWAD(),
				selectItemByIdx( ui->iwadListView, iwadIdx );  // which is needed, so that the map names are ready to be selected when launch options are loaded.
				                                               // If these 2 could be collapsed into a single updateMapsFromIWAD(), that would be great.
			}
			else
			{
				QMessageBox::warning( this, "IWAD no longer exists",
					"IWAD selected for this preset ("%preset.selectedIWAD%") no longer exists, please select another one." );
				deselectSelectedItems( ui->iwadListView );  // this invokes the callback, which updates the preset and clears the list of maps
			}
		}
		else
		{
			QMessageBox::warning( this, "IWAD no longer exists",
				"IWAD selected for this preset ("%preset.selectedIWAD%") no longer exists, please select another one." );
			deselectSelectedItems( ui->iwadListView );  // this invokes the callback, which updates the preset and clears the list of maps
		}
	}
	else
	{
		deselectSelectedItems( ui->iwadListView );  // this invokes the callback, which updates the preset and clears the list of maps
	}

	// restore selected MapPack
	QList< TreePosition > selectedMapPacks = preset.selectedMapPacks;  // make a copy before it is deleted in the callback
	deselectSelectedItems( ui->mapDirView );  // this invokes the callback, which updates the preset and clears the list
	for (const TreePosition & pos : selectedMapPacks)
	{
		QModelIndex mapIdx = mapModel.getNodeByPosition( pos );
		if (mapIdx.isValid())
		{
			if (QFileInfo::exists( mapModel.getFSPath( mapIdx ) ))
			{
				selectItemByIdx( ui->mapDirView, mapIdx );  // this invokes the callback, which updates the preset
			}
			else
			{
				QMessageBox::warning( this, "Map file no longer exists",
					"Map file selected for this preset ("%pos.toString()%") no longer exists." );
			}
		}
		else
		{
			QMessageBox::warning( this, "Map file no longer exists",
				"Map file selected for this preset ("%pos.toString()%") no longer exists." );
		}
	}

	// restore list of mods
	deselectSelectedItems( ui->modListView );  // this actually doesn't call a toggle callback, because the list is checkbox-based
	modModel.startCompleteUpdate();
	modModel.clear();
	for (auto modIt = preset.mods.begin(); modIt != preset.mods.end(); )  // need iterator, so that we can erase non-existing
	{
		const Mod & mod = *modIt;

		if (QFileInfo::exists( mod.path ))
		{
			modModel.append( mod );
			++modIt;
		}
		else
		{
			QMessageBox::warning( this, "Mod no longer exists",
				"A mod from the preset ("%mod.path%") no longer exists. It will be removed from the list." );
			modIt = preset.mods.erase( modIt );  // keep the list widget in sync with the preset list
		}
	}
	modModel.finishCompleteUpdate();

	if (optsStorage == STORE_TO_PRESET)
	{
		restoreLaunchOptions( preset.opts );
	}

	// restore additional command line arguments
	ui->presetCmdArgsLine->setText( preset.cmdArgs );

	updateLaunchCommand();
}

void MainWindow::togglePresetSubWidgets( bool enabled )
{
	ui->engineCmbBox->setEnabled( enabled );
	ui->configCmbBox->setEnabled( enabled );
	ui->configAddBtn->setEnabled( enabled );
	ui->iwadListView->setEnabled( enabled );
	ui->mapDirView->setEnabled( enabled );
	ui->modListView->setEnabled( enabled );
	ui->modBtnAdd->setEnabled( enabled );
	ui->modBtnAddDir->setEnabled( enabled );
	ui->modBtnDel->setEnabled( enabled );
	ui->modBtnUp->setEnabled( enabled );
	ui->modBtnDown->setEnabled( enabled );
	ui->presetCmdArgsLine->setEnabled( enabled );
}

void MainWindow::clearPresetSubWidgets()
{
	ui->engineCmbBox->setCurrentIndex( -1 );
	// map names, saves and demos will be cleared and deselected automatically

	deselectSelectedItems( ui->iwadListView );
	deselectSelectedItems( ui->mapDirView );
	deselectSelectedItems( ui->modListView );

	modModel.startCompleteUpdate();
	modModel.clear();
	modModel.finishCompleteUpdate();

	ui->presetCmdArgsLine->clear();
}


//----------------------------------------------------------------------------------------------------------------------
//  item selection

void MainWindow::selectEngine( int index )
{
	// sometimes, when doing list updates, we don't want this to happen
	if (disableSelectionCallbacks)
		return;

	// update the current preset
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedPresetIdx >= 0)
	{
		if (index < 0)  // engine combo box was reset to "no engine selected" state
			presetModel[ selectedPresetIdx ].selectedEnginePath.clear();
		else
			presetModel[ selectedPresetIdx ].selectedEnginePath = engineModel[ index ].path;
	}

	if (index >= 0)
	{
		verifyFile( engineModel[ index ].path, "The selected engine (%1) no longer exists, please select another one." );
	}

	updateConfigFilesFromDir();
	updateSaveFilesFromDir();
	updateDemoFilesFromDir();

	updateLaunchCommand();
}

void MainWindow::selectConfig( int index )
{
	// sometimes, when doing list updates, we don't want this to happen
	if (disableSelectionCallbacks)
		return;

	// update the current preset
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedPresetIdx >= 0)
	{
		if (index < 0)  // config combo box was reset to "no config selected" state
			presetModel[ selectedPresetIdx ].selectedConfig.clear();
		else
			presetModel[ selectedPresetIdx ].selectedConfig = configModel[ index ].fileName;
	}

	bool validConfigSelected = false;

	if (index > 0)  // at index 0 there is an empty placeholder to allow deselecting config
	{
		QString configPath = getPathFromFileName(
			engineModel[ ui->engineCmbBox->currentIndex() ].configDir,  // if config was selected, engine selection must be valid too
			configModel[ index ].fileName
		);
		validConfigSelected = verifyFile( configPath, "The selected config (%1) no longer exists, please select another one." );
	}

	// update related UI elements
	ui->configAddBtn->setEnabled( validConfigSelected );

	updateLaunchCommand();
}

void MainWindow::togglePreset( const QItemSelection & selected, const QItemSelection & /*deselected*/ )
{
	if (!selected.indexes().isEmpty())
	{
		togglePresetSubWidgets( true );  // enable all widgets that contain preset settings
		restorePreset( selected.indexes()[0].row() );  // load the content of the selected preset into the other widgets
	}
	else  // the preset was deselected using CTRL
	{
		clearPresetSubWidgets();  // clear the other widgets that display the content of the preset
		togglePresetSubWidgets( false );  // disable the widgets so that user can't enter data that would not be saved anywhere
	}
}

void MainWindow::toggleIWAD( const QItemSelection & selected, const QItemSelection & /*deselected*/ )
{
	// sometimes, when doing list updates, we don't want this to happen
	if (disableSelectionCallbacks)
		return;

	int selectedIWADIdx = selected.indexes().isEmpty() ? -1 : selected.indexes()[0].row();

	// update the current preset
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedPresetIdx >= 0)
	{
		if (selectedIWADIdx < 0)
			presetModel[ selectedPresetIdx ].selectedIWAD.clear();
		else
			presetModel[ selectedPresetIdx ].selectedIWAD = iwadModel[ selectedIWADIdx ].path;
	}

	if (selectedIWADIdx >= 0)
	{
		verifyFile( iwadModel[ selectedIWADIdx ].path, "The selected IWAD (%1) no longer exists, please select another one." );
	}

	updateMapsFromIWAD();

	updateLaunchCommand();
}

void MainWindow::toggleMapPack( const QItemSelection & selected, const QItemSelection & /*deselected*/ )
{
	// sometimes, when doing list updates, we don't want this to happen
	if (disableSelectionCallbacks)
		return;

	QList< TreePosition > selectedMapPacks;
	const auto selectedIndexes = selected.indexes();
	for (const QModelIndex & index : selectedIndexes)
		selectedMapPacks.append( mapModel.getNodePosition( index ) );

	// update the current preset
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedPresetIdx >= 0)
	{
		presetModel[ selectedPresetIdx ].selectedMapPacks = selectedMapPacks;
	}

	updateLaunchCommand();
}

void MainWindow::modDataChanged( const QModelIndex & topLeft, const QModelIndex & bottomRight, const QVector<int> & roles )
{
	int topModIdx = topLeft.row();
	int bottomModIdx = bottomRight.row();

	if (roles.contains( Qt::CheckStateRole ))  // check state of some checkboxes changed
	{
		// update the current preset
		int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
		if (selectedPresetIdx >= 0)
		{
			for (int idx = topModIdx; idx <= bottomModIdx; idx++)
			{
				presetModel[ selectedPresetIdx ].mods[ idx ].checked = modModel[ idx ].checked;
			}
		}

		updateLaunchCommand();
	}
}

void MainWindow::showMapPackDesc( const QModelIndex & index )
{
	QString mapDataFilePath = mapModel.getFSPath( index );
	QString mapDescFileName = QFileInfo( mapDataFilePath ).completeBaseName() + ".txt";
	QString mapDescFilePath = mapDataFilePath.mid( 0, mapDataFilePath.lastIndexOf('.') ) + ".txt";  // QFileInfo won't help with this

	if (!QFileInfo::exists( mapDescFilePath ))
	{
		qWarning() << "Map description file ("%mapDescFileName%") does not exist";
		return;
	}

	QFile descFile( mapDescFilePath );
	if (!descFile.open( QIODevice::Text | QIODevice::ReadOnly ))
	{
		QMessageBox::warning( this, "Cannot open map description",
			"Failed to open description file "%mapDescFileName%": "%descFile.errorString() );
		return;
	}

	QByteArray desc = descFile.readAll();

	QDialog descDialog( this );
	descDialog.setObjectName( "MapDescription" );
	descDialog.setWindowTitle( "Map pack description" );
	descDialog.setWindowModality( Qt::WindowModal );

	QVBoxLayout * layout = new QVBoxLayout( &descDialog );

	QPlainTextEdit * textEdit = new QPlainTextEdit( &descDialog );
	textEdit->setReadOnly( true );
	textEdit->setWordWrapMode( QTextOption::NoWrap );
	QFont font = QFontDatabase::systemFont( QFontDatabase::FixedFont );
	font.setPointSize( 10 );
	textEdit->setFont( font );

	textEdit->setPlainText( desc );

	layout->addWidget( textEdit );

	// estimate the optimal window size
	int dialogWidth  = int( 75.0f * float( font.pointSize() ) * 0.84f ) + 30;
	int dialogHeight = int( 40.0f * float( font.pointSize() ) * 1.62f ) + 30;
	descDialog.resize( dialogWidth, dialogHeight );

	// position it to the right of map widget
	int windowCenterX = this->pos().x() + (this->width() / 2);
	int windowCenterY = this->pos().y() + (this->height() / 2);
	descDialog.move( windowCenterX, windowCenterY - (descDialog.height() / 2) );

	descDialog.exec();
}


//----------------------------------------------------------------------------------------------------------------------
//  preset list manipulation

void MainWindow::presetAdd()
{
  static uint presetNum = 1;

	appendItem( ui->presetListView, presetModel, { "Preset"+QString::number( presetNum++ ) } );

	// clear the widgets to represent an empty preset
	// the widgets must be cleared AFTER the new preset is added and selected, otherwise it will clear the prev preset
	clearPresetSubWidgets();

	// open edit mode so that user can name the preset
	ui->presetListView->edit( presetModel.index( presetModel.count() - 1, 0 ) );
}

void MainWindow::presetDelete()
{
	QMessageBox::StandardButton reply = QMessageBox::question( this,
		"Delete preset?", "Are you sure you want to delete this preset?", QMessageBox::Yes | QMessageBox::No
	);
	if (reply != QMessageBox::Yes)
		return;

	deleteSelectedItem( ui->presetListView, presetModel );

	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedPresetIdx >= 0)
	{
		restorePreset( selectedPresetIdx );
	}
	else
	{
		clearPresetSubWidgets();
		togglePresetSubWidgets( false );  // disable the widgets so that user can't enter data that would not be saved anywhere
	}
}

void MainWindow::presetClone()
{
	int origIdx = cloneSelectedItem( ui->presetListView, presetModel );

	if (origIdx >= 0)
	{
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
	                                             "Doom mod files (*.wad *.WAD *.deh *.DEH *.bex *.BEX *.pk3 *.PK3 *.pk7 *.PK7 *.zip *.ZIP *.7z *.7Z);;"
	                                             "DukeNukem data files (*.grp *.rff);;"
	                                             "All files (*)" );
	if (path.isEmpty())  // user probably clicked cancel
		return;

	// the path comming out of the file dialog is always absolute
	if (pathContext.useRelativePaths())
		path = pathContext.getRelativePath( path );

	Mod mod( QFileInfo( path ), true );

	appendItem( ui->modListView, modModel, mod );

	// add it also to the current preset
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedPresetIdx >= 0)
	{
		presetModel[ selectedPresetIdx ].mods.append( mod );
	}

	updateLaunchCommand();
}

void MainWindow::modAddDir()
{
	QString path = QFileDialog::getExistingDirectory( this, "Locate the mod directory", modSettings.dir );
	if (path.isEmpty())  // user probably clicked cancel
		return;

	// the path comming out of the file dialog is always absolute
	if (pathContext.useRelativePaths())
		path = pathContext.getRelativePath( path );

	Mod mod( QFileInfo( path ), true );

	appendItem( ui->modListView, modModel, mod );

	// add it also to the current preset
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedPresetIdx >= 0)
	{
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
	if (selectedPresetIdx >= 0)
	{
		int deletedCnt = 0;
		for (int deletedIdx : deletedIndexes)
		{
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
	if (selectedPresetIdx >= 0)
	{
		for (int movedIdx : movedIndexes)
		{
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
	if (selectedPresetIdx >= 0)
	{
		for (int movedIdx : movedIndexes)
		{
			presetModel[ selectedPresetIdx ].mods.move( movedIdx, movedIdx + 1 );
		}
	}

	updateLaunchCommand();
}

void MainWindow::modsDropped( int /*row*/, int /*count*/ )
{
	// update the preset
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedPresetIdx >= 0)
	{
		presetModel[ selectedPresetIdx ].mods = modModel.list();  // not the most optimal way, but the size of the list will be always small
	}

	updateLaunchCommand();
}


//----------------------------------------------------------------------------------------------------------------------
//  keyboard control

/*void MainWindow::modConfirm()
{
	int selectedModIdx = getSelectedItemIdx( ui->modListView );
	if (selectedModIdx >= 0)
	{
		modModel[ selectedModIdx ].checked = !modModel[ selectedModIdx ].checked;
		modModel.contentChanged( selectedModIdx, selectedModIdx + 1 );
		toggleMod( modModel.makeIndex( selectedModIdx ) );
	}
}*/


//----------------------------------------------------------------------------------------------------------------------
//  launch options

void MainWindow::modeStandard()
{
	STORE_OPTION( mode, STANDARD )

	ui->mapCmbBox->setEnabled( false );
	ui->saveFileCmbBox->setEnabled( false );
	ui->mapCmbBox_demo->setEnabled( false );
	ui->demoFileLine_record->setEnabled( false );
	ui->demoFileCmbBox_replay->setEnabled( false );

	toggleOptionsSubwidgets( false );

	if (ui->multiplayerChkBox->isChecked() && ui->multRoleCmbBox->currentIndex() == SERVER)
	{
		ui->multRoleCmbBox->setCurrentIndex( CLIENT );   // only client can use standard launch mode
	}

	updateLaunchCommand();
}

void MainWindow::modeLaunchMap()
{
	STORE_OPTION( mode, LAUNCH_MAP )

	ui->mapCmbBox->setEnabled( true );
	ui->saveFileCmbBox->setEnabled( false );
	ui->mapCmbBox_demo->setEnabled( false );
	ui->demoFileLine_record->setEnabled( false );
	ui->demoFileCmbBox_replay->setEnabled( false );

	toggleOptionsSubwidgets( true );

	if (ui->multiplayerChkBox->isChecked() && ui->multRoleCmbBox->currentIndex() == CLIENT)
	{
		ui->multRoleCmbBox->setCurrentIndex( SERVER );   // only server can select a map
	}

	updateLaunchCommand();
}

void MainWindow::modeSavedGame()
{
	STORE_OPTION( mode, LOAD_SAVE )

	ui->mapCmbBox->setEnabled( false );
	ui->saveFileCmbBox->setEnabled( true );
	ui->mapCmbBox_demo->setEnabled( false );
	ui->demoFileLine_record->setEnabled( false );
	ui->demoFileCmbBox_replay->setEnabled( false );

	toggleOptionsSubwidgets( false );

	updateLaunchCommand();
}

void MainWindow::modeRecordDemo()
{
	STORE_OPTION( mode, RECORD_DEMO )

	ui->mapCmbBox->setEnabled( false );
	ui->saveFileCmbBox->setEnabled( false );
	ui->mapCmbBox_demo->setEnabled( true );
	ui->demoFileLine_record->setEnabled( true );
	ui->demoFileCmbBox_replay->setEnabled( false );

	toggleOptionsSubwidgets( true );

	updateLaunchCommand();
}

void MainWindow::modeReplayDemo()
{
	STORE_OPTION( mode, REPLAY_DEMO )

	ui->mapCmbBox->setEnabled( false );
	ui->saveFileCmbBox->setEnabled( false );
	ui->mapCmbBox_demo->setEnabled( false );
	ui->demoFileLine_record->setEnabled( false );
	ui->demoFileCmbBox_replay->setEnabled( true );

	toggleOptionsSubwidgets( false );

	ui->multiplayerChkBox->setEnabled( false );   // no multiplayer when replaying demo

	updateLaunchCommand();
}

void MainWindow::toggleOptionsSubwidgets( bool enabled )
{
	ui->skillCmbBox->setEnabled( enabled );
	ui->skillSpinBox->setEnabled( enabled && ui->skillCmbBox->currentIndex() == Skill::CUSTOM );
	ui->noMonstersChkBox->setEnabled( enabled );
	ui->fastMonstersChkBox->setEnabled( enabled );
	ui->monstersRespawnChkBox->setEnabled( enabled );
	ui->gameOptsBtn->setEnabled( enabled );
	ui->compatOptsBtn->setEnabled( enabled );
}

void MainWindow::selectMap( const QString & mapName )
{
	// sometimes, when doing list updates, we don't want this to happen
	if (disableSelectionCallbacks)
		return;

	STORE_OPTION( mapName, mapName )

	updateLaunchCommand();
}

void MainWindow::selectSavedGame( int saveIdx )
{
	// sometimes, when doing list updates, we don't want this to happen
	if (disableSelectionCallbacks)
		return;

	QString saveFileName = saveIdx >= 0 ? saveModel[ saveIdx ].fileName : "";

	STORE_OPTION( saveFile, saveFileName )

	updateLaunchCommand();
}

void MainWindow::selectMap_demo( const QString & mapName )
{
	// sometimes, when doing list updates, we don't want this to happen
	if (disableSelectionCallbacks)
		return;

	STORE_OPTION( mapName_demo, mapName )

	updateLaunchCommand();
}

void MainWindow::changeDemoFile_record( const QString & fileName )
{
	STORE_OPTION( demoFile_record, fileName )

	updateLaunchCommand();
}

void MainWindow::selectDemoFile_replay( int demoIdx )
{
	// sometimes, when doing list updates, we don't want this to happen
	if (disableSelectionCallbacks)
		return;

	QString demoFileName = demoIdx >= 0 ? demoModel[ demoIdx ].fileName : "";

	STORE_OPTION( demoFile_replay, demoFileName )

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
	STORE_OPTION( skillNum, uint( skill ) )

	if (skill < Skill::CUSTOM)
		ui->skillCmbBox->setCurrentIndex( skill );

	updateLaunchCommand();
}

void MainWindow::toggleNoMonsters( bool checked )
{
	STORE_OPTION( noMonsters, checked )

	updateLaunchCommand();
}

void MainWindow::toggleFastMonsters( bool checked )
{
	STORE_OPTION( fastMonsters, checked )

	updateLaunchCommand();
}

void MainWindow::toggleMonstersRespawn( bool checked )
{
	STORE_OPTION( monstersRespawn, checked )

	updateLaunchCommand();
}

void MainWindow::toggleAllowCheats( bool checked )
{
	STORE_OPTION( allowCheats, checked )

	updateLaunchCommand();
}

void MainWindow::selectMonitor( int index )
{
	STORE_OPTION( monitorIdx, index )

	updateLaunchCommand();
}

void MainWindow::changeResolutionX( const QString & xStr )
{
	STORE_OPTION( resolutionX, xStr.toUInt() )

	updateLaunchCommand();
}

void MainWindow::changeResolutionY( const QString & yStr )
{
	STORE_OPTION( resolutionY, yStr.toUInt() )

	updateLaunchCommand();
}

void MainWindow::toggleNoSound( bool checked )
{
	STORE_OPTION( noSound, checked )

	updateLaunchCommand();
}

void MainWindow::toggleNoSFX( bool checked )
{
	STORE_OPTION( noSFX, checked )

	updateLaunchCommand();
}

void MainWindow::toggleNoMusic( bool checked )
{
	STORE_OPTION( noMusic, checked )

	updateLaunchCommand();
}

void MainWindow::changeSaveDir( const QString & dir )
{
	STORE_OPTION( saveDir, dir )

	if (isValidDir( dir ))
		updateSaveFilesFromDir();

	updateLaunchCommand();
}

void MainWindow::changeScreenshotDir( const QString & dir )
{
	STORE_OPTION( screenshotDir, dir )

	updateLaunchCommand();
}

void MainWindow::browseSaveDir()
{
	QString path = QFileDialog::getExistingDirectory( this, "Locate the directory with saves", pathContext.baseDir().path() );
	if (path.isEmpty())  // user probably clicked cancel
		return;

	// the path comming out of the file dialog is always absolute
	if (pathContext.useRelativePaths())
		path = pathContext.getRelativePath( path );

	ui->saveDirLine->setText( path );
	// the rest is done in saveDirLine callback
}

void MainWindow::browseScreenshotDir()
{
	QString path = QFileDialog::getExistingDirectory( this, "Locate the directory for screenshots", pathContext.baseDir().path() );
	if (path.isEmpty())  // user probably clicked cancel
		return;

	// the path comming out of the file dialog is always absolute
	if (pathContext.useRelativePaths())
		path = pathContext.getRelativePath( path );

	ui->screenshotDirLine->setText( path );
	// the rest is done in screenshotDirLine callback
}

void MainWindow::toggleMultiplayer( bool checked )
{
	STORE_OPTION( isMultiplayer, checked )

	int multRole = ui->multRoleCmbBox->currentIndex();
	int gameMode = ui->gameModeCmbBox->currentIndex();
	bool isDeathMatch = gameMode >= DEATHMATCH && gameMode <= ALT_TEAM_DEATHMATCH;
	bool isTeamPlay = gameMode == TEAM_DEATHMATCH || gameMode == ALT_TEAM_DEATHMATCH || gameMode == COOPERATIVE;

	ui->multRoleCmbBox->setEnabled( checked );
	ui->hostnameLine->setEnabled( checked && multRole == CLIENT );
	ui->portSpinBox->setEnabled( checked );
	ui->netModeCmbBox->setEnabled( checked );
	ui->gameModeCmbBox->setEnabled( checked && multRole == SERVER );
	ui->playerCountSpinBox->setEnabled( checked && multRole == SERVER );
	ui->teamDmgSpinBox->setEnabled( checked && multRole == SERVER && isTeamPlay );
	ui->timeLimitSpinBox->setEnabled( checked && multRole == SERVER && isDeathMatch );
	ui->fragLimitSpinBox->setEnabled( checked && multRole == SERVER && isDeathMatch );

	if (checked)
	{
		if (ui->launchMode_map->isChecked() && multRole == CLIENT)  // client doesn't select map, server does
		{
			ui->launchMode_standard->click();
		}
		if (ui->launchMode_standard->isChecked() && multRole == SERVER)  // server MUST choose a map
		{
			ui->launchMode_map->click();
		}
		if (ui->launchMode_replayDemo->isChecked())  // can't replay demo in multiplayer
		{
			ui->launchMode_standard->click();
		}
	}

	updateLaunchCommand();
}

void MainWindow::selectMultRole( int role )
{
	STORE_OPTION( multRole, MultRole( role ) )

	bool multEnabled = ui->multiplayerChkBox->isChecked();
	int gameMode = ui->gameModeCmbBox->currentIndex();
	bool isDeathMatch = gameMode >= DEATHMATCH && gameMode <= ALT_TEAM_DEATHMATCH;
	bool isTeamPlay = gameMode == TEAM_DEATHMATCH || gameMode == ALT_TEAM_DEATHMATCH || gameMode == COOPERATIVE;

	ui->hostnameLine->setEnabled( multEnabled && role == CLIENT );
	ui->gameModeCmbBox->setEnabled( multEnabled && role == SERVER );
	ui->playerCountSpinBox->setEnabled( multEnabled && role == SERVER );
	ui->teamDmgSpinBox->setEnabled( multEnabled && role == SERVER && isTeamPlay );
	ui->timeLimitSpinBox->setEnabled( multEnabled && role == SERVER && isDeathMatch );
	ui->fragLimitSpinBox->setEnabled( multEnabled && role == SERVER && isDeathMatch );

	if (multEnabled)
	{
		if (ui->launchMode_map->isChecked() && role == CLIENT)  // client doesn't select map, server does
		{
			ui->launchMode_standard->click();
		}
		if (ui->launchMode_standard->isChecked() && role == SERVER)  // server MUST choose a map
		{
			ui->launchMode_map->click();
		}
	}

	updateLaunchCommand();
}

void MainWindow::changeHost( const QString & hostName )
{
	STORE_OPTION( hostName, hostName )

	updateLaunchCommand();
}

void MainWindow::changePort( int port )
{
	STORE_OPTION( port, uint16_t( port ) )

	updateLaunchCommand();
}

void MainWindow::selectNetMode( int netMode )
{
	STORE_OPTION( netMode, NetMode( netMode ) )

	updateLaunchCommand();
}

void MainWindow::selectGameMode( int gameMode )
{
	STORE_OPTION( gameMode, GameMode( gameMode ) )

	bool multEnabled = ui->multiplayerChkBox->isChecked();
	int multRole = ui->multRoleCmbBox->currentIndex();
	bool isDeathMatch = gameMode >= DEATHMATCH && gameMode <= ALT_TEAM_DEATHMATCH;
	bool isTeamPlay = gameMode == TEAM_DEATHMATCH || gameMode == ALT_TEAM_DEATHMATCH || gameMode == COOPERATIVE;

	ui->teamDmgSpinBox->setEnabled( multEnabled && multRole == SERVER && isTeamPlay );
	ui->timeLimitSpinBox->setEnabled( multEnabled && multRole == SERVER && isDeathMatch );
	ui->fragLimitSpinBox->setEnabled( multEnabled && multRole == SERVER && isDeathMatch );

	updateLaunchCommand();
}

void MainWindow::changePlayerCount( int count )
{
	STORE_OPTION( playerCount, uint( count ) )

	updateLaunchCommand();
}

void MainWindow::changeTeamDamage( double damage )
{
	STORE_OPTION( teamDamage, damage )

	updateLaunchCommand();
}

void MainWindow::changeTimeLimit( int timeLimit )
{
	STORE_OPTION( timeLimit, uint( timeLimit ) )

	updateLaunchCommand();
}

void MainWindow::changeFragLimit( int fragLimit )
{
	STORE_OPTION( fragLimit, uint( fragLimit ) )

	updateLaunchCommand();
}

//----------------------------------------------------------------------------------------------------------------------
//  additional command line arguments

void MainWindow::updatePresetCmdArgs( const QString & text )
{
	// update the current preset
	int selectedPresetIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedPresetIdx >= 0)
	{
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
	pathContext.toggleAbsolutePaths( absolute );

	for (Engine & engine : engineModel)
	{
		engine.path = pathContext.convertPath( engine.path );
		engine.configDir = pathContext.convertPath( engine.configDir );
	}

	iwadSettings.dir = pathContext.convertPath( iwadSettings.dir );
	for (IWAD & iwad : iwadModel)
	{
		iwad.path = pathContext.convertPath( iwad.path );
	}

	mapSettings.dir = pathContext.convertPath( mapSettings.dir );
	mapModel.setBaseDir( mapSettings.dir );

	modSettings.dir = pathContext.convertPath( modSettings.dir );
	for (Mod & mod : modModel)
	{
		mod.path = pathContext.convertPath( mod.path );
	}

	for (Preset & preset : presetModel)
	{
		for (Mod & mod : preset.mods)
		{
			mod.path = pathContext.convertPath( mod.path );
		}
		preset.selectedEnginePath = pathContext.convertPath( preset.selectedEnginePath );
		preset.selectedIWAD = pathContext.convertPath( preset.selectedIWAD );
	}

	updateLaunchCommand();
}


//----------------------------------------------------------------------------------------------------------------------
//  automatic list updates according to directory content

// All lists must be updated with special care. In some widgets, when a selection is reset it calls our "item selected"
// callback and that causes the command to regenerate. Which means on every update tick the command is changed back and
// forth - first time when the old item is deselected when the list is cleared and second time when the new item is
// selected after the list is filled. This has an unplesant effect that it isn't possible to inspect the whole launch
// command or copy from it, because your cursor is cursor position is constantly reset by the constant updates.
// We work around this by setting a flag before the update starts and unsetting it after the update finishes. This flag
// is then used to abort the callbacks that are called during the update.
// However the originally selected file might have been deleted from the directory and then it is no longer in the list
// to be re-selected, so we have to manually notify the callbacks (which were disabled before) that the selection was
// reset, so that everything updates correctly.

void MainWindow::updateListsFromDirs()
{
	if (iwadSettings.updateFromDir)
		updateIWADsFromDir();
	updateMapPacksFromDir();
	updateConfigFilesFromDir();
	updateSaveFilesFromDir();
	updateDemoFilesFromDir();
}

void MainWindow::updateIWADsFromDir()
{
	QItemSelection origSelection = ui->iwadListView->selectionModel()->selection();

	// workaround (read the big comment above)
	disableSelectionCallbacks = true;

	updateListFromDir< IWAD >( iwadModel, ui->iwadListView, iwadSettings.dir, iwadSettings.searchSubdirs, pathContext, isIWAD );

	disableSelectionCallbacks = false;

	QItemSelection newSelection = ui->iwadListView->selectionModel()->selection();
	if (newSelection != origSelection)
	{
		// selection changed while the callbacks were disabled, we need to call them manually
		toggleIWAD( newSelection, origSelection );
	}
}

void MainWindow::updateMapPacksFromDir()
{
	QItemSelection origSelection = ui->mapDirView->selectionModel()->selection();

	// workaround (read the big comment above)
	disableSelectionCallbacks = true;

	updateTreeFromDir( mapModel, ui->mapDirView, mapSettings.dir, pathContext, isMapPack );

	disableSelectionCallbacks = false;

	QItemSelection newSelection = ui->mapDirView->selectionModel()->selection();
	if (newSelection != origSelection)
	{
		// selection changed while the callbacks were disabled, we need to call them manually
		toggleMapPack( newSelection, origSelection );
	}
}

void MainWindow::updateConfigFilesFromDir()
{
	int currentEngineIdx = ui->engineCmbBox->currentIndex();

	QString configDir = currentEngineIdx >= 0 ? engineModel[ currentEngineIdx ].configDir : "";

	// workaround (read the big comment above)
	int origConfigIdx = ui->configCmbBox->currentIndex();

	disableSelectionCallbacks = true;  // workaround (read the big comment above)

	updateComboBoxFromDir( configModel, ui->configCmbBox, configDir, /*recursively*/false, /*emptyItem*/true, pathContext,
		/*isDesiredFile*/[]( const QFileInfo & file ) { return configFileSuffixes.contains( file.suffix().toLower() ); }
	);

	disableSelectionCallbacks = false;

	int newConfigIdx = ui->configCmbBox->currentIndex();
	if (newConfigIdx != origConfigIdx)
	{
		// selection changed while the callbacks were disabled, we need to call them manually
		selectConfig( newConfigIdx );
	}
}

void MainWindow::updateSaveFilesFromDir()
{
	int currentEngineIdx = ui->engineCmbBox->currentIndex();

	QString saveDir =
		!ui->saveDirLine->text().isEmpty()  // if custom save dir is specified
			? ui->saveDirLine->text()       // then use it
			: currentEngineIdx >= 0 ? engineModel[ currentEngineIdx ].configDir : "";  // otherwise use config dir

	// workaround (read the big comment above)
	int origSaveIdx = ui->saveFileCmbBox->currentIndex();

	disableSelectionCallbacks = true;  // workaround (read the big comment above)

	updateComboBoxFromDir( saveModel, ui->saveFileCmbBox, saveDir, /*recursively*/false, /*emptyItem*/false, pathContext,
		/*isDesiredFile*/[]( const QFileInfo & file ) { return file.suffix().toLower() == saveFileSuffix; }
	);

	disableSelectionCallbacks = false;

	int newSaveIdx = ui->saveFileCmbBox->currentIndex();
	if (newSaveIdx != origSaveIdx)
	{
		// selection changed while the callbacks were disabled, we need to call them manually
		selectSavedGame( newSaveIdx );
	}
}

void MainWindow::updateDemoFilesFromDir()
{
	int currentEngineIdx = ui->engineCmbBox->currentIndex();
	QString demoDir = currentEngineIdx >= 0 ? engineModel[ currentEngineIdx ].configDir : "";

	// workaround (read the big comment above)
	int origDemoIdx = ui->demoFileCmbBox_replay->currentIndex();

	disableSelectionCallbacks = true;  // workaround (read the big comment above)

	updateComboBoxFromDir( demoModel, ui->demoFileCmbBox_replay, demoDir, /*recursively*/false, /*emptyItem*/false, pathContext,
		/*isDesiredFile*/[]( const QFileInfo & file ) { return file.suffix().toLower() == demoFileSuffix; }
	);

	disableSelectionCallbacks = false;

	int newDemoIdx = ui->demoFileCmbBox_replay->currentIndex();
	if (newDemoIdx != origDemoIdx)
	{
		// selection changed while the callbacks were disabled, we need to call them manually
		selectDemoFile_replay( newDemoIdx );
	}
}

// this is not called regularly, but only when an IWAD is selected or deselected
void MainWindow::updateMapsFromIWAD()
{
	// note down the currently selected item
	QString origText = ui->mapCmbBox->currentText();
	QString origText_demo = ui->mapCmbBox_demo->currentText();

	// TODO: refactor to a single shared mapModel

	disableSelectionCallbacks = true;  // workaround (read the big comment above)

	ui->mapCmbBox->setCurrentIndex( -1 );
	ui->mapCmbBox->clear();
	ui->mapCmbBox_demo->setCurrentIndex( -1 );
	ui->mapCmbBox_demo->clear();

	int iwadIdx = getSelectedItemIdx( ui->iwadListView );
	if (iwadIdx < 0)
	{
		disableSelectionCallbacks = false;
		return;
	}

	// read the map names from file
	const WadInfo & wadInfo = getCachedWadInfo( iwadModel[ iwadIdx ].path );

	// fill the combox-box
	if (wadInfo.successfullyRead && !wadInfo.mapNames.isEmpty())
	{
		for (const QString & mapName : wadInfo.mapNames)
		{
			ui->mapCmbBox->addItem( mapName );
			ui->mapCmbBox_demo->addItem( mapName );
		}
	}
	else  // if we haven't found any map names in the IWAD, fallback to the standard DOOM 2 names
	{
		for (int i = 1; i <= 32; i++)
		{
			QString mapName = QStringLiteral("MAP%1").arg( i, 2, 10, QChar('0') );
			ui->mapCmbBox->addItem( mapName );
			ui->mapCmbBox_demo->addItem( mapName );
		}
	}

	// restore the originally selected item
	int newIdx = ui->mapCmbBox->findText( origText );
	ui->mapCmbBox->setCurrentIndex( newIdx );
	int newIdx_demo = ui->mapCmbBox_demo->findText( origText_demo );
	ui->mapCmbBox_demo->setCurrentIndex( newIdx_demo );

	disableSelectionCallbacks = false;

	if (ui->mapCmbBox->currentText() != origText)
	{
		// selection changed while the callbacks were disabled, we need to call them manually
		selectMap( origText );
	}
	if (ui->mapCmbBox_demo->currentText() != origText_demo)
	{
		// selection changed while the callbacks were disabled, we need to call them manually
		selectMap_demo( origText_demo );
	}
}


//----------------------------------------------------------------------------------------------------------------------
//  saving and loading user data entered into the launcher

void MainWindow::saveOptions( const QString & filePath )
{
	QJsonObject jsRoot;

	// this will be used to detect options created by older versions and supress "missing element" warnings
	jsRoot["version"] = appVersion;

	{
		QJsonObject jsGeometry;

		const QRect & geometry = this->geometry();
		jsGeometry["width"] = geometry.width();
		jsGeometry["height"] = geometry.height();

		jsRoot["geometry"] = jsGeometry;
	}

	jsRoot["check_for_updates"] = checkForUpdates;

	jsRoot["use_absolute_paths"] = pathContext.useAbsolutePaths();

	jsRoot["options_storage"] = int( optsStorage );

	jsRoot["close_on_launch"] = closeOnLaunch;

	{
		QJsonObject jsEngines;

		jsEngines["engines"] = serializeList( engineModel.list() );

		jsRoot["engines"] = jsEngines;
	}

	{
		QJsonObject jsIWADs = serialize( iwadSettings );

		if (!iwadSettings.updateFromDir)
			jsIWADs["IWADs"] = serializeList( iwadModel.list() );

		jsRoot["IWADs"] = jsIWADs;
	}

	jsRoot["maps"] = serialize( mapSettings );

	jsRoot["mods"] = serialize( modSettings );

	{
		QJsonArray jsPresetArray;
		for (const Preset & preset : presetModel)
		{
			QJsonObject jsPreset = serialize( preset, optsStorage == STORE_TO_PRESET );
			jsPresetArray.append( jsPreset );
		}
		jsRoot["presets"] = jsPresetArray;
	}

	jsRoot["additional_args"] = ui->globalCmdArgsLine->text();

	if (optsStorage == STORE_GLOBALLY)
	{
		jsRoot["options"] = serialize( opts );
	}

	int presetIdx = getSelectedItemIdx( ui->presetListView );
	jsRoot["selected_preset"] = presetIdx >= 0 ? presetModel[ presetIdx ].name : "";

	QJsonDocument jsonDoc( jsRoot );
	QByteArray rawData = jsonDoc.toJson();

	// Open the file after the JSON is filled up and serialized to minimize the time
	// where the file is empty (after it's opened for writing) and waiting for the data to be written.
	QFile file( filePath );
	if (!file.open( QIODevice::WriteOnly ))
	{
		QMessageBox::warning( this, "Error saving options",
			"Could not open file "%filePath%" for writing: "%file.errorString() );
		return;
	}

	file.write( rawData );
	file.close();

	//return file.error() == QFile::NoError;
}

void MainWindow::loadOptions( const QString & filePath )
{
	QFile file( filePath );
	if (!file.open( QIODevice::ReadOnly ))
	{
		QMessageBox::warning( this, "Error loading options",
			"Could not open file "%filePath%" for reading: "%file.errorString() );
		optionsCorrupted = true;
		return;
	}

	QByteArray data = file.readAll();

	QJsonParseError error;
	QJsonDocument jsonDoc = QJsonDocument::fromJson( data, &error );
	if (jsonDoc.isNull())
	{
		QMessageBox::warning( this, "Error loading options", "Loading options failed: "%error.errorString() );
		optionsCorrupted = true;
		return;
	}

	// We use this contextual mechanism instead of standard JSON getters, because when something fails to load
	// we want to print a useful error message with information exactly which JSON element is broken.
	JsonDocumentCtx jsonDocCtx( jsonDoc );
	const JsonObjectCtx & jsRoot = jsonDocCtx.rootObject();

	// detect that we are loading options from older versions so that we can supress "missing element" warnings
	jsonDocCtx.toggleWarnings( false );  // read version with warnings disabled
	QString version = jsRoot.getString( "version" );
	if (!version.isEmpty() && compareVersions( version, appVersion ) >= 0)  // missing "version" means old version
		jsonDocCtx.toggleWarnings( true );  // only re-enable warnings if the options are up to date

	if (JsonObjectCtx jsGeometry = jsRoot.getObject( "geometry" ))
	{
		int width = jsGeometry.getInt( "width", -1 );
		int height = jsGeometry.getInt( "height", -1 );
		if (width > 0 && height > 0)
			this->resize( width, height );
	}

	// preset must be deselected first, so that the cleared selections doesn't save in the selected preset
	deselectSelectedItems( ui->presetListView );

	checkForUpdates = jsRoot.getBool( "check_for_updates", true );

	pathContext.toggleAbsolutePaths( jsRoot.getBool( "use_absolute_paths", false ) );

	// this must be loaded early, because we need to know whether to attempt loading the opts from the presets
	optsStorage = jsRoot.getEnum< OptionsStorage >( "options_storage", STORE_GLOBALLY );

	closeOnLaunch = jsRoot.getBool( "close_on_launch", false );

	if (JsonObjectCtx jsEngines = jsRoot.getObject( "engines" ))
	{
		disableSelectionCallbacks = true;  // prevent unnecessary reloading of config/save/demo lists

		ui->engineCmbBox->setCurrentIndex( -1 );

		engineModel.startCompleteUpdate();

		engineModel.clear();

		if (JsonArrayCtx jsEngineArray = jsEngines.getArray( "engines" ))
		{
			for (int i = 0; i < jsEngineArray.size(); i++)
			{
				JsonObjectCtx jsEngine = jsEngineArray.getObject( i );
				if (!jsEngine)  // wrong type on position i -> skip this entry
					continue;

				Engine engine;
				deserialize( jsEngine, engine );

				if (engine.path.isEmpty())  // element isn't present in JSON -> skip this entry
					continue;

				if (!verifyFile( engine.path, "An engine from the saved options (%1) no longer exists. It will be removed from the list." ))
					continue;

				engineModel.append( std::move( engine ) );
			}
		}

		engineModel.finishCompleteUpdate();

		disableSelectionCallbacks = false;
	}

	if (JsonObjectCtx jsIWADs = jsRoot.getObject( "IWADs" ))
	{
		deselectSelectedItems( ui->iwadListView );  // this invokes the callback, which updates the dependent widgets

		iwadModel.startCompleteUpdate();

		iwadModel.clear();

		deserialize( jsIWADs, iwadSettings );

		if (iwadSettings.updateFromDir)
		{
			verifyDir( iwadSettings.dir, "IWAD directory from the saved options (%1) no longer exists. Please update it in Menu -> Setup." );
		}
		else
		{
			if (JsonArrayCtx jsIWADArray = jsIWADs.getArray( "IWADs" ))
			{
				for (int i = 0; i < jsIWADArray.size(); i++)
				{
					JsonObjectCtx jsIWAD = jsIWADArray.getObject( i );
					if (!jsIWAD)  // wrong type on position i - skip this entry
						continue;

					IWAD iwad;
					deserialize( jsIWAD, iwad );

					if (iwad.name.isEmpty() || iwad.path.isEmpty())  // element isn't present in JSON -> skip this entry
						continue;

					if (!verifyFile( iwad.path, "An IWAD from the saved options (%1) no longer exists. It will be removed from the list." ))
						continue;

					iwadModel.append( std::move( iwad ) );
				}
			}
		}

		iwadModel.finishCompleteUpdate();
	}

	if (JsonObjectCtx jsMaps = jsRoot.getObject( "maps" ))
	{
		deselectSelectedItems( ui->mapDirView );

		deserialize( jsMaps, mapSettings );

		verifyDir( mapSettings.dir, "Map directory from the saved options (%1) no longer exists. Please update it in Menu -> Setup." );
	}

	if (JsonObjectCtx jsMods = jsRoot.getObject( "mods" ))
	{
		deselectSelectedItems( ui->modListView );

		deserialize( jsMods, modSettings );

		verifyDir( modSettings.dir, "Mod directory from the saved options (%1) no longer exists. Please update it in Menu -> Setup." );
	}

	if (JsonArrayCtx jsPresetArray = jsRoot.getArray( "presets" ))
	{
		deselectSelectedItems( ui->presetListView );  // this invokes the callback, which disables the dependent widgets

		presetModel.startCompleteUpdate();

		presetModel.clear();

		for (int i = 0; i < jsPresetArray.size(); i++)
		{
			JsonObjectCtx jsPreset = jsPresetArray.getObject( i );
			if (!jsPreset)  // wrong type on position i - skip this entry
				continue;

			Preset preset;
			deserialize( jsPreset, preset, optsStorage == STORE_TO_PRESET );

			presetModel.append( std::move( preset ) );
		}

		presetModel.finishCompleteUpdate();
	}

	ui->globalCmdArgsLine->setText( jsRoot.getString( "additional_args" ) );

	// launch options
	if (optsStorage == STORE_GLOBALLY)
	{
		if (JsonObjectCtx jsOptions = jsRoot.getObject( "options" ))
		{
			deserialize( jsOptions, opts );
		}
	}

	// make sure all paths loaded from JSON are stored in correct format
	toggleAbsolutePaths( pathContext.useAbsolutePaths() );

	optionsCorrupted = false;

	file.close();

	// update the lists from which the restored preset will select items
	if (iwadSettings.updateFromDir)
		updateIWADsFromDir();
	updateMapPacksFromDir();
	// the rest of the lists will get updated when an engine is restored from a preset

	// load the last selected preset
	QString selectedPreset = jsRoot.getString( "selected_preset" );
	if (!selectedPreset.isEmpty())
	{
		int selectedPresetIdx = findSuch( presetModel, [ selectedPreset ]( const Preset & preset )
		                                                                 { return preset.name == selectedPreset; } );
		if (selectedPresetIdx >= 0)
		{
			selectItemByIdx( ui->presetListView, selectedPresetIdx );  // this invokes the callback, which enables the dependent widgets
			                                                           // and calls restorePreset(...);
		}
		else
		{
			QMessageBox::warning( nullptr, "Preset no longer exists",
				"Preset that was selected last time ("%selectedPreset%") no longer exists. Did you mess up with the options.json?" );
		}
	}

	// this must be done after the lists are already updated because we want to select existing items in combo boxes,
	//               and after preset loading because the preset will select IWAD which will fill the map combo box
	if (optsStorage == STORE_GLOBALLY)
	{
		restoreLaunchOptions( opts );
	}

	updateLaunchCommand();
}

void MainWindow::restoreLaunchOptions( const LaunchOptions & opts )
{
	// launch mode
	if (opts.mode == STANDARD)
		ui->launchMode_standard->click();
	else if (opts.mode == LAUNCH_MAP)
		ui->launchMode_map->click();
	else if (opts.mode == LOAD_SAVE)
		ui->launchMode_savefile->click();

	// details of launch mode
	ui->mapCmbBox->setCurrentText( opts.mapName );
	if (!opts.saveFile.isEmpty())
	{
		int saveFileIdx = findSuch( saveModel, [ &opts ]( const SaveFile & save ) { return save.fileName == opts.saveFile; } );
		if (saveFileIdx >= 0)
		{
			ui->saveFileCmbBox->setCurrentIndex( saveFileIdx );
		}
		else
		{
			ui->saveFileCmbBox->setCurrentIndex( -1 );
			QMessageBox::warning( this, "Save file no longer exists",
				"Save file \""%opts.saveFile%"\" no longer exists, please select another one." );
		}
	}
	ui->mapCmbBox_demo->setCurrentText( opts.mapName_demo );
	ui->demoFileLine_record->setText( opts.demoFile_record );
	if (!opts.demoFile_replay.isEmpty())
	{
		int demoFileIdx = findSuch( demoModel, [ &opts ]( const DemoFile & demo ) { return demo.fileName == opts.demoFile_replay; } );
		if (demoFileIdx >= 0)
		{
			ui->demoFileCmbBox_replay->setCurrentIndex( demoFileIdx );
		}
		else
		{
			ui->demoFileCmbBox_replay->setCurrentIndex( -1 );
			QMessageBox::warning( this, "Demo file no longer exists",
				"Demo file \""%opts.demoFile_replay%"\" no longer exists, please select another one." );
		}
	}

	// gameplay
	ui->skillSpinBox->setValue( int( opts.skillNum ) );
	ui->noMonstersChkBox->setChecked( opts.noMonsters );
	ui->fastMonstersChkBox->setChecked( opts.fastMonsters );
	ui->monstersRespawnChkBox->setChecked( opts.monstersRespawn );
	compatOptsCmdArgs = CompatOptsDialog::getCmdArgsFromOptions( opts.compatOpts );
	ui->allowCheatsChkBox->setChecked( opts.allowCheats );

	// video
	if (opts.monitorIdx < ui->monitorCmbBox->count())
		ui->monitorCmbBox->setCurrentIndex( opts.monitorIdx );
	if (opts.resolutionX > 0)
		ui->resolutionXLine->setText( QString::number( opts.resolutionX ) );
	if (opts.resolutionY > 0)
		ui->resolutionYLine->setText( QString::number( opts.resolutionY ) );

	// audio
	ui->noSoundChkBox->setChecked( opts.noSound );
	ui->noSfxChkBox->setChecked( opts.noSFX );
	ui->noMusicChkBox->setChecked( opts.noMusic );

	// alternative paths
	ui->saveDirLine->setText( opts.saveDir );
	ui->screenshotDirLine->setText( opts.screenshotDir );

	// multiplayer
	ui->multiplayerChkBox->setChecked( opts.isMultiplayer );
	ui->multRoleCmbBox->setCurrentIndex( int( opts.multRole ) );
	ui->hostnameLine->setText( opts.hostName );
	ui->portSpinBox->setValue( opts.port );
	ui->netModeCmbBox->setCurrentIndex( int( opts.netMode ) );
	ui->gameModeCmbBox->setCurrentIndex( int( opts.gameMode ) );
	ui->playerCountSpinBox->setValue( int( opts.playerCount ) );
	ui->teamDmgSpinBox->setValue( opts.teamDamage );
	ui->timeLimitSpinBox->setValue( int( opts.timeLimit ) );
	ui->fragLimitSpinBox->setValue( int( opts.fragLimit ) );
}

void MainWindow::exportPreset()
{
	int selectedIdx = getSelectedItemIdx( ui->presetListView );
	if (selectedIdx < 0)
	{
		QMessageBox::warning( this, "No preset selected", "Select a preset from the preset list." );
		return;
	}

	QString filePath = QFileDialog::getSaveFileName( this, "Export preset", QString(), scriptFileSuffix );
	if (filePath.isEmpty())  // user probably clicked cancel
	{
		return;
	}

	QFileInfo fileInfo( filePath );

	QFile file( filePath );
	if (!file.open( QIODevice::WriteOnly | QIODevice::Text ))
	{
		QMessageBox::warning( this, "Cannot open file", "Cannot open file for writing. Check directory permissions" );
		return;
	}

	QTextStream stream( &file );

	stream << generateLaunchCommand( fileInfo.path() ) << endl;  // keep endl to maintain compatibility with older Qt

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
	int selectedEngineIdx = ui->engineCmbBox->currentIndex();
	if (selectedEngineIdx < 0)
	{
		ui->commandLine->clear();
		return;  // no sense to generate a command when we don't even know the engine
	}

	QString curCommand = ui->commandLine->text();

	// the command needs to be relative to the engine's directory,
	// because it will be executed with the current directory set to engine's directory
	QString baseDir = getDirOfFile( engineModel[ selectedEngineIdx ].path );

	QString newCommand = generateLaunchCommand( baseDir );

	// Don't replace the line widget's content if there is no change. It would just annoy a user who is trying to select
	// and copy part of the line, by constantly reseting his selection.
	if (newCommand != curCommand)
	{
		//static int i = 0;
		//qDebug() << "updating " << i++;
		ui->commandLine->setText( newCommand );
	}
}

QString MainWindow::generateLaunchCommand( const QString & baseDir )
{
	// baseDir - which base directory to derive the relative paths from

	// All stored paths are relative to pathContext.baseDir(), but we need them relative to baseDir.
	PathContext base( pathContext.useAbsolutePaths(), baseDir, pathContext.baseDir() );

	QString newCommand;
	QTextStream cmdStream( &newCommand, QIODevice::WriteOnly );

	int selectedEngineIdx = ui->engineCmbBox->currentIndex();
	if (selectedEngineIdx >= 0)
	{
		const Engine & selectedEngine = engineModel[ selectedEngineIdx ];

		//verifyFile( selectedEngine.path, "The selected engine (%1) no longer exists. Please update its path in Menu -> Setup." );
		cmdStream << "\"" << base.rebasePath( selectedEngine.path ) << "\"";

		const int configIdx = ui->configCmbBox->currentIndex();
		if (configIdx > 0)  // at index 0 there is an empty placeholder to allow deselecting config
		{
			QString configPath = getPathFromFileName( selectedEngine.configDir, configModel[ configIdx ].fileName );

			//verifyFile( configPath, "The selected config (%1) no longer exists. Please update the config dir in Menu -> Setup" );
			cmdStream << " -config \"" << base.rebasePath( configPath ) << "\"";
		}
	}
	else
	{
		return {};  // no sense to generate a command when we don't even know the engine
	}

	int selectedIwadIdx = getSelectedItemIdx( ui->iwadListView );
	if (selectedIwadIdx >= 0)
	{
		//verifyFile( iwadModel[ selectedIwadIdx ].path, "The selected IWAD (%1) no longer exists. Please update it in Menu -> Setup." );
		cmdStream << " -iwad \"" << base.rebasePath( iwadModel[ selectedIwadIdx ].path ) << "\"";
	}

	for (QModelIndex & selectedMapIdx : getSelectedItemIdxs( ui->mapDirView ))
	{
		if (selectedMapIdx.isValid())
		{
			QString mapFilePath = mapModel.getFSPath( selectedMapIdx );
			//verifyFile( mapFilePath, "The selected map pack (%1) no longer exists. Please select another one." );
			if (QFileInfo( mapFilePath ).suffix().toLower() == "deh")
				cmdStream << " -deh \"" << base.rebasePath( mapFilePath ) << "\"";
			else if (QFileInfo( mapFilePath ).suffix().toLower() == "bex")
				cmdStream << " -bex \"" << base.rebasePath( mapFilePath ) << "\"";
			else
				cmdStream << " -file \"" << base.rebasePath( mapFilePath ) << "\"";
		}
	}

	for (const Mod & mod : modModel)
	{
		if (mod.checked)
		{
			//verifyFile( mod.path, "The selected mod (%1) no longer exists. Please update the mod list." );
			if (QFileInfo( mod.path ).suffix().toLower() == "deh")
				cmdStream << " -deh \"" << base.rebasePath( mod.path ) << "\"";
			else if (QFileInfo( mod.path ).suffix().toLower() == "bex")
				cmdStream << " -bex \"" << base.rebasePath( mod.path ) << "\"";
			else
				cmdStream << " -file \"" << base.rebasePath( mod.path ) << "\"";
		}
	}

	if (ui->allowCheatsChkBox->isChecked())
		cmdStream << " +sv_cheats 1";

	if (ui->monitorCmbBox->currentIndex() > 0)  // the first item is a placeholder for leaving it default
	{
		// terrible hack, but it's not my fault
		int vid_adapter;
		if (selectedEngineIdx >= 0 && getFileNameFromPath( engineModel[ selectedEngineIdx ].path ).startsWith("zdoom"))
			vid_adapter = ui->monitorCmbBox->currentIndex();      // in ZDoom monitors are indexed from 1
		else
			vid_adapter = ui->monitorCmbBox->currentIndex() - 1;  // but in newer derivatives from 0
		cmdStream << " +vid_adapter " << vid_adapter;
	}
	if (!ui->resolutionXLine->text().isEmpty())
		cmdStream << " -width " << ui->resolutionXLine->text();
	if (!ui->resolutionYLine->text().isEmpty())
		cmdStream << " -height " << ui->resolutionYLine->text();

	if (ui->noSoundChkBox->isChecked())
		cmdStream << " -nosound";
	if (ui->noSfxChkBox->isChecked())
		cmdStream << " -nosfx";
	if (ui->noMusicChkBox->isChecked())
		cmdStream << " -nomusic";

	if (!ui->saveDirLine->text().isEmpty())
		cmdStream << " -savedir \"" << ui->saveDirLine->text() << "\"";
	if (!ui->screenshotDirLine->text().isEmpty())
		cmdStream << " +screenshot_dir \"" << ui->screenshotDirLine->text() << "\"";

	if (ui->launchMode_map->isChecked())
	{
		cmdStream << " +map " << ui->mapCmbBox->currentText();
	}
	else if (ui->launchMode_savefile->isChecked() && ui->saveFileCmbBox->currentIndex() >= 0)
	{
		QString savePath = getPathFromFileName( engineModel[ selectedEngineIdx ].configDir, saveModel[ ui->saveFileCmbBox->currentIndex() ].fileName );
		cmdStream << " -loadgame \"" << base.rebasePath( savePath ) << "\"";
	}
	else if (ui->launchMode_recordDemo->isChecked() && !ui->demoFileLine_record->text().isEmpty())
	{
		cmdStream << " -record " << ui->demoFileLine_record->text();
		cmdStream << " +map " << ui->mapCmbBox_demo->currentText();
	}
	else if (ui->launchMode_replayDemo->isChecked() && ui->demoFileCmbBox_replay->currentIndex() >= 0)
	{
		cmdStream << " -playdemo " << ui->demoFileCmbBox_replay->currentText();
	}

	if (ui->launchMode_map->isChecked() || ui->launchMode_recordDemo->isChecked())
	{
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
		if (!compatOptsCmdArgs.isEmpty())
			cmdStream << " " << compatOptsCmdArgs;
	}

	if (ui->multiplayerChkBox->isChecked())
	{
		switch (ui->multRoleCmbBox->currentIndex())
		{
		 case MultRole::SERVER:
			cmdStream << " -host " << ui->playerCountSpinBox->text();
			if (ui->portSpinBox->value() != 5029)
				cmdStream << " -port " << ui->portSpinBox->text();
			switch (ui->gameModeCmbBox->currentIndex())
			{
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
			if (ui->fragLimitSpinBox->value() != 0)
				cmdStream << " +fraglimit " << ui->fragLimitSpinBox->text();
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
	int selectedEngineIdx = ui->engineCmbBox->currentIndex();
	if (selectedEngineIdx < 0)
	{
		QMessageBox::warning( this, "No engine selected", "No Doom engine is selected." );
		return;
	}

	QString currentDir = QDir::currentPath();
	QString engineDir = getDirOfFile( engineModel[ selectedEngineIdx ].path );

	// before we execute the command, we need to switch the current dir to the engine's dir
	// because some engines search for their own files in the current dir and would fail if started from elsewhere
	QDir::setCurrent( engineDir );

	// the command paths are always generated relative to the engine's dir
	bool success = QProcess::startDetached( ui->commandLine->text() );

	// restore the previous current dir
	QDir::setCurrent( currentDir );

	if (!success)
	{
		QMessageBox::warning( this, tr("Launch error"), tr("Failed to execute launch command.") );
		return;
	}

	if (closeOnLaunch)
	{
		QApplication::quit();
	}
}
