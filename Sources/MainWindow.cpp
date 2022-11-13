//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of the main window, including all its tabs
//======================================================================================================================

#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include "AboutDialog.hpp"
#include "SetupDialog.hpp"
#include "OptionsStorageDialog.hpp"
#include "NewConfigDialog.hpp"
#include "GameOptsDialog.hpp"
#include "CompatOptsDialog.hpp"
#include "ProcessOutputWindow.hpp"

#include "OptionsSerializer.hpp"
#include "Version.hpp"
#include "UpdateChecker.hpp"
#include "EngineTraits.hpp"
#include "ColorThemes.hpp"

#include "LangUtils.hpp"
#include "FileSystemUtils.hpp"
#include "OSUtils.hpp"
#include "OwnFileDialog.hpp"
#include "WidgetUtils.hpp"
#include "DoomUtils.hpp"
#include "MiscUtils.hpp"  // checkPath, highlightInvalidPath

#include <QVector>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QStringBuilder>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QFileIconProvider>
#include <QMessageBox>
#include <QTimer>
#include <QProcess>

#include <QVBoxLayout>
#include <QPlainTextEdit>
#include <QFontDatabase>

#include <QDebug>


//======================================================================================================================

static const char defaultOptionsFileName [] = "options.json";

#if IS_WINDOWS
	static const QString scriptFileSuffix = "*.bat";
	static const QString shortcutFileSuffix = "*.lnk";
#else
	static const QString scriptFileSuffix = "*.sh";
#endif

static constexpr bool VerifyPaths = true;
static constexpr bool DontVerifyPaths = false;


//======================================================================================================================
//  MainWindow-specific utils

QStringList MainWindow::getSelectedMapPacks() const
{
	QStringList selectedMapPacks;

	// clicking on an item in QTreeView with QFileSystemModel selects all elements (columns) of a row,
	// but we only care about the first one
	const auto selectedRows = getSelectedRows( ui->mapDirView );

	// extract the file paths
	for (const QModelIndex & index : selectedRows)
	{
		QString modelPath = mapModel.filePath( index );
		selectedMapPacks.append( pathContext.convertPath( modelPath ) );
	}

	return selectedMapPacks;
}

QString MainWindow::getConfigDir() const
{
	int currentEngineIdx = ui->engineCmbBox->currentIndex();
	return currentEngineIdx >= 0 ? engineModel[ currentEngineIdx ].configDir : "";
}

QString MainWindow::getSaveDir() const
{
	QString alternativeSaveDir = ui->saveDirLine->text();
	if (!alternativeSaveDir.isEmpty())  // if custom save dir is specified
		return alternativeSaveDir;      // then use it
	else                                // otherwise
		return getConfigDir();          // use config dir
}

LaunchMode MainWindow::getLaunchModeFromUI() const
{
	if (ui->launchMode_map->isChecked())
		return LaunchMode::LaunchMap;
	else if (ui->launchMode_savefile->isChecked())
		return LaunchMode::LoadSave;
	else if (ui->launchMode_recordDemo->isChecked())
		return LaunchMode::RecordDemo;
	else if (ui->launchMode_replayDemo->isChecked())
		return LaunchMode::ReplayDemo;
	else
		return LaunchMode::Default;
}

LaunchOptions & MainWindow::activeLaunchOptions()
{
	if (settings.launchOptsStorage == StoreToPreset)
	{
		int selectedPresetIdx = getSelectedItemIndex( ui->presetListView );
		if (selectedPresetIdx >= 0)
		{
			return presetModel[ selectedPresetIdx ].launchOpts;
		}
	}

	// We always have to save somewhere, because generateLaunchCommand() might read stored values from it.
	// In case the optsStorage == DontStore, we will just skip serializing it to JSON.
	return launchOpts;
}

MultiplayerOptions & MainWindow::activeMultiplayerOptions()
{
	if (settings.launchOptsStorage == StoreToPreset)
	{
		int selectedPresetIdx = getSelectedItemIndex( ui->presetListView );
		if (selectedPresetIdx >= 0)
		{
			return presetModel[ selectedPresetIdx ].multOpts;
		}
	}

	// We always have to save somewhere, because generateLaunchCommand() might read stored values from it.
	// In case the optsStorage == DontStore, we will just skip serializing it to JSON.
	return multOpts;
}

GameplayOptions & MainWindow::activeGameplayOptions()
{
	if (settings.gameOptsStorage == StoreToPreset)
	{
		int selectedPresetIdx = getSelectedItemIndex( ui->presetListView );
		if (selectedPresetIdx >= 0)
		{
			return presetModel[ selectedPresetIdx ].gameOpts;
		}
	}

	// We always have to save somewhere, because generateLaunchCommand() might read stored values from it.
	// In case the optsStorage == DontStore, we will just skip serializing it to JSON.
	return gameOpts;
}

CompatibilityOptions & MainWindow::activeCompatOptions()
{
	if (settings.compatOptsStorage == StoreToPreset)
	{
		int selectedPresetIdx = getSelectedItemIndex( ui->presetListView );
		if (selectedPresetIdx >= 0)
		{
			return presetModel[ selectedPresetIdx ].compatOpts;
		}
	}

	// We always have to save somewhere, because generateLaunchCommand() might read stored values from it.
	// In case the optsStorage == DontStore, we will just skip serializing it to JSON.
	return compatOpts;
}

VideoOptions & MainWindow::activeVideoOptions()
{
	if (settings.videoOptsStorage == StoreToPreset)
	{
		int selectedPresetIdx = getSelectedItemIndex( ui->presetListView );
		if (selectedPresetIdx >= 0)
		{
			return presetModel[ selectedPresetIdx ].videoOpts;
		}
	}

	// We always have to save somewhere, because generateLaunchCommand() might read stored values from it.
	// In case the optsStorage == DontStore, we will just skip serializing it to JSON.
	return videoOpts;
}

AudioOptions & MainWindow::activeAudioOptions()
{
	if (settings.audioOptsStorage == StoreToPreset)
	{
		int selectedPresetIdx = getSelectedItemIndex( ui->presetListView );
		if (selectedPresetIdx >= 0)
		{
			return presetModel[ selectedPresetIdx ].audioOpts;
		}
	}

	// We always have to save somewhere, because generateLaunchCommand() might read stored values from it.
	// In case the optsStorage == DontStore, we will just skip serializing it to JSON.
	return audioOpts;
}

/// Stores value to a preset if it's selected.
#define STORE_TO_PRESET_IF_SELECTED( presetMember, value ) \
{\
	int selectedPresetIdx = getSelectedItemIndex( ui->presetListView ); \
	if (selectedPresetIdx >= 0) \
	{\
		presetModel[ selectedPresetIdx ].presetMember = (value); \
	}\
}

/// Stores value to a preset if it's selected and if it's safe to do.
#define STORE_TO_PRESET_IF_SAFE( presetMember, value ) \
{\
	/* Only store the value to the preset if we are not restoring from it, */ \
	/* otherwise we would overwrite a stored value we want to restore later. */ \
	if (!restoringPresetInProgress) \
	{\
		int selectedPresetIdx = getSelectedItemIndex( ui->presetListView ); \
		if (selectedPresetIdx >= 0) \
		{\
			presetModel[ selectedPresetIdx ].presetMember = (value); \
		}\
	}\
}

/// Stores value to a global storage if it's safe to do.
#define STORE_TO_GLOBAL_STORAGE_IF_SAFE( globalMember, value ) \
{\
	/* Only store the value to the global options if we are not restoring from it, */ \
	/* otherwise we would overwrite a stored value we want to restore later. */ \
	if (!restoringOptionsInProgress) \
		globalMember = (value); \
}

/// Stores value to a dynamically selected storage if it's possible and if it's safe to do.
#define STORE_TO_DYNAMIC_STORAGE_IF_SAFE( storageSetting, activeStorage, structMember, value ) \
{\
	/* Only store the value to the global options or preset if we are not restoring from the same location, */ \
	/* otherwise we would overwrite a stored value we want to restore later. */ \
	bool preventSaving = (storageSetting == StoreGlobally && restoringOptionsInProgress) \
	                  || (storageSetting == StoreToPreset && restoringPresetInProgress); \
	if (!preventSaving) \
		activeStorage.structMember = (value); \
}

#define STORE_LAUNCH_OPTION( structMember, value ) \
{\
	STORE_TO_DYNAMIC_STORAGE_IF_SAFE( settings.launchOptsStorage, activeLaunchOptions(), structMember, value ) \
}

#define STORE_MULT_OPTION( structMember, value ) \
{\
	STORE_TO_DYNAMIC_STORAGE_IF_SAFE( settings.launchOptsStorage, activeMultiplayerOptions(), structMember, value ) \
}

#define STORE_GAMEPLAY_OPTION( structMember, value ) \
{\
	STORE_TO_DYNAMIC_STORAGE_IF_SAFE( settings.gameOptsStorage, activeGameplayOptions(), structMember, value ) \
}

#define STORE_COMPAT_OPTION( structMember, value ) \
{\
	STORE_TO_DYNAMIC_STORAGE_IF_SAFE( settings.compatOptsStorage, activeCompatOptions(), structMember, value ) \
}

#define STORE_ALT_PATH( structMember, value ) \
{\
	STORE_TO_PRESET_IF_SAFE( altPaths.structMember, value ) \
}

#define STORE_VIDEO_OPTION( structMember, value ) \
{\
	STORE_TO_DYNAMIC_STORAGE_IF_SAFE( settings.videoOptsStorage, activeVideoOptions(), structMember, value ) \
}

#define STORE_AUDIO_OPTION( structMember, value ) \
{\
	STORE_TO_DYNAMIC_STORAGE_IF_SAFE( settings.audioOptsStorage, activeAudioOptions(), structMember, value ) \
}

#define STORE_PRESET_OPTION( structMember, value ) \
{\
	STORE_TO_PRESET_IF_SAFE( structMember, value ) \
}

#define STORE_GLOBAL_OPTION( structMember, value ) \
{\
	STORE_TO_GLOBAL_STORAGE_IF_SAFE( globalOpts.structMember, value ) \
}


//======================================================================================================================
//  MainWindow

MainWindow::MainWindow()
:
	QMainWindow( nullptr ),
	DialogCommon(
		PathContext( QApplication::applicationDirPath(), useAbsolutePathsByDefault )  // all relative paths will internally be stored relative to the application's dir
	),
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
	mapModel(),
	modModel(
		/*makeDisplayString*/ []( const Mod & mod ) { return mod.fileName; }
	),
	presetModel(
		/*makeDisplayString*/ []( const Preset & preset ) { return preset.name; }
	)
{
	ui = new Ui::MainWindow;
	ui->setupUi( this );

	this->setWindowTitle( windowTitle() + ' ' + appVersion );

	// OS-specific initialization

 #if IS_WINDOWS
	// Qt on Windows does not automatically follow OS preferences, so we have to monitor the OS settings for changes
	// and manually change our theme when it does.
	themeWatcher.start();
 #endif

	// setup main menu actions

	connect( ui->initialSetupAction, &QAction::triggered, this, &thisClass::runSetupDialog );
	connect( ui->optionsStorageAction, &QAction::triggered, this, &thisClass::runOptsStorageDialog );
	connect( ui->exportPresetToScriptAction, &QAction::triggered, this, &thisClass::exportPresetToScript );
	connect( ui->exportPresetToShortcutAction, &QAction::triggered, this, &thisClass::exportPresetToShortcut );
	//connect( ui->importPresetAction, &QAction::triggered, this, &thisClass::importPreset );
	connect( ui->aboutAction, &QAction::triggered, this, &thisClass::runAboutDialog );
	connect( ui->exitAction, &QAction::triggered, this, &thisClass::close );

	if (!isWindows())  // Windows-only feature
		ui->exportPresetToShortcutAction->setEnabled( false );

	// setup main list views

	setupPresetList();
	setupIWADList();
	setupMapPackList();
	setupModList();

	// setup combo-boxes

	// we use custom model for engines, because we want to display the same list differently in different window
	ui->engineCmbBox->setModel( &engineModel );
	connect( ui->engineCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectEngine );

	ui->configCmbBox->setModel( &configModel );
	connect( ui->configCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectConfig );

	ui->saveFileCmbBox->setModel( &saveModel );
	connect( ui->saveFileCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectSavedGame );

	ui->demoFileCmbBox_replay->setModel( &demoModel );
	connect( ui->demoFileCmbBox_replay, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectDemoFile_replay );

	connect( ui->mapCmbBox, &QComboBox::currentTextChanged, this, &thisClass::changeMap );
	connect( ui->mapCmbBox_demo, &QComboBox::currentTextChanged, this, &thisClass::changeMap_demo );

	connect( ui->demoFileLine_record, &QLineEdit::textChanged, this, &thisClass::changeDemoFile_record );

	ui->compatLevelCmbBox->addItem("");  // always have this empty item there, so that we can restore index 0

	// setup buttons

	connect( ui->configCloneBtn, &QToolButton::clicked, this, &thisClass::cloneConfig );

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
	connect( ui->launchMode_default, &QRadioButton::clicked, this, &thisClass::modeDefault );
	connect( ui->launchMode_map, &QRadioButton::clicked, this, &thisClass::modeLaunchMap );
	connect( ui->launchMode_savefile, &QRadioButton::clicked, this, &thisClass::modeSavedGame );
	connect( ui->launchMode_recordDemo, &QRadioButton::clicked, this, &thisClass::modeRecordDemo );
	connect( ui->launchMode_replayDemo, &QRadioButton::clicked, this, &thisClass::modeReplayDemo );

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

	// gameplay
	connect( ui->skillCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectSkill );
	connect( ui->skillSpinBox, QOverload<int>::of( &QSpinBox::valueChanged ), this, &thisClass::changeSkillNum );
	connect( ui->noMonstersChkBox, &QCheckBox::toggled, this, &thisClass::toggleNoMonsters );
	connect( ui->fastMonstersChkBox, &QCheckBox::toggled, this, &thisClass::toggleFastMonsters );
	connect( ui->monstersRespawnChkBox, &QCheckBox::toggled, this, &thisClass::toggleMonstersRespawn );
	connect( ui->gameOptsBtn, &QPushButton::clicked, this, &thisClass::runGameOptsDialog );
	connect( ui->compatOptsBtn, &QPushButton::clicked, this, &thisClass::runCompatOptsDialog );
	connect( ui->compatLevelCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectCompatLevel );
	connect( ui->allowCheatsChkBox, &QCheckBox::toggled, this, &thisClass::toggleAllowCheats );

	// alternative paths
	connect( ui->usePresetNameChkBox, &QCheckBox::toggled, this, &thisClass::toggleUsePresetName );
	connect( ui->saveDirLine, &QLineEdit::textChanged, this, &thisClass::changeSaveDir );
	connect( ui->screenshotDirLine, &QLineEdit::textChanged, this, &thisClass::changeScreenshotDir );
	connect( ui->saveDirBtn, &QPushButton::clicked, this, &thisClass::browseSaveDir );
	connect( ui->screenshotDirBtn, &QPushButton::clicked, this, &thisClass::browseScreenshotDir );

	// video
	loadMonitorInfo( ui->monitorCmbBox );
	connect( ui->monitorCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectMonitor );
	connect( ui->resolutionXLine, &QLineEdit::textChanged, this, &thisClass::changeResolutionX );
	connect( ui->resolutionYLine, &QLineEdit::textChanged, this, &thisClass::changeResolutionY );
	connect( ui->showFpsChkBox, &QCheckBox::toggled, this, &thisClass::toggleShowFps );

	// audio
	connect( ui->noSoundChkBox, &QCheckBox::toggled, this, &thisClass::toggleNoSound );
	connect( ui->noSfxChkBox, &QCheckBox::toggled, this, &thisClass::toggleNoSFX);
	connect( ui->noMusicChkBox, &QCheckBox::toggled, this, &thisClass::toggleNoMusic );

	// setup the rest of widgets

	connect( ui->presetCmdArgsLine, &QLineEdit::textChanged, this, &thisClass::updatePresetCmdArgs );
	connect( ui->globalCmdArgsLine, &QLineEdit::textChanged, this, &thisClass::updateGlobalCmdArgs );
	connect( ui->launchBtn, &QPushButton::clicked, this, &thisClass::launch );

	// this will call the function when the window is fully initialized and displayed
	// not sure, which one of these 2 options is better
	//QMetaObject::invokeMethod( this, &thisClass::onWindowShown, Qt::ConnectionType::QueuedConnection ); // this doesn't work in Qt 5.9
	QTimer::singleShot( 0, this, &thisClass::onWindowShown );
}

void MainWindow::setupPresetList()
{
	// connect the view with model
	ui->presetListView->setModel( &presetModel );

	// set selection rules
	ui->presetListView->setSelectionMode( QAbstractItemView::SingleSelection );

	// setup editing and separators
	presetModel.toggleEditing( true );
	ui->presetListView->toggleNameEditing( true );

	// set drag&drop behaviour
	ui->presetListView->toggleIntraWidgetDragAndDrop( true );
	ui->presetListView->toggleInterWidgetDragAndDrop( false );
	ui->presetListView->toggleExternalFileDragAndDrop( false );

	// set reaction when an item is selected
	connect( ui->presetListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &thisClass::togglePreset );

	// setup reaction to key shortcuts and right click
	ui->presetListView->toggleContextMenu( true );
	ui->presetListView->enableItemCloning();
	ui->presetListView->enableInsertSeparator();
	connect( ui->presetListView->addAction, &QAction::triggered, this, &thisClass::presetAdd );
	connect( ui->presetListView->deleteAction, &QAction::triggered, this, &thisClass::presetDelete );
	connect( ui->presetListView->cloneAction, &QAction::triggered, this, &thisClass::presetClone );
	connect( ui->presetListView->moveUpAction, &QAction::triggered, this, &thisClass::presetMoveUp );
	connect( ui->presetListView->moveDownAction, &QAction::triggered, this, &thisClass::presetMoveDown );
	connect( ui->presetListView->insertSeparatorAction, &QAction::triggered, this, &thisClass::presetInsertSeparator );
}

void MainWindow::setupIWADList()
{
	// connect the view with model
	ui->iwadListView->setModel( &iwadModel );

	// set selection rules
	ui->iwadListView->setSelectionMode( QAbstractItemView::SingleSelection );

	// set reaction when an item is selected
	connect( ui->iwadListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &thisClass::toggleIWAD );
}

void MainWindow::setupMapPackList()
{
	// connect the view with model
	ui->mapDirView->setModel( &mapModel );

	// set selection rules
	ui->mapDirView->setSelectionMode( QAbstractItemView::ExtendedSelection );

	// set item filters
	mapModel.setFilter( QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks );
	mapModel.setNameFilters( getModFileSuffixes() );
	mapModel.setNameFilterDisables( false );

	// remove the column names at the top
	ui->mapDirView->setHeaderHidden( true );

	// remove all other columns except the first one with name
	for (int i = 1; i < mapModel.columnCount(); ++i)
		ui->mapDirView->hideColumn(i);

	// make the view display a horizontal scrollbar rather than clipping the items
	ui->mapDirView->toggleAutomaticColumnResizing( true );

	// remove icons
	class EmptyIconProvider : public QFileIconProvider
	{
	 public:
		virtual QIcon icon( IconType ) const override { return QIcon(); }
		virtual QIcon icon( const QFileInfo & ) const override { return QIcon(); }
	};
	mapModel.setIconProvider( new EmptyIconProvider );

	// set drag&drop behaviour
	ui->mapDirView->setDragEnabled( true );
	ui->mapDirView->setDragDropMode( QAbstractItemView::DragOnly );

	// set reaction when an item is selected
	connect( ui->mapDirView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &thisClass::toggleMapPack );
	connect( ui->mapDirView, &QTreeView::doubleClicked, this, &thisClass::showMapPackDesc );
}

void MainWindow::setupModList()
{
	// connect the view with model
	ui->modListView->setModel( &modModel );

	// set selection rules
	ui->modListView->setSelectionMode( QAbstractItemView::ExtendedSelection );

	// setup item checkboxes
	modModel.toggleCheckableItems( true );

	// setup separators
	ui->modListView->toggleNameEditing( true );

	// give the model our path convertor, it will need it for converting paths dropped from directory
	modModel.setPathContext( &pathContext );

	// set drag&drop behaviour
	ui->modListView->toggleIntraWidgetDragAndDrop( true );
	ui->modListView->toggleInterWidgetDragAndDrop( true );
	ui->modListView->toggleExternalFileDragAndDrop( true );
	connect( ui->modListView, QOverload< int, int >::of( &EditableListView::itemsDropped ), this, &thisClass::modsDropped );

	// set reaction when an item is checked or unchecked
	connect( &modModel, &QAbstractListModel::dataChanged, this, &thisClass::modDataChanged );

	// setup reaction to key shortcuts and right click
	ui->modListView->toggleContextMenu( true );
	ui->modListView->enableOpenFileLocation();
	ui->modListView->enableInsertSeparator();
	connect( ui->modListView->addAction, &QAction::triggered, this, &thisClass::modAdd );
	connect( ui->modListView->deleteAction, &QAction::triggered, this, &thisClass::modDelete );
	connect( ui->modListView->moveUpAction, &QAction::triggered, this, &thisClass::modMoveUp );
	connect( ui->modListView->moveDownAction, &QAction::triggered, this, &thisClass::modMoveDown );
	connect( ui->modListView->insertSeparatorAction, &QAction::triggered, this, &thisClass::modInsertSeparator );
}

void MainWindow::loadMonitorInfo( QComboBox * box )
{
	const auto monitors = listMonitors();
	for (const MonitorInfo & monitor : monitors)
	{
		QString monitorDesc;
		QTextStream descStream( &monitorDesc, QIODevice::WriteOnly );

		descStream << monitor.name << " - " << monitor.width << 'x' << monitor.height;
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
	appDataDir.setPath( getThisAppDataDir() );
	if (!appDataDir.exists())
	{
		appDataDir.mkpath(".");
	}

	optionsFilePath = appDataDir.filePath( defaultOptionsFileName );

	// try to load last saved state
	if (isValidFile( optionsFilePath ))
	{
		loadOptions( optionsFilePath );
	}
	else  // this is a first run, perform an initial setup
	{
		runSetupDialog();
	}

	// integrate the loaded storage settings into the titles of options group-boxes
	updateOptionsGrpBoxTitles( settings );

	// if the presets are empty, add a default one so that users don't complain that they can't enter anything
	if (presetModel.isEmpty())
	{
		// This selects the new item, which invokes the callback, which enables the dependent widgets and calls restorePreset(...)
		appendItem( ui->presetListView, presetModel, { "Default" } );

		// automatically select those essential items (engine, IWAD, ...) that are alone in their list
		// This is done to make it as easy as possible for beginners who just downloaded GZDoom and Brutal Doom
		// and want to start it as easily as possible with no interest about all the other possibilities.
		autoselectLoneItems();
	}

	if (settings.checkForUpdates)
	{
		updateChecker.checkForUpdates_async(
			/* result callback */[ this ]( UpdateChecker::Result result, QString /*errorDetail*/, QStringList versionInfo )
			{
				if (result == UpdateChecker::UpdateAvailable)
				{
					settings.checkForUpdates = showUpdateNotification( this, versionInfo, /*checkbox*/true );
				}
				// silently ignore the rest of the results, since nobody asked for anything
			}
		);
	}

	// setup an update timer
	startTimer( 1000 );
}

void MainWindow::updateOptionsGrpBoxTitles( const StorageSettings & storageSettings )
{
	static const char * const optsStorageStrings [] =
	{
		"not stored",
		"stored globally",
		"stored in preset"
	};

	static const auto updateGroupBoxTitle = []( QGroupBox * grpBox, OptionsStorage storage )
	{
		QString newStorageDesc = optsStorageStrings[ storage ] + QStringLiteral(" (configurable)");
		grpBox->setTitle( replaceStringBetween( grpBox->title(), '[', ']', newStorageDesc ) );
	};

	updateGroupBoxTitle( ui->launchModeGrpBox, storageSettings.launchOptsStorage );
	updateGroupBoxTitle( ui->multiplayerGrpBox, storageSettings.launchOptsStorage );
	updateGroupBoxTitle( ui->gameplayGrpBox, storageSettings.gameOptsStorage );
	updateGroupBoxTitle( ui->compatGrpBox, storageSettings.compatOptsStorage );
	updateGroupBoxTitle( ui->videoGrpBox, storageSettings.videoOptsStorage );
	updateGroupBoxTitle( ui->audioGrpBox, storageSettings.audioOptsStorage );
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
		// Because the current dir is now different, the launcher's paths no longer work,
		// so prevent reloading lists from directories because user would lose selection.
		if (!engineRunning)
		{
			updateListsFromDirs();
		}
	}

	if (tickCount % 60 == 0)
	{
		if (!optionsCorrupted)  // don't overwrite existing file with empty data, when there was just one small syntax error
			saveOptions( optionsFilePath );
	}
}

void MainWindow::closeEvent( QCloseEvent * event )
{
	if (!optionsCorrupted)  // don't overwrite existing file with empty data, when there was just one small syntax error
		saveOptions( optionsFilePath );

 #if IS_WINDOWS
	themeWatcher.terminate();
 #endif

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
	AboutDialog dialog( this, settings.checkForUpdates );

	dialog.exec();

	settings.checkForUpdates = dialog.checkForUpdates;
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
		pathContext.baseDir(),
		engineModel.list(),
		iwadModel.list(),
		iwadSettings,
		mapSettings,
		modSettings,
		settings
	);

	int code = dialog.exec();

	// update the data only if user clicked Ok
	if (code == QDialog::Accepted)
	{
		// write down the previously selected items
		auto currentEngine = getCurrentItemID( ui->engineCmbBox, engineModel );
		auto currentIWAD = getCurrentItemID( ui->iwadListView, iwadModel );
		auto selectedIWAD = getSelectedItemID( ui->iwadListView, iwadModel );

		// deselect the items
		ui->engineCmbBox->setCurrentIndex( -1 );
		deselectAllAndUnsetCurrent( ui->iwadListView );

		// make sure all data and indexes are invalidated and no longer used
		engineModel.startCompleteUpdate();
		iwadModel.startCompleteUpdate();

		// update our data from the dialog
		engineModel.assignList( std::move( dialog.engineModel.list() ) );
		updateEngineTraits();  // sync engineTraits with engineModel
		iwadModel.assignList( std::move( dialog.iwadModel.list() ) );
		iwadSettings = dialog.iwadSettings;
		mapSettings = dialog.mapSettings;
		modSettings = dialog.modSettings;
		settings = dialog.settings;
		// update all stored paths
		toggleAbsolutePaths( settings.useAbsolutePaths );
		currentEngine = pathContext.convertPath( currentEngine );
		currentIWAD = pathContext.convertPath( currentIWAD );
		selectedIWAD = pathContext.convertPath( selectedIWAD );

		// notify the widgets to re-draw their content
		engineModel.finishCompleteUpdate();
		iwadModel.finishCompleteUpdate();
		refreshMapPacks();

		// select back the previously selected items
		setCurrentItemByID( ui->engineCmbBox, engineModel, currentEngine );
		setCurrentItemByID( ui->iwadListView, iwadModel, currentIWAD );
		selectItemByID( ui->iwadListView, iwadModel, selectedIWAD );

		updateLaunchCommand();
	}
}

void MainWindow::runOptsStorageDialog()
{
	// The dialog gets a copy of all the required data and when it's confirmed, we copy them back.
	// This allows the user to cancel all the changes he made without having to manually revert them.

	OptionsStorageDialog dialog( this, settings );

	int code = dialog.exec();

	// update the data only if user clicked Ok
	if (code == QDialog::Accepted)
	{
		settings.assign( dialog.storageSettings );

		updateOptionsGrpBoxTitles( settings );

		updateLaunchCommand();
	}
}

void MainWindow::runGameOptsDialog()
{
	GameplayOptions & activeGameOpts = activeGameplayOptions();

	GameOptsDialog dialog( this, activeGameOpts );

	int code = dialog.exec();

	// update the data only if user clicked Ok
	if (code == QDialog::Accepted)
	{
		activeGameOpts.assign( dialog.gameplayDetails );
		updateLaunchCommand();
	}
}

void MainWindow::runCompatOptsDialog()
{
	CompatibilityOptions & activeCompatOpts = activeCompatOptions();

	CompatOptsDialog dialog( this, activeCompatOpts );

	int code = dialog.exec();

	// update the data only if user clicked Ok
	if (code == QDialog::Accepted)
	{
		activeCompatOpts.assign( dialog.compatDetails );
		// cache the command line args string, so that it doesn't need to be regenerated on every command line update
		compatOptsCmdArgs = CompatOptsDialog::getCmdArgsFromOptions( dialog.compatDetails );
		updateLaunchCommand();
	}
}

void MainWindow::cloneConfig()
{
	QDir configDir( engineModel[ ui->engineCmbBox->currentIndex() ].configDir );  // if config was selected, engine selection must be valid too
	QFileInfo oldConfig( configDir.filePath( ui->configCmbBox->currentText() ) );

	NewConfigDialog dialog( this, oldConfig.completeBaseName() );

	int code = dialog.exec();

	// perform the action only if user clicked Ok
	if (code != QDialog::Accepted)
	{
		return;
	}

	QString oldConfigPath = oldConfig.filePath();
	QString newConfigPath = configDir.filePath( dialog.newConfigName + '.' + oldConfig.suffix() );
	bool copied = QFile::copy( oldConfigPath, newConfigPath );
	if (!copied)
	{
		QMessageBox::warning( this, "Error copying file", "Couldn't create file "%newConfigPath%". Check permissions." );
		return;
	}

	// regenerate config list so that we can select it
	updateConfigFilesFromDir();

	// select the new file automatically for convenience
	QString newConfigName = getFileNameFromPath( newConfigPath );
	int newConfigIdx = findSuch( configModel, [&]( const ConfigFile & cfg ) { return cfg.fileName == newConfigName; } );
	if (newConfigIdx >= 0)
	{
		ui->configCmbBox->setCurrentIndex( newConfigIdx );
	}
}


//----------------------------------------------------------------------------------------------------------------------
//  item selection

/// Automatically selects those essential items (like engine of IWAD) that are alone in their list.
void MainWindow::autoselectLoneItems()
{
	if (engineModel.size() == 1 && ui->engineCmbBox->currentIndex() < 0)
		ui->engineCmbBox->setCurrentIndex( 0 );
	if (iwadModel.size() == 1 && !isSomethingSelected( ui->iwadListView ))
		selectAndSetCurrentByIndex( ui->iwadListView, 0 );
}

void MainWindow::togglePreset( const QItemSelection & /*selected*/, const QItemSelection & /*deselected*/ )
{
	int selectedPresetIdx = getSelectedItemIndex( ui->presetListView );
	if (selectedPresetIdx >= 0 && !presetModel[ selectedPresetIdx ].isSeparator)
	{
		togglePresetSubWidgets( true );  // enable all widgets that contain preset settings
		restorePreset( selectedPresetIdx );  // load the content of the selected preset into the other widgets
	}
	else  // the preset was deselected using CTRL
	{
		togglePresetSubWidgets( false );  // disable the widgets so that user can't enter data that would not be saved anywhere
		clearPresetSubWidgets();  // clear the other widgets that display the content of the preset
	}
}

void MainWindow::togglePresetSubWidgets( bool enabled )
{
	ui->engineCmbBox->setEnabled( enabled );
	ui->configCmbBox->setEnabled( enabled );
	ui->configCloneBtn->setEnabled( enabled );
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

	deselectAllAndUnsetCurrent( ui->iwadListView );
	deselectAllAndUnsetCurrent( ui->mapDirView );
	deselectAllAndUnsetCurrent( ui->modListView );

	modModel.startCompleteUpdate();
	modModel.clear();
	modModel.finishCompleteUpdate();

	ui->presetCmdArgsLine->clear();
}

void MainWindow::selectEngine( int index )
{
	if (disableSelectionCallbacks)
		return;

	// update the current preset
	int selectedPresetIdx = getSelectedItemIndex( ui->presetListView );
	if (selectedPresetIdx >= 0 && !restoringPresetInProgress)
	{
		if (index < 0)  // engine combo box was reset to "no engine selected" state
			presetModel[ selectedPresetIdx ].selectedEnginePath.clear();
		else
			presetModel[ selectedPresetIdx ].selectedEnginePath = engineModel[ index ].path;
	}

	bool supportsCustomMapNames = false;

	if (index >= 0)
	{
		if (!isValidFile( engineModel[ index ].path ))
			engineModel[ index ].foregroundColor = Qt::red;

		supportsCustomMapNames = engineTraits[ index ].supportsCustomMapNames();
	}

	// only allow editing, if the engine supports specifying map by its name
	ui->mapCmbBox->setEditable( supportsCustomMapNames );
	ui->mapCmbBox_demo->setEditable( supportsCustomMapNames );

	// automatic alt dirs are derived from engine's config directory, which has now changed, so this need to be refreshed
	if (globalOpts.usePresetNameAsDir)
	{
		QString presetName = selectedPresetIdx >= 0 ? presetModel[ selectedPresetIdx ].name : "";
		setAltDirsRelativeToConfigs( sanitizePath( presetName ) );
	}

	updateConfigFilesFromDir();
	updateSaveFilesFromDir();
	updateDemoFilesFromDir();
	updateCompatLevels();

	updateLaunchCommand();
}

void MainWindow::selectConfig( int index )
{
	if (disableSelectionCallbacks)
		return;

	// update the current preset
	int selectedPresetIdx = getSelectedItemIndex( ui->presetListView );
	if (selectedPresetIdx >= 0 && !restoringPresetInProgress)
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
		validConfigSelected = isValidFile( configPath );
	}

	// update related UI elements
	ui->configCloneBtn->setEnabled( validConfigSelected );

	updateLaunchCommand();
}

void MainWindow::toggleIWAD( const QItemSelection & /*selected*/, const QItemSelection & /*deselected*/ )
{
	if (disableSelectionCallbacks)
		return;

	int selectedIWADIdx = getSelectedItemIndex( ui->iwadListView );

	// update the current preset
	int selectedPresetIdx = getSelectedItemIndex( ui->presetListView );
	if (selectedPresetIdx >= 0 && !restoringPresetInProgress)
	{
		if (selectedIWADIdx < 0)
			presetModel[ selectedPresetIdx ].selectedIWAD.clear();
		else
			presetModel[ selectedPresetIdx ].selectedIWAD = iwadModel[ selectedIWADIdx ].path;
	}

	if (selectedIWADIdx >= 0)
	{
		if (!isValidFile( iwadModel[ selectedIWADIdx ].path ))
			iwadModel[ selectedIWADIdx ].foregroundColor = Qt::red;
	}

	updateMapsFromSelectedWADs();

	updateLaunchCommand();
}

void MainWindow::toggleMapPack( const QItemSelection & /*selected*/, const QItemSelection & /*deselected*/ )
{
	if (disableSelectionCallbacks)
		return;

	QStringList selectedMapPacks = getSelectedMapPacks();

	// update the current preset
	int selectedPresetIdx = getSelectedItemIndex( ui->presetListView );
	if (selectedPresetIdx >= 0 && !restoringPresetInProgress)
	{
		presetModel[ selectedPresetIdx ].selectedMapPacks = selectedMapPacks;
	}

	// expand the parent directory nodes that are collapsed
	const auto selectedRows = getSelectedRows( ui->mapDirView );
	for (const QModelIndex & index : selectedRows)
		expandParentsOfNode( ui->mapDirView, index );

	updateMapsFromSelectedWADs( selectedMapPacks );

	// if this is a known map pack, that starts at different level than the first one, automatically select it
	if (selectedMapPacks.size() >= 1 && !isDirectory( selectedMapPacks[0] ))
	{
		// if there is multiple of them, there isn't really any better solution than to just take the first one
		QString wadFileName = getFileNameFromPath( selectedMapPacks[0] );
		QString startingMap = getStartingMap( wadFileName );
		if (!startingMap.isEmpty())
		{
			if (ui->mapCmbBox->findText( startingMap ) >= 0)
			{
				ui->mapCmbBox->setCurrentText( startingMap );
				ui->mapCmbBox_demo->setCurrentText( startingMap );
			}
			else
			{
				QMessageBox::warning( this, "Cannot set starting map",
					"Starting map "%startingMap%" was not found in the "%wadFileName%". Bug or corrupted file?" );
			}
		}
	}

	updateLaunchCommand();
}

void MainWindow::modDataChanged( const QModelIndex & topLeft, const QModelIndex & bottomRight, const QVector<int> & roles )
{
	int topModIdx = topLeft.row();
	int bottomModIdx = bottomRight.row();

	// update the current preset
	int selectedPresetIdx = getSelectedItemIndex( ui->presetListView );
	if (selectedPresetIdx >= 0 && !restoringPresetInProgress)
	{
		for (int idx = topModIdx; idx <= bottomModIdx; idx++)
		{
			presetModel[ selectedPresetIdx ].mods[ idx ] = modModel[ idx ];
		}
	}

	if (roles.contains( Qt::CheckStateRole ))  // check state of some checkboxes changed
	{
		updateLaunchCommand();
	}
}

void MainWindow::showMapPackDesc( const QModelIndex & index )
{
	QString mapDataFilePath = mapModel.filePath( index );
	QFileInfo mapDataFileInfo( mapDataFilePath );

	if (!mapDataFileInfo.isFile())  // user could click on a directory
	{
		return;
	}

	// get the corresponding file with txt suffix
	QString mapDescFileName = mapDataFileInfo.completeBaseName() + ".txt";
	QString mapDescFilePath = mapDataFilePath.mid( 0, mapDataFilePath.lastIndexOf('.') ) + ".txt";  // QFileInfo won't help with this

	if (!isValidFile( mapDescFilePath ))
	{
		QMessageBox::warning( this, "Cannot open map description",
			"Map description file \""%mapDescFileName%"\" does not exist" );
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

	// automatically select those essential items (engine, IWAD, ...) that are alone in their list
	// This is done to make it as easy as possible for beginners who just downloaded GZDoom and Brutal Doom
	// and want to start it as easily as possible with no interest about all the other possibilities.
	autoselectLoneItems();

	// open edit mode so that user can name the preset
	ui->presetListView->edit( presetModel.index( presetModel.count() - 1, 0 ) );
}

void MainWindow::presetDelete()
{
	int selectedIdx = getSelectedItemIndex( ui->presetListView );
	if (selectedIdx >= 0 && !presetModel[ selectedIdx ].isSeparator)
	{
		QMessageBox::StandardButton reply = QMessageBox::question( this,
			"Delete preset?", "Are you sure you want to delete preset " % presetModel[ selectedIdx ].name % "?",
			QMessageBox::Yes | QMessageBox::No
		);
		if (reply != QMessageBox::Yes)
			return;
	}

	int deletedIdx = deleteSelectedItem( ui->presetListView, presetModel );
	if (deletedIdx < 0)  // no item was selected
		return;

	int selectedPresetIdx = getSelectedItemIndex( ui->presetListView );
	if (selectedPresetIdx >= 0)
	{
		restorePreset( selectedPresetIdx );
	}
	else
	{
		togglePresetSubWidgets( false );  // disable the widgets so that user can't enter data that would not be saved anywhere
		clearPresetSubWidgets();
	}
}

void MainWindow::presetClone()
{
	int origIdx = cloneSelectedItem( ui->presetListView, presetModel );

	if (origIdx >= 0)
	{
		// open edit mode so that user can name the preset
		editItemAtIndex( ui->presetListView, presetModel.size() - 1 );
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

void MainWindow::presetInsertSeparator()
{
	Preset separator;
	separator.isSeparator = true;
	separator.name = "New Separator";

	int selectedIdx = getSelectedItemIndex( ui->presetListView );
	int insertIdx = selectedIdx < 0 ? presetModel.size() : selectedIdx;  // append if none

	insertItem( ui->presetListView, presetModel, separator, insertIdx );

	// open edit mode so that user can name the preset
	editItemAtIndex( ui->presetListView, insertIdx );
}


//----------------------------------------------------------------------------------------------------------------------
//  mod list manipulation

void MainWindow::modAdd()
{
	QStringList paths = OwnFileDialog::getOpenFileNames( this, "Locate the mod file", modSettings.dir,
		  makeFileFilter( "Doom mod files", pwadSuffixes )
		+ makeFileFilter( "DukeNukem data files", dukeSuffixes )
		+ "All files (*)"
	);
	if (paths.isEmpty())  // user probably clicked cancel
		return;

	// the path comming out of the file dialog is always absolute
	if (pathContext.usingRelativePaths())
		for (QString & path : paths)
			path = pathContext.getRelativePath( path );

	int selectedPresetIdx = getSelectedItemIndex( ui->presetListView );

	for (const QString & path : paths)
	{
		Mod mod( QFileInfo( path ), true );

		appendItem( ui->modListView, modModel, mod );

		// add it also to the current preset
		if (selectedPresetIdx >= 0)
		{
			presetModel[ selectedPresetIdx ].mods.append( mod );
		}
	}

	updateLaunchCommand();
}

void MainWindow::modAddDir()
{
	QString path = OwnFileDialog::getExistingDirectory( this, "Locate the mod directory", modSettings.dir );

	if (path.isEmpty())  // user probably clicked cancel
		return;

	// the path comming out of the file dialog is always absolute
	if (pathContext.usingRelativePaths())
		path = pathContext.getRelativePath( path );

	Mod mod( QFileInfo( path ), true );

	appendItem( ui->modListView, modModel, mod );

	// add it also to the current preset
	int selectedPresetIdx = getSelectedItemIndex( ui->presetListView );
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
	int selectedPresetIdx = getSelectedItemIndex( ui->presetListView );
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
	restoringPresetInProgress = true;  // prevent modDataChanged() from updating our preset too early and incorrectly

	QVector<int> movedIndexes = moveUpSelectedItems( ui->modListView, modModel );

	restoringPresetInProgress = false;

	if (movedIndexes.isEmpty())  // no item was selected or they were already at the top
		return;

	// move it up also in the preset
	int selectedPresetIdx = getSelectedItemIndex( ui->presetListView );
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
	restoringPresetInProgress = true;  // prevent modDataChanged() from updating our preset too early and incorrectly

	QVector<int> movedIndexes = moveDownSelectedItems( ui->modListView, modModel );

	restoringPresetInProgress = false;

	if (movedIndexes.isEmpty())  // no item was selected or they were already at the bottom
		return;

	// move it down also in the preset
	int selectedPresetIdx = getSelectedItemIndex( ui->presetListView );
	if (selectedPresetIdx >= 0)
	{
		for (int movedIdx : movedIndexes)
		{
			presetModel[ selectedPresetIdx ].mods.move( movedIdx, movedIdx + 1 );
		}
	}

	updateLaunchCommand();
}

void MainWindow::modInsertSeparator()
{
	Mod separator;
	separator.isSeparator = true;
	separator.fileName = "New Separator";

	QVector<int> selectedIndexes = getSelectedItemIndexes( ui->modListView );
	int insertIdx = selectedIndexes.empty() ? modModel.size() : selectedIndexes[0];  // append if none

	insertItem( ui->modListView, modModel, separator, insertIdx );

	// insert it also to the current preset
	int selectedPresetIdx = getSelectedItemIndex( ui->presetListView );
	if (selectedPresetIdx >= 0)
	{
		presetModel[ selectedPresetIdx ].mods.insert( insertIdx, separator );
	}

	// open edit mode so that user can name the preset
	editItemAtIndex( ui->modListView, insertIdx );
}

void MainWindow::modsDropped( int dropRow, int count )
{
	// update the preset
	int selectedPresetIdx = getSelectedItemIndex( ui->presetListView );
	if (selectedPresetIdx >= 0)
	{
		presetModel[ selectedPresetIdx ].mods = modModel.list();  // not the most optimal way, but the size of the list will be always small
	}

	// if these files were dragged here from the map pack list, deselect them there
	QDir mapRootDir = mapModel.rootDirectory();
	for (int row = dropRow; row < dropRow + count; ++row)
	{
		const Mod & mod = modModel[ row ];
		if (isInsideDir( mod.path, mapRootDir ))
		{
			QModelIndex mapPackIdx = mapModel.index( mod.path );
			if (mapPackIdx.isValid())
			{
				deselectItemByIndex( ui->mapDirView, mapPackIdx );
			}
		}
	}

	updateLaunchCommand();
}


//----------------------------------------------------------------------------------------------------------------------
//  launch mode

void MainWindow::modeDefault()
{
	STORE_LAUNCH_OPTION( mode, Default )

	ui->mapCmbBox->setEnabled( false );
	ui->saveFileCmbBox->setEnabled( false );
	ui->mapCmbBox_demo->setEnabled( false );
	ui->demoFileLine_record->setEnabled( false );
	ui->demoFileCmbBox_replay->setEnabled( false );

	toggleSkillSubwidgets( false );
	toggleOptionsSubwidgets( true && !ui->multiplayerChkBox->isChecked() );

	if (ui->multiplayerChkBox->isChecked() && ui->multRoleCmbBox->currentIndex() == Server)
	{
		ui->multRoleCmbBox->setCurrentIndex( Client );   // only client can use default launch mode
	}

	updateLaunchCommand();
}

void MainWindow::modeLaunchMap()
{
	STORE_LAUNCH_OPTION( mode, LaunchMap )

	ui->mapCmbBox->setEnabled( true );
	ui->saveFileCmbBox->setEnabled( false );
	ui->mapCmbBox_demo->setEnabled( false );
	ui->demoFileLine_record->setEnabled( false );
	ui->demoFileCmbBox_replay->setEnabled( false );

	toggleSkillSubwidgets( true );
	toggleOptionsSubwidgets( true );

	if (ui->multiplayerChkBox->isChecked() && ui->multRoleCmbBox->currentIndex() == Client)
	{
		ui->multRoleCmbBox->setCurrentIndex( Server );   // only server can select a map
	}

	updateLaunchCommand();
}

void MainWindow::modeSavedGame()
{
	STORE_LAUNCH_OPTION( mode, LoadSave )

	ui->mapCmbBox->setEnabled( false );
	ui->saveFileCmbBox->setEnabled( true );
	ui->mapCmbBox_demo->setEnabled( false );
	ui->demoFileLine_record->setEnabled( false );
	ui->demoFileCmbBox_replay->setEnabled( false );

	toggleSkillSubwidgets( false );
	toggleOptionsSubwidgets( false );

	updateLaunchCommand();
}

void MainWindow::modeRecordDemo()
{
	STORE_LAUNCH_OPTION( mode, RecordDemo )

	ui->mapCmbBox->setEnabled( false );
	ui->saveFileCmbBox->setEnabled( false );
	ui->mapCmbBox_demo->setEnabled( true );
	ui->demoFileLine_record->setEnabled( true );
	ui->demoFileCmbBox_replay->setEnabled( false );

	toggleSkillSubwidgets( true );
	toggleOptionsSubwidgets( true );

	updateLaunchCommand();
}

void MainWindow::modeReplayDemo()
{
	STORE_LAUNCH_OPTION( mode, ReplayDemo )

	ui->mapCmbBox->setEnabled( false );
	ui->saveFileCmbBox->setEnabled( false );
	ui->mapCmbBox_demo->setEnabled( false );
	ui->demoFileLine_record->setEnabled( false );
	ui->demoFileCmbBox_replay->setEnabled( true );

	toggleSkillSubwidgets( false );
	toggleOptionsSubwidgets( false );

	ui->multiplayerChkBox->setChecked( false );   // no multiplayer when replaying demo

	updateLaunchCommand();
}

void MainWindow::toggleSkillSubwidgets( bool enabled )
{
	// skillIdx is an index in the combo-box, which starts from 0, but Doom skill numbers actually start from 1
	int skillNum = ui->skillCmbBox->currentIndex() + 1;

	ui->skillCmbBox->setEnabled( enabled );
	ui->skillSpinBox->setEnabled( enabled && skillNum == Skill::Custom );
}

void MainWindow::toggleOptionsSubwidgets( bool enabled )
{
	ui->noMonstersChkBox->setEnabled( enabled );
	ui->fastMonstersChkBox->setEnabled( enabled );
	ui->monstersRespawnChkBox->setEnabled( enabled );
	ui->gameOptsBtn->setEnabled( enabled );
	ui->compatOptsBtn->setEnabled( enabled );
	ui->compatLevelCmbBox->setEnabled( enabled && lastCompLvlStyle != CompatLevelStyle::None );
}

void MainWindow::changeMap( const QString & mapName )
{
	if (disableSelectionCallbacks)
		return;

	STORE_LAUNCH_OPTION( mapName, mapName )

	updateLaunchCommand();
}

void MainWindow::changeMap_demo( const QString & mapName )
{
	if (disableSelectionCallbacks)
		return;

	STORE_LAUNCH_OPTION( mapName_demo, mapName )

	updateLaunchCommand();
}

void MainWindow::selectSavedGame( int saveIdx )
{
	if (disableSelectionCallbacks)
		return;

	QString saveFileName = saveIdx >= 0 ? saveModel[ saveIdx ].fileName : "";

	STORE_LAUNCH_OPTION( saveFile, saveFileName )

	updateLaunchCommand();
}

void MainWindow::changeDemoFile_record( const QString & fileName )
{
	if (disableSelectionCallbacks)
		return;

	STORE_LAUNCH_OPTION( demoFile_record, fileName )

	updateLaunchCommand();
}

void MainWindow::selectDemoFile_replay( int demoIdx )
{
	if (disableSelectionCallbacks)
		return;

	QString demoFileName = demoIdx >= 0 ? demoModel[ demoIdx ].fileName : "";

	STORE_LAUNCH_OPTION( demoFile_replay, demoFileName )

	updateLaunchCommand();
}


//----------------------------------------------------------------------------------------------------------------------
//  gameplay options

void MainWindow::selectSkill( int skillIdx )
{
	// skillIdx is an index in the combo-box which starts from 0, but Doom skill number actually starts from 1
	int skillNum = skillIdx + 1;

	ui->skillSpinBox->setEnabled( skillNum == Skill::Custom );

	// changeSkillNum() and selectSkill() indirectly invoke each other,
	// this prevents unnecessary updates and potential infinite recursion
	if (disableSelectionCallbacks)
		return;

	ui->skillSpinBox->setValue( skillNum );

	updateLaunchCommand();
}

void MainWindow::changeSkillNum( int skillNum )
{
	STORE_GAMEPLAY_OPTION( skillNum, skillNum )

	// changeSkillNum() and selectSkill() indirectly invoke each other,
	// this prevents unnecessary updates and potential infinite recursion
	disableSelectionCallbacks = true;

	// the combo-box starts from 0, but Doom skill number actually starts from 1
	if (skillNum > 0 && skillNum < Skill::Custom)
		ui->skillCmbBox->setCurrentIndex( skillNum - 1 );
	else
		ui->skillCmbBox->setCurrentIndex( Skill::Custom - 1 );

	disableSelectionCallbacks = false;

	updateLaunchCommand();
}

void MainWindow::toggleNoMonsters( bool checked )
{
	STORE_GAMEPLAY_OPTION( noMonsters, checked )

	updateLaunchCommand();
}

void MainWindow::toggleFastMonsters( bool checked )
{
	STORE_GAMEPLAY_OPTION( fastMonsters, checked )

	updateLaunchCommand();
}

void MainWindow::toggleMonstersRespawn( bool checked )
{
	STORE_GAMEPLAY_OPTION( monstersRespawn, checked )

	updateLaunchCommand();
}

void MainWindow::toggleAllowCheats( bool checked )
{
	STORE_GAMEPLAY_OPTION( allowCheats, checked )

	updateLaunchCommand();
}


//----------------------------------------------------------------------------------------------------------------------
//  compatibility

void MainWindow::selectCompatLevel( int compatLevel )
{
	if (disableSelectionCallbacks)
		return;

	STORE_COMPAT_OPTION( compatLevel, compatLevel - 1 )  // first item is reserved for indicating no selection

	updateLaunchCommand();
}


//----------------------------------------------------------------------------------------------------------------------
//  multiplayer

void MainWindow::toggleMultiplayer( bool checked )
{
	STORE_MULT_OPTION( isMultiplayer, checked )

	int multRole = ui->multRoleCmbBox->currentIndex();
	int gameMode = ui->gameModeCmbBox->currentIndex();
	bool isDeathMatch = gameMode >= Deathmatch && gameMode <= AltTeamDeathmatch;
	bool isTeamPlay = gameMode == TeamDeathmatch || gameMode == AltTeamDeathmatch || gameMode == Cooperative;

	ui->multRoleCmbBox->setEnabled( checked );
	ui->hostnameLine->setEnabled( checked && multRole == Client );
	ui->portSpinBox->setEnabled( checked );
	ui->netModeCmbBox->setEnabled( checked );
	ui->gameModeCmbBox->setEnabled( checked && multRole == Server );
	ui->playerCountSpinBox->setEnabled( checked && multRole == Server );
	ui->teamDmgSpinBox->setEnabled( checked && multRole == Server && isTeamPlay );
	ui->timeLimitSpinBox->setEnabled( checked && multRole == Server && isDeathMatch );
	ui->fragLimitSpinBox->setEnabled( checked && multRole == Server && isDeathMatch );

	LaunchMode launchMode = getLaunchModeFromUI();

	if (checked)
	{
		if (launchMode == LaunchMap && multRole == Client)  // client doesn't select map, server does
		{
			ui->launchMode_default->click();
			launchMode = Default;
		}
		if (launchMode == Default && multRole == Server)  // server MUST choose a map
		{
			ui->launchMode_map->click();
			launchMode = LaunchMap;
		}
		if (launchMode == ReplayDemo)  // can't replay demo in multiplayer
		{
			ui->launchMode_default->click();
			launchMode = Default;
		}
	}

	toggleOptionsSubwidgets(
		(launchMode == Default && !checked) || launchMode == LaunchMap || launchMode == RecordDemo
	);

	updateLaunchCommand();
}

void MainWindow::selectMultRole( int role )
{
	STORE_MULT_OPTION( multRole, MultRole( role ) )

	bool multEnabled = ui->multiplayerChkBox->isChecked();
	int gameMode = ui->gameModeCmbBox->currentIndex();
	bool isDeathMatch = gameMode >= Deathmatch && gameMode <= AltTeamDeathmatch;
	bool isTeamPlay = gameMode == TeamDeathmatch || gameMode == AltTeamDeathmatch || gameMode == Cooperative;

	ui->hostnameLine->setEnabled( multEnabled && role == Client );
	ui->gameModeCmbBox->setEnabled( multEnabled && role == Server );
	ui->playerCountSpinBox->setEnabled( multEnabled && role == Server );
	ui->teamDmgSpinBox->setEnabled( multEnabled && role == Server && isTeamPlay );
	ui->timeLimitSpinBox->setEnabled( multEnabled && role == Server && isDeathMatch );
	ui->fragLimitSpinBox->setEnabled( multEnabled && role == Server && isDeathMatch );

	if (multEnabled)
	{
		if (ui->launchMode_map->isChecked() && role == Client)  // client doesn't select map, server does
		{
			ui->launchMode_default->click();
		}
		if (ui->launchMode_default->isChecked() && role == Server)  // server MUST choose a map
		{
			ui->launchMode_map->click();
		}
	}

	updateLaunchCommand();
}

void MainWindow::changeHost( const QString & hostName )
{
	STORE_MULT_OPTION( hostName, hostName )

	updateLaunchCommand();
}

void MainWindow::changePort( int port )
{
	STORE_MULT_OPTION( port, uint16_t( port ) )

	updateLaunchCommand();
}

void MainWindow::selectNetMode( int netMode )
{
	STORE_MULT_OPTION( netMode, NetMode( netMode ) )

	updateLaunchCommand();
}

void MainWindow::selectGameMode( int gameMode )
{
	STORE_MULT_OPTION( gameMode, GameMode( gameMode ) )

	bool multEnabled = ui->multiplayerChkBox->isChecked();
	int multRole = ui->multRoleCmbBox->currentIndex();
	bool isDeathMatch = gameMode >= Deathmatch && gameMode <= AltTeamDeathmatch;
	bool isTeamPlay = gameMode == TeamDeathmatch || gameMode == AltTeamDeathmatch || gameMode == Cooperative;

	ui->teamDmgSpinBox->setEnabled( multEnabled && multRole == Server && isTeamPlay );
	ui->timeLimitSpinBox->setEnabled( multEnabled && multRole == Server && isDeathMatch );
	ui->fragLimitSpinBox->setEnabled( multEnabled && multRole == Server && isDeathMatch );

	updateLaunchCommand();
}

void MainWindow::changePlayerCount( int count )
{
	STORE_MULT_OPTION( playerCount, uint( count ) )

	updateLaunchCommand();
}

void MainWindow::changeTeamDamage( double damage )
{
	STORE_MULT_OPTION( teamDamage, damage )

	updateLaunchCommand();
}

void MainWindow::changeTimeLimit( int timeLimit )
{
	STORE_MULT_OPTION( timeLimit, uint( timeLimit ) )

	updateLaunchCommand();
}

void MainWindow::changeFragLimit( int fragLimit )
{
	STORE_MULT_OPTION( fragLimit, uint( fragLimit ) )

	updateLaunchCommand();
}


//----------------------------------------------------------------------------------------------------------------------
//  alternative paths

void MainWindow::setAltDirsRelativeToConfigs( const QString & dirName )
{
	int selectedEngineIdx = ui->engineCmbBox->currentIndex();
	if (selectedEngineIdx >= 0)
	{
		QString dirPath = getPathFromFileName( engineModel[ selectedEngineIdx ].configDir, dirName );

		ui->saveDirLine->setText( dirPath );
		// Do not set screenshot_dir for engines that don't support it,
		// some of them are bitchy and won't start if you supply them with unknown command line parameter.
		if (engineTraits[ selectedEngineIdx ].hasScreenshotDirParam())
			ui->screenshotDirLine->setText( dirPath );
		else
			ui->screenshotDirLine->clear();
	}
}

void MainWindow::toggleUsePresetName( bool checked )
{
	STORE_GLOBAL_OPTION( usePresetNameAsDir, checked )

	ui->saveDirLine->setEnabled( !checked );
	ui->saveDirBtn->setEnabled( !checked );
	ui->screenshotDirLine->setEnabled( !checked );
	ui->screenshotDirBtn->setEnabled( !checked );

	if (checked)
	{
		int selectedPresetIdx = getSelectedItemIndex( ui->presetListView );
		QString presetName = selectedPresetIdx >= 0 ? presetModel[ selectedPresetIdx ].name : "";

		// overwrite whatever is currently in the preset or global launch options
		setAltDirsRelativeToConfigs( sanitizePath( presetName ) );
	}
}

void MainWindow::changeSaveDir( const QString & dir )
{
	// Unlike the other launch options, here we in fact want to overwrite the stored value even during restoring,
	// because we want the saved option usePresetNameAsDir to have a priority over the saved directories.
	STORE_TO_PRESET_IF_SELECTED( altPaths.saveDir, dir )

	highlightInvalidDir( ui->saveDirLine, dir );

	if (isValidDir( dir ))
		updateSaveFilesFromDir();

	updateLaunchCommand();
}

void MainWindow::changeScreenshotDir( const QString & dir )
{
	// Unlike the other launch options, here we in fact want to overwrite the stored value even during restoring,
	// because we want the saved option usePresetNameAsDir to have a priority over the saved directories.
	STORE_TO_PRESET_IF_SELECTED( altPaths.screenshotDir, dir )

	highlightInvalidDir( ui->screenshotDirLine, dir );

	updateLaunchCommand();
}

void MainWindow::browseSaveDir()
{
	browseDir( this, "with saves", ui->saveDirLine );
}

void MainWindow::browseScreenshotDir()
{
	browseDir( this, "for screenshots", ui->screenshotDirLine );
}


//----------------------------------------------------------------------------------------------------------------------
//  video options

void MainWindow::selectMonitor( int index )
{
	STORE_VIDEO_OPTION( monitorIdx, index )

	updateLaunchCommand();
}

void MainWindow::changeResolutionX( const QString & xStr )
{
	STORE_VIDEO_OPTION( resolutionX, xStr.toUInt() )

	updateLaunchCommand();
}

void MainWindow::changeResolutionY( const QString & yStr )
{
	STORE_VIDEO_OPTION( resolutionY, yStr.toUInt() )

	updateLaunchCommand();
}

void MainWindow::toggleShowFps( bool checked )
{
	STORE_VIDEO_OPTION( showFPS, checked )

	updateLaunchCommand();
}


//----------------------------------------------------------------------------------------------------------------------
//  audio options

void MainWindow::toggleNoSound( bool checked )
{
	STORE_AUDIO_OPTION( noSound, checked )

	updateLaunchCommand();
}

void MainWindow::toggleNoSFX( bool checked )
{
	STORE_AUDIO_OPTION( noSFX, checked )

	updateLaunchCommand();
}

void MainWindow::toggleNoMusic( bool checked )
{
	STORE_AUDIO_OPTION( noMusic, checked )

	updateLaunchCommand();
}


//----------------------------------------------------------------------------------------------------------------------
//  additional command line arguments

void MainWindow::updatePresetCmdArgs( const QString & text )
{
	STORE_PRESET_OPTION( cmdArgs, text )

	updateLaunchCommand();
}

void MainWindow::updateGlobalCmdArgs( const QString & text )
{
	STORE_GLOBAL_OPTION( cmdArgs, text )

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
	mapModel.setRootPath( mapSettings.dir );

	modSettings.dir = pathContext.convertPath( modSettings.dir );
	for (Mod & mod : modModel)
	{
		mod.path = pathContext.convertPath( mod.path );
	}

	for (Preset & preset : presetModel)
	{
		preset.selectedEnginePath = pathContext.convertPath( preset.selectedEnginePath );
		preset.selectedIWAD = pathContext.convertPath( preset.selectedIWAD );
		for (QString & selectedMapPack : preset.selectedMapPacks)
		{
			selectedMapPack = pathContext.convertPath( selectedMapPack );
		}
		for (Mod & mod : preset.mods)
		{
			mod.path = pathContext.convertPath( mod.path );
		}
	}

	ui->saveDirLine->setText( pathContext.convertPath( ui->saveDirLine->text() ) );
	ui->screenshotDirLine->setText( pathContext.convertPath( ui->screenshotDirLine->text() ) );

	updateLaunchCommand();
}

/// initializes engineTraits to be in sync with current engineModel
void MainWindow::updateEngineTraits()
{
	engineTraits.clear();
	for (const Engine & engine : engineModel)
	{
		engineTraits.append( getEngineTraits( engine ) );
	}
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
	updateConfigFilesFromDir();
	updateSaveFilesFromDir();
	updateDemoFilesFromDir();
}

void MainWindow::updateIWADsFromDir()
{
	auto origSelection = getSelectedItemIDs( ui->iwadListView, iwadModel );

	// workaround (read the big comment above)
	disableSelectionCallbacks = true;

	updateListFromDir< IWAD >( iwadModel, ui->iwadListView, iwadSettings.dir, iwadSettings.searchSubdirs, pathContext, isIWAD );

	disableSelectionCallbacks = false;

	auto newSelection = getSelectedItemIDs( ui->iwadListView, iwadModel );
	if (!areSelectionsEqual( newSelection, origSelection ))
	{
		// selection changed while the callbacks were disabled, we need to call them manually
		toggleIWAD( ui->iwadListView->selectionModel()->selection(), QItemSelection()/*TODO*/ );
	}
}

void MainWindow::refreshMapPacks()
{
	// We use QFileSystemModel which updates the data from directory automatically.
	// But when the directory is changed, the model and view needs to be reset.
	QModelIndex newRootIdx = mapModel.setRootPath( mapSettings.dir );
	ui->mapDirView->setRootIndex( newRootIdx );
}

void MainWindow::updateConfigFilesFromDir()
{
	QString configDir = getConfigDir();

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
	QString saveDir = getSaveDir();

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
	QString demoDir = getSaveDir();

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

/// Updates the compat level combo-box according to the currently selected engine.
void MainWindow::updateCompatLevels()
{
	int selectedEngineIdx = ui->engineCmbBox->currentIndex();
	CompatLevelStyle currentCompLvlStyle = (selectedEngineIdx >= 0)
	                                         ? engineTraits[ selectedEngineIdx ].compatLevelStyle()
	                                         : CompatLevelStyle::None;

	if (currentCompLvlStyle != lastCompLvlStyle)
	{
		disableSelectionCallbacks = true;  // workaround (read the big comment above)

		ui->compatLevelCmbBox->setCurrentIndex( -1 );

		// automatically initialize compat level combox fox according to the selected engine (its compat level options)
		ui->compatLevelCmbBox->clear();
		ui->compatLevelCmbBox->addItem("");  // keep one empty item to allow explicitly deselecting
		if (currentCompLvlStyle != CompatLevelStyle::None)
		{
			for (const QString & compatLvlStr : getCompatLevels( currentCompLvlStyle ))
				ui->compatLevelCmbBox->addItem( compatLvlStr );
		}

		ui->compatLevelCmbBox->setCurrentIndex( 0 );

		disableSelectionCallbacks = false;

		// available compat levels changed -> reset the compat level index, because the previous one is no longer valid
		selectCompatLevel( 0 );

		// keep the widget enabled only if the engine supports compatibility levels
		LaunchMode launchMode = getLaunchModeFromUI();
		ui->compatLevelCmbBox->setEnabled(
			currentCompLvlStyle != CompatLevelStyle::None && launchMode != LoadSave && launchMode != ReplayDemo
		);

		lastCompLvlStyle = currentCompLvlStyle;
	}
}

// this is not called regularly, but only when an IWAD or map WAD is selected or deselected
void MainWindow::updateMapsFromSelectedWADs( std::optional< QStringList > selectedMapPacks )
{
	int selectedIwadIdx = getSelectedItemIndex( ui->iwadListView );
	QString selectedIwadPath = selectedIwadIdx >= 0 ? iwadModel[ selectedIwadIdx ].path : "";

	if (!selectedMapPacks)  // optimization, it the caller already has them, use his ones instead of getting them again
		selectedMapPacks = getSelectedMapPacks();

	// note down the currently selected items
	QString origText = ui->mapCmbBox->currentText();
	QString origText_demo = ui->mapCmbBox_demo->currentText();

	disableSelectionCallbacks = true;  // workaround (read the big comment above)

	do
	{
		// We don't use the model-view architecture like with the rest of combo-boxes,
		// because the editable combo-box doesn't work well with a custom model.

		ui->mapCmbBox->setCurrentIndex( -1 );
		ui->mapCmbBox_demo->setCurrentIndex( -1 );

		ui->mapCmbBox->clear();
		ui->mapCmbBox_demo->clear();

		if (selectedIwadIdx < 0)
		{
			break;  // if no IWAD is selected, let's leave this empty, it cannot be launched anyway
		}

		QStringList selectedWADs = QStringList( selectedIwadPath ) + *selectedMapPacks;

		// read the map names from the selected files and merge them so that entries are not duplicated
		QMap< QString, int > uniqueMapNames;  // we cannot use QSet because that one is unordered and we need to retain order
		for (const QString & selectedWAD : selectedWADs)
		{
			if (!isDirectory( selectedWAD ))
			{
				const WadInfo & wadInfo = getCachedWadInfo( selectedWAD );
				if (wadInfo.successfullyRead)
					for (const QString & mapName : wadInfo.mapNames)
						uniqueMapNames.insert( mapName.toUpper(), 0 );  // the int doesn't matter
			}
		}

		// fill the combox-box
		if (!uniqueMapNames.isEmpty())
		{
			auto mapNames = uniqueMapNames.keys();
			ui->mapCmbBox->addItems( mapNames );
			ui->mapCmbBox_demo->addItems( mapNames );
		}
		else  // if we haven't found any map names in the WADs, fallback to the standard names based on IWAD name
		{
			auto mapNames = getStandardMapNames( getFileNameFromPath( selectedIwadPath ) );
			ui->mapCmbBox->addItems( mapNames );
			ui->mapCmbBox_demo->addItems( mapNames );
		}

		// restore the originally selected item
		ui->mapCmbBox->setCurrentIndex( ui->mapCmbBox->findText( origText ) );
		ui->mapCmbBox_demo->setCurrentIndex( ui->mapCmbBox_demo->findText( origText_demo ) );
	}
	while (false);  // this trick allows us to exit from the block without returning from a function

	disableSelectionCallbacks = false;

	if (ui->mapCmbBox->currentText() != origText)
	{
		// selection changed while the callbacks were disabled, we need to call them manually
		changeMap( ui->mapCmbBox->currentText() );
	}
	if (ui->mapCmbBox_demo->currentText() != origText_demo)
	{
		// selection changed while the callbacks were disabled, we need to call them manually
		changeMap_demo( ui->mapCmbBox->currentText() );
	}
}


//----------------------------------------------------------------------------------------------------------------------
//  saving and loading user data

bool MainWindow::saveOptions( const QString & filePath )
{
	OptionsToSave opts =
	{
		// files
		engineModel.list(),
		iwadModel.list(),

		// options
		launchOpts,
		multOpts,
		gameOpts,
		compatOpts,
		videoOpts,
		audioOpts,
		globalOpts,

		// presets
		presetModel.list(),
		getSelectedItemIndex( ui->presetListView ),

		// global settings
		iwadSettings,
		mapSettings,
		modSettings,
		settings,
		this->geometry()
	};

	return writeOptionsToFile( opts, filePath );
}

bool MainWindow::loadOptions( const QString & filePath )
{
	// Some options can be read directly into the class members using references,
	// but the models can't, because the UI must be prepared for reseting its models first.
	OptionsToLoad opts
	{
		// files - load into intermediate storage
		{},  // engines
		{},  // IWADs

		// options - load directly into this class members
		launchOpts,
		multOpts,
		gameOpts,
		compatOpts,
		videoOpts,
		audioOpts,
		globalOpts,

		// presets - read into intermediate storage
		{},  // presets
		{},  // selected preset

		// global settings - load directly into this class members
		iwadSettings,
		mapSettings,
		modSettings,
		settings,
		{}  // window geometry
	};

	bool optionsRead = readOptionsFromFile( opts, filePath );
	if (!optionsRead)
	{
		if (QFileInfo::exists( filePath ))
			optionsCorrupted = true;  // file exists but cannot be read, don't overwrite it, give user chance to fix it
		return false;
	}

	restoreLoadedOptions( std::move(opts) );

	optionsCorrupted = false;
	return true;
}


//----------------------------------------------------------------------------------------------------------------------
//  restoring stored options into the UI

void MainWindow::restoreLoadedOptions( OptionsToLoad && opts )
{
	setColorTheme( settings.theme );

	if (opts.geometry.width > 0 && opts.geometry.height > 0)
		this->resize( opts.geometry.width, opts.geometry.height );

	// Restoring any stored options is tricky.
	// Every change of a selection, like  cmbBox->setCurrentIndex( stored.idx )  or  selectItemByIdx( stored.idx )
	// causes the corresponding callbacks to be called which in turn might cause some other stored value to be changed.
	// For example  ui->engineCmbBox->setCurrentIndex( idx )  calls  selectEngine( idx )  which might indirectly call
	// selectConfig( idx )  which will overwrite the stored selected config and we will then restore a wrong one.
	// The least complicated workaround seems to be simply setting a flag indicating that we are in the middle of
	// restoring saved options, and then prevent storing values when this flag is set.
	restoringOptionsInProgress = true;

	// preset must be deselected first, so that the cleared selections don't save in the selected preset
	deselectAllAndUnsetCurrent( ui->presetListView );

	// engines
	{
		ui->engineCmbBox->setCurrentIndex( -1 );  // if something is selected, this invokes the callback, which updates the dependent widgets

		engineModel.startCompleteUpdate();
		engineModel.assignList( std::move(opts.engines) );
		updateEngineTraits();  // sync engineTraits with engineModel
		engineModel.finishCompleteUpdate();
	}

	// IWADs
	{
		deselectAllAndUnsetCurrent( ui->iwadListView );  // if something is selected, this invokes the callback, which updates the dependent widgets

		if (iwadSettings.updateFromDir)
		{
			updateIWADsFromDir();  // populates the list from iwadSettings.dir
		}
		else
		{
			iwadModel.startCompleteUpdate();
			iwadModel.assignList( std::move(opts.iwads) );
			iwadModel.finishCompleteUpdate();
		}
	}

	// maps
	{
		deselectAllAndUnsetCurrent( ui->mapDirView );

		refreshMapPacks();  // populates the list from mapSettings.dir
	}

	// mods
	{
		deselectAllAndUnsetCurrent( ui->modListView );

		modModel.startCompleteUpdate();
		modModel.clear();
		// mods will be restored to the UI, when a preset is selected
		modModel.finishCompleteUpdate();
	}

	// presets
	{
		deselectAllAndUnsetCurrent( ui->presetListView );  // this invokes the callback, which disables the dependent widgets

		presetModel.startCompleteUpdate();
		presetModel.assignList( std::move(opts.presets) );
		presetModel.finishCompleteUpdate();
	}

	// make sure all paths loaded from JSON are stored in correct format
	toggleAbsolutePaths( settings.useAbsolutePaths );

	// load the last selected preset
	if (!opts.selectedPreset.isEmpty())
	{
		int selectedPresetIdx = findSuch( presetModel, [&]( const Preset & preset )
		                                               { return preset.name == opts.selectedPreset; } );
		if (selectedPresetIdx >= 0)
		{
			// This invokes the callback, which enables the dependent widgets and calls restorePreset(...)
			selectAndSetCurrentByIndex( ui->presetListView, selectedPresetIdx );
		}
		else
		{
			QMessageBox::warning( nullptr, "Preset no longer exists",
				"Preset that was selected last time ("%opts.selectedPreset%") no longer exists. Did you mess up with the options.json?" );
		}
	}

	// this must be done after the lists are already updated because we want to select existing items in combo boxes,
	//               and after preset loading because the preset will select engine and IWAD which will fill some combo boxes
	if (settings.launchOptsStorage == StoreGlobally)
		restoreLaunchAndMultOptions( launchOpts, multOpts );  // this clears items that are invalid

	if (settings.gameOptsStorage == StoreGlobally)
		restoreGameplayOptions( gameOpts );

	if (settings.compatOptsStorage == StoreGlobally)
		restoreCompatibilityOptions( compatOpts );

	if (settings.videoOptsStorage == StoreGlobally)
		restoreVideoOptions( videoOpts );

	if (settings.audioOptsStorage == StoreGlobally)
		restoreAudioOptions( audioOpts );

	restoreGlobalOptions( globalOpts );

	restoringOptionsInProgress = false;

	updateLaunchCommand();
}

void MainWindow::restorePreset( int presetIdx )
{
	// Restoring any stored options is tricky.
	// Every change of a selection, like  cmbBox->setCurrentIndex( stored.idx )  or  selectItemByIdx( stored.idx )
	// causes the corresponding callbacks to be called which in turn might cause some other stored value to be changed.
	// For example  ui->engineCmbBox->setCurrentIndex( idx )  calls  selectEngine( idx )  which might indirectly call
	// selectConfig( idx )  which will overwrite the stored selected config and we will then restore a wrong one.
	// The least complicated workaround seems to be simply setting a flag indicating that we are in the middle of
	// restoring saved options, and then prevent storing values when this flag is set.

	restoringPresetInProgress = true;

	Preset & preset = presetModel[ presetIdx ];

	// restore selected engine
	{
		disableSelectionCallbacks = true;  // prevent unnecessary widget updates

		ui->engineCmbBox->setCurrentIndex( -1 );

		if (!preset.selectedEnginePath.isEmpty())  // the engine combo box might have been empty when creating this preset
		{
			int engineIdx = findSuch( engineModel, [&]( const Engine & engine )
												   { return engine.path == preset.selectedEnginePath; } );
			if (engineIdx >= 0)
			{
				ui->engineCmbBox->setCurrentIndex( engineIdx );

				if (!isValidFile( preset.selectedEnginePath ))
				{
					QMessageBox::warning( this, "Engine no longer exists",
						"Engine selected for this preset ("%preset.selectedEnginePath%") no longer exists, please update the engines at Menu -> Initial Setup." );
					engineModel[ engineIdx ].foregroundColor = Qt::red;
				}
			}
			else
			{
				QMessageBox::warning( this, "Engine no longer exists",
					"Engine selected for this preset ("%preset.selectedEnginePath%") was removed from engine list, please select another one." );
				preset.selectedEnginePath.clear();
			}
		}

		disableSelectionCallbacks = false;

		// manually notify our class about the change, so that the preset and dependent widgets get updated
		// This is needed before configs, saves and demos are restored, so that the entries are ready to be selected from.
		selectEngine( ui->engineCmbBox->currentIndex() );
	}

	// restore selected config
	{
		disableSelectionCallbacks = true;  // prevent unnecessary widget updates

		ui->configCmbBox->setCurrentIndex( -1 );

		if (!configModel.isEmpty())  // the engine might have not been selected yet so the configs have not been loaded
		{
			int configIdx = findSuch( configModel, [&]( const ConfigFile & config )
												   { return config.fileName == preset.selectedConfig; } );
			if (configIdx >= 0)
			{
				ui->configCmbBox->setCurrentIndex( configIdx );

				// No sense to verify if this config file exists, the configModel has just been updated during
				// selectEngine( ui->engineCmbBox->currentIndex() ), so there are only existing entries.
			}
			else
			{
				QMessageBox::warning( this, "Config no longer exists",
					"Config file selected for this preset ("%preset.selectedConfig%") no longer exists, please select another one." );
				preset.selectedConfig.clear();
			}
		}

		disableSelectionCallbacks = false;

		// manually notify our class about the change, so that the preset and dependent widgets get updated
		selectConfig( ui->configCmbBox->currentIndex() );
	}

	// restore selected IWAD
	{
		disableSelectionCallbacks = true;  // prevent unnecessary widget updates

		deselectSelectedItems( ui->iwadListView );

		if (!preset.selectedIWAD.isEmpty())  // the IWAD may have not been selected when creating this preset
		{
			int iwadIdx = findSuch( iwadModel, [&]( const IWAD & iwad ) { return iwad.path == preset.selectedIWAD; } );
			if (iwadIdx >= 0)
			{
				selectItemByIndex( ui->iwadListView, iwadIdx );

				if (!isValidFile( preset.selectedIWAD ))
				{
					QMessageBox::warning( this, "IWAD no longer exists",
						"IWAD selected for this preset ("%preset.selectedIWAD%") no longer exists, please select another one." );
					iwadModel[ iwadIdx ].foregroundColor = Qt::red;
				}
			}
			else
			{
				QMessageBox::warning( this, "IWAD no longer exists",
					"IWAD selected for this preset ("%preset.selectedIWAD%") no longer exists, please select another one." );
				preset.selectedIWAD.clear();
			}
		}

		disableSelectionCallbacks = false;

		// manually notify our class about the change, so that the preset and dependent widgets get updated
		// This is needed before launch options are restored, so that the map names are ready to be selected.
		toggleIWAD( ui->iwadListView->selectionModel()->selection(), QItemSelection()/*TODO*/ );
	}

	// restore selected MapPack
	{
		disableSelectionCallbacks = true;  // prevent unnecessary widget updates

		QDir mapRootDir = mapModel.rootDirectory();

		deselectSelectedItems( ui->mapDirView );

		const QList< QString > mapPacksCopy = preset.selectedMapPacks;
		preset.selectedMapPacks.clear();  // clear the list in the preset and let it repopulate only with valid items
		for (const QString & path : mapPacksCopy)
		{
			QModelIndex mapIdx = mapModel.index( path );
			if (mapIdx.isValid() && isInsideDir( path, mapRootDir ))
			{
				if (isValidFile( path ))
				{
					preset.selectedMapPacks.append( path );  // put back only items that are valid
					selectItemByIndex( ui->mapDirView, mapIdx );
				}
				else
				{
					QMessageBox::warning( this, "Map file no longer exists",
						"Map file selected for this preset ("%path%") no longer exists." );
				}
			}
			else
			{
				QMessageBox::warning( this, "Map file no longer exists",
					"Map file selected for this preset ("%path%") couldn't be found in the map directory ("%mapRootDir.path()%")." );
			}
		}

		disableSelectionCallbacks = false;

		// manually notify our class about the change, so that the preset and dependent widgets get updated
		// Because this updates the preset with selected items, only the valid ones will be stored and invalid removed.
		toggleMapPack( ui->mapDirView->selectionModel()->selection(), QItemSelection()/*TODO*/ );
	}

	// restore list of mods
	{
		deselectSelectedItems( ui->modListView );  // this actually doesn't call a toggle callback, because the list is checkbox-based

		modModel.startCompleteUpdate();
		modModel.clear();
		for (Mod & mod : preset.mods)
		{
			modModel.append( mod );
			if (!mod.isSeparator && !isValidFile( mod.path ))
			{
				QMessageBox::warning( this, "Mod no longer exists",
					"A mod from the preset ("%mod.path%") no longer exists. Please update it." );
				modModel.last().foregroundColor = Qt::red;
			}
		}
		modModel.finishCompleteUpdate();
	}

	if (settings.launchOptsStorage == StoreToPreset)
		restoreLaunchAndMultOptions( preset.launchOpts, preset.multOpts );  // this clears items that are invalid

	if (settings.gameOptsStorage == StoreToPreset)
		restoreGameplayOptions( preset.gameOpts );

	if (settings.compatOptsStorage == StoreToPreset)
		restoreCompatibilityOptions( preset.compatOpts );

	if (settings.videoOptsStorage == StoreToPreset)
		restoreVideoOptions( preset.videoOpts );

	if (settings.audioOptsStorage == StoreToPreset)
		restoreAudioOptions( preset.audioOpts );

	restoreAlternativePaths( preset.altPaths );

	// restore additional command line arguments
	ui->presetCmdArgsLine->setText( preset.cmdArgs );

	restoringPresetInProgress = false;

	// if "Use preset name as directory" is enabled, overwrite the preset custom directories with its name
	// do it with restoringInProgress == false to update the current preset with the new overwriten dirs.
	if (globalOpts.usePresetNameAsDir)
	{
		setAltDirsRelativeToConfigs( sanitizePath( preset.name ) );
	}

	updateLaunchCommand();
}

void MainWindow::restoreLaunchAndMultOptions( LaunchOptions & launchOpts, const MultiplayerOptions & multOpts )
{
	// launch mode
	switch (launchOpts.mode)
	{
	 case LaunchMode::LaunchMap:
		ui->launchMode_map->click();
		break;
	 case LaunchMode::LoadSave:
		ui->launchMode_savefile->click();
		break;
	 case LaunchMode::RecordDemo:
		ui->launchMode_recordDemo->click();
		break;
	 case LaunchMode::ReplayDemo:
		ui->launchMode_replayDemo->click();
		break;
	 default:
		ui->launchMode_default->click();
		break;
	}

	// details of launch mode
	ui->mapCmbBox->setCurrentIndex( ui->mapCmbBox->findText( launchOpts.mapName ) );
	if (!launchOpts.saveFile.isEmpty())
	{
		int saveFileIdx = findSuch( saveModel, [&]( const SaveFile & save )
		                                       { return save.fileName == launchOpts.saveFile; } );
		ui->saveFileCmbBox->setCurrentIndex( saveFileIdx );

		if (saveFileIdx < 0)
		{
			QMessageBox::warning( this, "Save file no longer exists",
				"Save file \""%launchOpts.saveFile%"\" no longer exists, please select another one." );
			launchOpts.saveFile.clear();  // if previous index was -1, callback is not called, so we clear the invalid item manually
		}
	}
	ui->mapCmbBox_demo->setCurrentIndex( ui->mapCmbBox_demo->findText( launchOpts.mapName_demo ) );
	ui->demoFileLine_record->setText( launchOpts.demoFile_record );
	if (!launchOpts.demoFile_replay.isEmpty())
	{
		int demoFileIdx = findSuch( demoModel, [&]( const DemoFile & demo )
		                                       { return demo.fileName == launchOpts.demoFile_replay; } );
		ui->demoFileCmbBox_replay->setCurrentIndex( demoFileIdx );

		if (demoFileIdx < 0)
		{
			QMessageBox::warning( this, "Demo file no longer exists",
				"Demo file \""%launchOpts.demoFile_replay%"\" no longer exists, please select another one." );
			launchOpts.demoFile_replay.clear();  // if previous index was -1, callback is not called, so we clear the invalid item manually
		}
	}

	// multiplayer
	ui->multiplayerChkBox->setChecked( multOpts.isMultiplayer );
	ui->multRoleCmbBox->setCurrentIndex( int( multOpts.multRole ) );
	ui->hostnameLine->setText( multOpts.hostName );
	ui->portSpinBox->setValue( multOpts.port );
	ui->netModeCmbBox->setCurrentIndex( int( multOpts.netMode ) );
	ui->gameModeCmbBox->setCurrentIndex( int( multOpts.gameMode ) );
	ui->playerCountSpinBox->setValue( int( multOpts.playerCount ) );
	ui->teamDmgSpinBox->setValue( multOpts.teamDamage );
	ui->timeLimitSpinBox->setValue( int( multOpts.timeLimit ) );
	ui->fragLimitSpinBox->setValue( int( multOpts.fragLimit ) );
}

void MainWindow::restoreGameplayOptions( const GameplayOptions & opts )
{
	ui->skillSpinBox->setValue( int( opts.skillNum ) );
	ui->noMonstersChkBox->setChecked( opts.noMonsters );
	ui->fastMonstersChkBox->setChecked( opts.fastMonsters );
	ui->monstersRespawnChkBox->setChecked( opts.monstersRespawn );
	ui->allowCheatsChkBox->setChecked( opts.allowCheats );
}

void MainWindow::restoreCompatibilityOptions( const CompatibilityOptions & opts )
{
	int compatLevelIdx = opts.compatLevel + 1;  // first item is reserved for indicating no selection
	if (compatLevelIdx >= ui->compatLevelCmbBox->count())
	{
		QMessageBox::critical( this, "Cannot restore compat level",
			"Stored compat level is out of bounds of the current combo-box content. Bug?" );
		return;
	}
	ui->compatLevelCmbBox->setCurrentIndex( compatLevelIdx );

	compatOptsCmdArgs = CompatOptsDialog::getCmdArgsFromOptions( opts );
}

void MainWindow::restoreAlternativePaths( const AlternativePaths & opts )
{
	ui->saveDirLine->setText( opts.saveDir );
	ui->screenshotDirLine->setText( opts.screenshotDir );
}

void MainWindow::restoreVideoOptions( const VideoOptions & opts )
{
	if (opts.monitorIdx < ui->monitorCmbBox->count())
		ui->monitorCmbBox->setCurrentIndex( opts.monitorIdx );
	if (opts.resolutionX > 0)
		ui->resolutionXLine->setText( QString::number( opts.resolutionX ) );
	if (opts.resolutionY > 0)
		ui->resolutionYLine->setText( QString::number( opts.resolutionY ) );
	ui->showFpsChkBox->setChecked( opts.showFPS );
}

void MainWindow::restoreAudioOptions( const AudioOptions & opts )
{
	ui->noSoundChkBox->setChecked( opts.noSound );
	ui->noSfxChkBox->setChecked( opts.noSFX );
	ui->noMusicChkBox->setChecked( opts.noMusic );
}

void MainWindow::restoreGlobalOptions( const GlobalOptions & opts )
{
	ui->usePresetNameChkBox->setChecked( opts.usePresetNameAsDir );

	ui->globalCmdArgsLine->setText( opts.cmdArgs );
}


//----------------------------------------------------------------------------------------------------------------------
//  command export

void MainWindow::exportPresetToScript()
{
	int selectedIdx = getSelectedItemIndex( ui->presetListView );
	if (selectedIdx < 0)
	{
		QMessageBox::warning( this, "No preset selected", "Select a preset from the preset list." );
		return;
	}

	QString filePath = OwnFileDialog::getSaveFileName( this, "Export preset", QString(), scriptFileSuffix );
	if (filePath.isEmpty())  // user probably clicked cancel
	{
		return;
	}
	QFileInfo fileInfo( filePath );

	auto cmd = generateLaunchCommand( fileInfo.path(), DontVerifyPaths, QuotePaths );

	QFile file( filePath );
	if (!file.open( QIODevice::WriteOnly | QIODevice::Text ))
	{
		QMessageBox::warning( this, "Cannot open file", "Cannot open file for writing. Check directory permissions." );
		return;
	}

	QTextStream stream( &file );

	stream << cmd.executable << " " << cmd.arguments.join(' ') << endl;  // keep endl to maintain compatibility with older Qt

	file.close();
}

void MainWindow::exportPresetToShortcut()
{
 #if IS_WINDOWS

	int selectedIdx = getSelectedItemIndex( ui->presetListView );
	if (selectedIdx < 0)
	{
		QMessageBox::warning( this, "No preset selected", "Select a preset from the preset list." );
		return;
	}

	const Preset & selectedPreset = presetModel[ selectedIdx ];

	int selectedEngineIdx = ui->engineCmbBox->currentIndex();
	if (selectedEngineIdx < 0)
	{
		QMessageBox::warning( this, "No engine selected", "No Doom engine is selected." );
		return;  // no sense to generate a command when we don't even know the engine
	}

	// The paths in the arguments needs to be relative to the engine's directory
	// because it will be executed with the current directory set to engine's directory,
	// But the executable itself must be either absolute or relative to the current working dir
	// so that it is correctly saved to the shortcut.
	QString enginePath = engineModel[ selectedEngineIdx ].path;
	QString workingDir = getAbsoluteDirOfFile( enginePath );

	QString filePath = OwnFileDialog::getSaveFileName( this, "Export preset", QString(), shortcutFileSuffix );
	if (filePath.isEmpty())  // user probably clicked cancel
	{
		return;
	}

	auto cmd = generateLaunchCommand( workingDir, DontVerifyPaths, QuotePaths );

	bool success = createWindowsShortcut( filePath, enginePath, cmd.arguments, workingDir, selectedPreset.name );
	if (!success)
	{
		QMessageBox::warning( this, "Cannot create shortcut", "Failed to create a shortcut. Check permissions." );
		return;
	}

 #else

	QMessageBox::warning( this, "Not supported", "This feature only works on Windows." );

 #endif
}

void MainWindow::importPresetFromScript()
{
	QMessageBox::warning( this, "Not implemented", "Sorry, this feature is not implemented yet." );
}


//----------------------------------------------------------------------------------------------------------------------
//  launch command generation

void MainWindow::updateLaunchCommand( bool verifyPaths )
{
	// optimization - don't regenerate the command when we're about to make more changes right away
	if (restoringOptionsInProgress || restoringPresetInProgress)
		return;

	//static uint callCnt = 1;
	//qDebug() << "updateLaunchCommand()" << callCnt++;

	int selectedEngineIdx = ui->engineCmbBox->currentIndex();
	if (selectedEngineIdx < 0)
	{
		ui->commandLine->clear();
		return;  // no sense to generate a command when we don't even know the engine
	}

	QString curCommand = ui->commandLine->text();

	// The command needs to be relative to the engine's directory,
	// because it will be executed with the current directory set to engine's directory.
	QString baseDir = getDirOfFile( engineModel[ selectedEngineIdx ].path );

	auto cmd = generateLaunchCommand( baseDir, verifyPaths, QuotePaths );

	QString newCommand = cmd.executable % ' ' % cmd.arguments.join(' ');

	// Don't replace the line widget's content if there is no change. It would just annoy a user who is trying to select
	// and copy part of the line, by constantly reseting his selection.
	if (newCommand != curCommand)
	{
		//static int updateCnt = 1;
		//qDebug() << "    updating " << updateCnt++;
		ui->commandLine->setText( newCommand );
	}
}

MainWindow::ShellCommand MainWindow::generateLaunchCommand( const QString & baseDir, bool verifyPaths, bool quotePaths )
{
	// All stored paths are relative to pathContext.baseDir(), but we need them relative to baseDir.
	PathContext base( baseDir, pathContext.baseDir(), pathContext.pathStyle(), quotePaths );

	ShellCommand cmd;

	int selectedEngineIdx = ui->engineCmbBox->currentIndex();
	if (selectedEngineIdx < 0)
	{
		return {};  // no sense to generate a command when we don't even know the engine
	}

	const Engine & selectedEngine = engineModel[ selectedEngineIdx ];
	const EngineTraits & engineTraits = this->engineTraits[ selectedEngineIdx ];

	{
		assertValidPath( verifyPaths, selectedEngine.path, "The selected engine (%1) no longer exists. Please update its path in Menu -> Setup." );

		// Either the executable is in a search path (C:\Windows\System32, /usr/bin, /snap/bin, ...)
		// in which case it should be (and sometimes must be) started directly by using only its name,
		if (isInSearchPath( selectedEngine.path ))
			cmd.executable = getFileNameFromPath( selectedEngine.path );
		// or it is in the current working directory (because we switched the working dir to the engine's dir),
		// in which case we ignore the absolute paths and add a relative path to the local file (with ./ on Linux).
		else
			cmd.executable = fixExePath( base.rebasePathToRelative( selectedEngine.path ) );

		// On Windows ZDoom doesn't log its output to stdout by default.
		// Force it to do so, so that our ProcessOutputWindow displays something.
		if (isWindows() && settings.showEngineOutput && engineTraits.hasStdoutParam())
			cmd.arguments << "-stdout";

		const int configIdx = ui->configCmbBox->currentIndex();
		if (configIdx > 0)  // at index 0 there is an empty placeholder to allow deselecting config
		{
			QString configPath = getPathFromFileName( selectedEngine.configDir, configModel[ configIdx ].fileName );

			assertValidPath( verifyPaths, configPath, "The selected config (%1) no longer exists. Please update the config dir in Menu -> Setup" );
			cmd.arguments << "-config" << base.rebaseAndQuotePath( configPath );
		}
	}

	int selectedIwadIdx = getSelectedItemIndex( ui->iwadListView );
	if (selectedIwadIdx >= 0)
	{
		assertValidPath( verifyPaths, iwadModel[ selectedIwadIdx ].path, "The selected IWAD (%1) no longer exists. Please select another one." );
		cmd.arguments << "-iwad" << base.rebaseAndQuotePath( iwadModel[ selectedIwadIdx ].path );
	}

	QVector< QString > selectedFiles;

	const QStringList selectedMapPacks = getSelectedMapPacks();
	for (const QString & mapFilePath : selectedMapPacks)
	{
		assertValidPath( verifyPaths, mapFilePath, "The selected map pack (%1) no longer exists. Please select another one." );

		QString suffix = QFileInfo( mapFilePath ).suffix().toLower();
		if (suffix == "deh")
			cmd.arguments << "-deh" << base.rebaseAndQuotePath( mapFilePath );
		else if (suffix == "bex")
			cmd.arguments << "-bex" << base.rebaseAndQuotePath( mapFilePath );
		else
			// combine all files under a single -file parameter
			selectedFiles.append( mapFilePath );
	}

	for (const Mod & mod : modModel)
	{
		if (mod.checked)
		{
			assertValidPath( verifyPaths, mod.path, "The selected mod (%1) no longer exists. Please update the mod list." );

			QString suffix = QFileInfo( mod.path ).suffix().toLower();
			if (suffix == "deh")
				cmd.arguments << "-deh" << base.rebaseAndQuotePath( mod.path );
			else if (suffix == "bex")
				cmd.arguments << "-bex" << base.rebaseAndQuotePath( mod.path );
			else
				// combine all files under a single -file parameter
				selectedFiles.append( mod.path );
		}
	}

	// Older engines only accept single file parameter, we must list them here all together.
	if (!selectedFiles.empty())
		cmd.arguments << "-file";
	for (const QString & filePath : selectedFiles)
		cmd.arguments << base.rebaseAndQuotePath( filePath );

	const GameplayOptions & activeGameOpts = activeGameplayOptions();
	const CompatibilityOptions & activeCompatOpts = activeCompatOptions();

	LaunchMode launchMode = getLaunchModeFromUI();
	if (launchMode == LaunchMap)
	{
		cmd.arguments << engineTraits.getMapArgs( ui->mapCmbBox->currentIndex(), ui->mapCmbBox->currentText() );
	}
	else if (launchMode == LoadSave && !ui->saveFileCmbBox->currentText().isEmpty())
	{
		QString savePath = getPathFromFileName( getSaveDir(), ui->saveFileCmbBox->currentText() );
		assertValidPath( verifyPaths, savePath, "The selected save file (%1) no longer exists. Please select another one." );
		cmd.arguments << "-loadgame" << base.rebaseAndQuotePath( savePath );
	}
	else if (launchMode == RecordDemo && !ui->demoFileLine_record->text().isEmpty())
	{
		QString demoPath = getPathFromFileName( getSaveDir(), ui->demoFileLine_record->text() );
		cmd.arguments << "-record" << base.rebaseAndQuotePath( demoPath );
		cmd.arguments << engineTraits.getMapArgs( ui->mapCmbBox_demo->currentIndex(), ui->mapCmbBox_demo->currentText() );
	}
	else if (launchMode == ReplayDemo && !ui->demoFileCmbBox_replay->currentText().isEmpty())
	{
		QString demoPath = getPathFromFileName( getSaveDir(), ui->demoFileCmbBox_replay->currentText() );
		assertValidPath( verifyPaths, demoPath, "The selected demo file (%1) no longer exists. Please select another one." );
		cmd.arguments << "-playdemo" << base.rebaseAndQuotePath( demoPath );
	}

	if (ui->skillCmbBox->isEnabled())
		cmd.arguments << "-skill" << ui->skillSpinBox->text();
	if (ui->noMonstersChkBox->isEnabled() && ui->noMonstersChkBox->isChecked())
		cmd.arguments << "-nomonsters";
	if (ui->fastMonstersChkBox->isEnabled() && ui->fastMonstersChkBox->isChecked())
		cmd.arguments << "-fast";
	if (ui->monstersRespawnChkBox->isEnabled() && ui->monstersRespawnChkBox->isChecked())
		cmd.arguments << "-respawn";
	if (ui->gameOptsBtn->isEnabled() && activeGameOpts.dmflags1 != 0)
		cmd.arguments << "+dmflags" << QString::number( activeGameOpts.dmflags1 );
	if (ui->gameOptsBtn->isEnabled() && activeGameOpts.dmflags2 != 0)
		cmd.arguments << "+dmflags2" << QString::number( activeGameOpts.dmflags2 );
	if (ui->compatLevelCmbBox->isEnabled() && activeCompatOpts.compatLevel >= 0)
		cmd.arguments << engineTraits.getCompatLevelArgs( activeCompatOpts.compatLevel );
	if (ui->compatOptsBtn->isEnabled() && !compatOptsCmdArgs.isEmpty())
		cmd.arguments << compatOptsCmdArgs;
	if (ui->allowCheatsChkBox->isChecked())
		cmd.arguments << "+sv_cheats" << "1";

	if (ui->multiplayerChkBox->isChecked())
	{
		switch (ui->multRoleCmbBox->currentIndex())
		{
		 case MultRole::Server:
			cmd.arguments << "-host" << ui->playerCountSpinBox->text();
			if (ui->portSpinBox->value() != 5029)
				cmd.arguments << "-port" << ui->portSpinBox->text();
			switch (ui->gameModeCmbBox->currentIndex())
			{
			 case Deathmatch:
				cmd.arguments << "-deathmatch";
				break;
			 case TeamDeathmatch:
				cmd.arguments << "-deathmatch" << "+teamplay";
				break;
			 case AltDeathmatch:
				cmd.arguments << "-altdeath";
				break;
			 case AltTeamDeathmatch:
				cmd.arguments << "-altdeath" << "+teamplay";
				break;
			 case Cooperative: // default mode, which is started without any param
				break;
			 default:
				QMessageBox::critical( this, "Invalid game mode index",
					"The game mode index is out of range. This is a bug, please create an issue on Github page." );
			}
			if (ui->teamDmgSpinBox->value() != 0.0)
				cmd.arguments << "+teamdamage" << QString::number( ui->teamDmgSpinBox->value(), 'f', 2 );
			if (ui->timeLimitSpinBox->value() != 0)
				cmd.arguments << "-timer" << ui->timeLimitSpinBox->text();
			if (ui->fragLimitSpinBox->value() != 0)
				cmd.arguments << "+fraglimit" << ui->fragLimitSpinBox->text();
			cmd.arguments << "-netmode" << QString::number( ui->netModeCmbBox->currentIndex() );
			break;
		 case MultRole::Client:
			cmd.arguments << "-join" << ui->hostnameLine->text() % ":" % ui->portSpinBox->text();
			break;
		 default:
			QMessageBox::critical( this, "Invalid multiplayer role index",
				"The multiplayer role index is out of range. This is a bug, please create an issue on Github page." );
		}
	}

	if (!ui->saveDirLine->text().isEmpty())
		cmd.arguments << engineTraits.saveDirParam() << base.rebaseAndQuotePath( ui->saveDirLine->text() );
	if (!ui->screenshotDirLine->text().isEmpty())
		cmd.arguments << "+screenshot_dir" << base.rebaseAndQuotePath( ui->screenshotDirLine->text() );

	if (ui->monitorCmbBox->currentIndex() > 0)
	{
		int monitorIndex = ui->monitorCmbBox->currentIndex() - 1;  // the first item is a placeholder for leaving it default
		cmd.arguments << "+vid_adapter" << engineTraits.getCmdMonitorIndex( monitorIndex );  // some engines index monitors from 1 and others from 0
	}
	if (!ui->resolutionXLine->text().isEmpty())
		cmd.arguments << "-width" << ui->resolutionXLine->text();
	if (!ui->resolutionYLine->text().isEmpty())
		cmd.arguments << "-height" << ui->resolutionYLine->text();
	if (ui->showFpsChkBox->isChecked())
		cmd.arguments << "+vid_fps" << "1";

	if (ui->noSoundChkBox->isChecked())
		cmd.arguments << "-nosound";
	if (ui->noSfxChkBox->isChecked())
		cmd.arguments << "-nosfx";
	if (ui->noMusicChkBox->isChecked())
		cmd.arguments << "-nomusic";

	if (!ui->presetCmdArgsLine->text().isEmpty())
		cmd.arguments << ui->presetCmdArgsLine->text().split( ' ', Qt::SkipEmptyParts );

	if (!ui->globalCmdArgsLine->text().isEmpty())
		cmd.arguments << ui->globalCmdArgsLine->text().split( ' ', Qt::SkipEmptyParts );

	return cmd;
}

void MainWindow::launch()
{
	int selectedEngineIdx = ui->engineCmbBox->currentIndex();
	if (selectedEngineIdx < 0)
	{
		QMessageBox::warning( this, "No engine selected", "No Doom engine is selected." );
		return;
	}

	QString engineDir = getAbsoluteDirOfFile( engineModel[ selectedEngineIdx ].path );

	// re-run the command construction, but display error message and abort when there is invalid path
	ShellCommand cmd;
	try
	{
		// When sending arguments to the process directly and skipping the shell parsing, the quotes are undesired.
		cmd = generateLaunchCommand( engineDir, VerifyPaths, DontQuotePaths );
	}
	catch (const FileOrDirNotFound &)
	{
		return;  // errors are already shown during the generation
	}

	// make sure the alternative save dir exists, because engine will not create it if demo file path points there
	QString alternativeSaveDir = ui->saveDirLine->text();
	if (!alternativeSaveDir.isEmpty())
	{
		bool saveDirExists = createDirIfDoesntExist( alternativeSaveDir );
		if (!saveDirExists)
		{
			QMessageBox::warning( this, "Error creating directory", "Failed to create directory "%alternativeSaveDir );
			// we can continue without this directory, it will just not save demos
		}
	}

	// Before we execute the command, we need to switch the current dir to the engine's dir,
	// because some engines search for their own files in the current dir and would fail if started from elsewhere.
	// The command paths are always generated relative to the engine's dir.
	QString currentDir = QDir::currentPath();
	QDir::setCurrent( engineDir );
	// Because the current dir is now different, the launcher's paths no longer work,
	// so prevent reloading lists from directories because user would lose selection.
	engineRunning = true;

	// at the end of function restore the previous current dir
	auto guard = atScopeEndDo( [&]()
	{
		QDir::setCurrent( currentDir );
		engineRunning = false;
		updateListsFromDirs();
	});

	if (settings.showEngineOutput)
	{
		ProcessOutputWindow processWindow( this );
		processWindow.runProcess( cmd.executable, cmd.arguments );
		//int resultCode = processWindow.result();
	}
	else
	{
		bool success = QProcess::startDetached( cmd.executable, cmd.arguments );
		if (!success)
		{
			QMessageBox::warning( this, "Launch error", "Failed to execute launch command." );
			return;
		}

		if (settings.closeOnLaunch)
		{
			QApplication::quit();
		}
	}
}
