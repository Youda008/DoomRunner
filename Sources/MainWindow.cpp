//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of the main window, including all its tabs
//======================================================================================================================

#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include "Dialogs/AboutDialog.hpp"
#include "Dialogs/SetupDialog.hpp"
#include "Dialogs/OptionsStorageDialog.hpp"
#include "Dialogs/NewConfigDialog.hpp"
#include "Dialogs/OwnFileDialog.hpp"
#include "Dialogs/GameOptsDialog.hpp"
#include "Dialogs/CompatOptsDialog.hpp"
#include "Dialogs/ProcessOutputWindow.hpp"

#include "OptionsSerializer.hpp"
#include "Version.hpp"  // window title
#include "UpdateChecker.hpp"
#include "Themes.hpp"
#include "EngineTraits.hpp"
#include "DoomFiles.hpp"

#include "Utils/LangUtils.hpp"
#include "Utils/ContainerUtils.hpp"
#include "Utils/FileSystemUtils.hpp"
#include "Utils/OSUtils.hpp"
#include "Utils/ExeReader.hpp"
#include "Utils/WADReader.hpp"
#include "Utils/WidgetUtils.hpp"
#include "Utils/MiscUtils.hpp"  // checkPath, highlightPathIfInvalid
#include "Utils/ErrorHandling.hpp"

// showMapPackDesc
#include <QVBoxLayout>
#include <QPlainTextEdit>
#include <QFontDatabase>

#include <QStringBuilder>
#include <QTextStream>  // exportPresetToScript, loadMonitorInfo
#include <QFile>
#include <QDir>
#include <QFileIconProvider>  // EmptyIconProvider
#include <QMessageBox>
#include <QShortcut>
#include <QTimer>
#include <QProcess>  // startDetached


//======================================================================================================================

static const char defaultOptionsFileName [] = "options.json";
static const char defaultCacheFileName [] = "file_info_cache.json";

#if IS_WINDOWS
	static const QString scriptFileSuffix = "*.bat";
	static const QString shortcutFileSuffix = "*.lnk";
#else
	static const QString scriptFileSuffix = "*.sh";
#endif

static constexpr bool VerifyPaths = true;
static constexpr bool DontVerifyPaths = false;

static constexpr int VarNameColumn = 0;
static constexpr int VarValueColumn = 1;


//======================================================================================================================
//  MainWindow-specific utils

template< typename Func >
void MainWindow::addShortcut( const QKeySequence & keys, const Func & shortcutAction )
{
	QShortcut * shortcut = new QShortcut( keys, this );
	connect( shortcut, &QShortcut::activated, this, shortcutAction );
}

Preset * MainWindow::getSelectedPreset() const
{
	int selectedPresetIdx = wdg::getSelectedItemIndex( ui->presetListView );
	return selectedPresetIdx >= 0 ? unconst( &presetModel[ selectedPresetIdx ] ) : nullptr;
}

EngineInfo * MainWindow::getSelectedEngine() const
{
	int selectedEngineIdx = ui->engineCmbBox->currentIndex();
	return selectedEngineIdx >= 0 ? unconst( &engineModel[ selectedEngineIdx ] ) : nullptr;
}

MainWindow::ConfigFile * MainWindow::getSelectedConfig() const
{
	int selectedConfigIdx = ui->configCmbBox->currentIndex();
	// at index 0 there is an empty placeholder to allow deselecting config
	return selectedConfigIdx > 0 ? unconst( &configModel[ selectedConfigIdx ] ) : nullptr;
}

IWAD * MainWindow::getSelectedIWAD() const
{
	int selectedIwadIdx = wdg::getSelectedItemIndex( ui->iwadListView );
	return selectedIwadIdx >= 0 ? unconst( &iwadModel[ selectedIwadIdx ] ) : nullptr;
}

template< typename Functor >
void MainWindow::forEachSelectedMapPack( const Functor & loopBody ) const
{
	// clicking on an item in QTreeView with QFileSystemModel selects all elements (columns) of a row,
	// but we only care about the first one
	const auto selectedRows = wdg::getSelectedRows( ui->mapDirView );

	// extract the file paths
	for (const QModelIndex & index : selectedRows)
	{
		QString modelPath = mapModel.filePath( index );
		loopBody( pathConvertor.convertPath( modelPath ) );
	}
}

QStringVec MainWindow::getSelectedMapPacks() const
{
	QStringVec selectedMapPacks;

	forEachSelectedMapPack( [&]( const QString & mapPackPath )
	{
		selectedMapPacks.append( mapPackPath );
	});

	return selectedMapPacks;
}

QStringList MainWindow::getUniqueMapNamesFromWADs( const QVector<QString> & selectedWADs ) const
{
	QMap< QString, int > uniqueMapNames;  // we cannot use QSet because that one is unordered and we need to retain order
	for (const QString & selectedWAD : selectedWADs)
	{
		if (!fs::isValidFile( selectedWAD ))
			continue;

		const doom::UncertainWadInfo & wadInfo = doom::g_cachedWadInfo.getFileInfo( selectedWAD );
		if (wadInfo.status != ReadStatus::Success)
			continue;

		for (const QString & mapName : wadInfo.mapNames)
			uniqueMapNames.insert( mapName.toUpper(), 0 );  // the 0 doesn't matter
	}
	return uniqueMapNames.keys();
}

QString MainWindow::getConfigDir() const
{
	int currentEngineIdx = ui->engineCmbBox->currentIndex();
	return currentEngineIdx >= 0 ? engineModel[ currentEngineIdx ].configDir : QString();
}

QString MainWindow::getDataDir() const
{
	int currentEngineIdx = ui->engineCmbBox->currentIndex();
	return currentEngineIdx >= 0 ? engineModel[ currentEngineIdx ].dataDir : QString();
}

QString MainWindow::getSaveDir() const
{
	// the path in saveDirLine is relative to the engine's data dir by convention, need to rebase it to the current working dir
	QString saveDirLine = ui->saveDirLine->text();
	if (!saveDirLine.isEmpty())                                     // if custom save dir is specified
		return engineDataDirRebaser.rebasePathBack( saveDirLine );  // then use it
	else                                                            // otherwise
		return getDataDir();                                        // use engine's data dir
}

QString MainWindow::getScreenshotDir() const
{
	// the path in screenshotDirLine is relative to the engine's data dir by convention, need to rebase it to the current working dir
	QString screenshotDirLine = ui->screenshotDirLine->text();
	if (!screenshotDirLine.isEmpty())                                     // if custom save dir is specified
		return engineDataDirRebaser.rebasePathBack( screenshotDirLine );  // then use it
	else                                                                  // otherwise
		return getDataDir();                                              // use engine's data dir
}

QString MainWindow::getDemoDir() const
{
	// let's not complicate things and treat save dir and demo dir as one
	return getSaveDir();
}

// converts a path relative to the engine's data dir to absolute path or vice versa
QString MainWindow::convertRebasedEngineDataPath( QString rebasedPath ) const
{
	// These branches are here only as optimization, we could easily just: rebase-back -> convert -> rebase.
	PathStyle inputStyle = fs::getPathStyle( rebasedPath );
	PathStyle launcherStyle = pathConvertor.pathStyle();
	if (inputStyle == PathStyle::Relative && launcherStyle == PathStyle::Absolute)
	{
		QString trueRelativePath = engineDataDirRebaser.rebasePathBack( rebasedPath );
		return pathConvertor.getAbsolutePath( trueRelativePath );
	}
	else if (inputStyle == PathStyle::Absolute && launcherStyle == PathStyle::Relative)
	{
		QString trueRelativePath = pathConvertor.getRelativePath( rebasedPath );
		return engineDataDirRebaser.rebasePath( trueRelativePath );
	}
	else
	{
		// nothing to be done, path is already in the right form
		return rebasedPath;
	}
}

QString MainWindow::rebaseSaveFilePath( const QString & filePath, const PathRebaser & workingDirRebaser, const EngineInfo * engine )
{
	// the base dir for the save file parameter depends on the engine and its version
	if (engine && engine->baseDirStyleForSaveFiles() == EngineTraits::SaveBaseDir::SaveDir)
	{
		QString saveDir = getSaveDir();
		PathRebaser saveDirRebaser( pathConvertor.workingDir(), saveDir, PathStyle::Relative, workingDirRebaser.quotePaths() );
		return saveDirRebaser.rebaseAndQuotePath( filePath );
	}
	else
	{
		return workingDirRebaser.rebaseAndQuotePath( filePath );
	}
};

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

// Iterates over all directories which the engine will need to access (either for reading or writing).
// Required for supporting sandbox environments like Snap or Flatpak
template< typename Functor >
void MainWindow::forEachDirToBeAccessed( const Functor & loopBody ) const
{
	const EngineInfo * selectedEngine = getSelectedEngine();
	if (!selectedEngine)
	{
		return;
	}

	// dir of config files
	if (const ConfigFile * selectedConfig = getSelectedConfig())
	{
		loopBody( selectedEngine->configDir );  // cannot be empty otherwise config would not be selected
	}

	// dir of IWAD
	if (const IWAD * selectedIWAD = getSelectedIWAD())
	{
		loopBody( fs::getDirOfFile( selectedIWAD->path ) );
	}

	// dir of map files
	if (wdg::isSomethingSelected( ui->mapDirView ))
	{
		loopBody( mapSettings.dir );  // all map files will always be inside the configured map dir
	}

	// dirs of mod files
	QDir modDir( modSettings.dir );
	bool modDirUsed = false;
	for (const Mod & mod : modModel)
	{
		if (!mod.checked)
			continue;

		if (fs::isInsideDir( mod.path, modDir ))  // aggregate all mods inside the configured mod dir under single dir path
		{
			if (!modDirUsed)  // use it only once
			{
				loopBody( modSettings.dir );
				modDirUsed = true;
			}
		}
		else  // but still add directories outside of the configured mod dir, because mod dir is only a hint
		{
			loopBody( fs::getDirOfFile( mod.path ) );
		}
	}

	// dir of saves and demo files
	LaunchMode launchMode = getLaunchModeFromUI();
	if ((launchMode == LoadSave && !ui->saveFileCmbBox->currentText().isEmpty())
	 || (launchMode == ReplayDemo && !ui->demoFileCmbBox_replay->currentText().isEmpty()))
	{
		loopBody( getSaveDir() );
	}

	// dir of screenshots
	if (!ui->screenshotDirLine->text().isEmpty())
	{
		loopBody( ui->screenshotDirLine->text() );
	}
}

// Gets (deduplicated) directories which the engine will need to access (either for reading or writing).
QStringVec MainWindow::getDirsToBeAccessed() const
{
	QSet< QString > dirSet;  // de-duplicate the paths

	forEachDirToBeAccessed( [&]( const QString & dir )
	{
		dirSet.insert( dir );
	});

	return QStringVec( dirSet.begin(), dirSet.end() );
}

// This needs to be called everytime the user make a change that needs to be saved into the options file.
void MainWindow::scheduleSavingOptions( bool storedOptionsModified )
{
	// update the options file at the nearest file-saving cycle
	optionsNeedUpdate = optionsNeedUpdate || storedOptionsModified;
}

LaunchOptions & MainWindow::activeLaunchOptions()
{
	if (settings.launchOptsStorage == StoreToPreset)
	{
		if (Preset * selectedPreset = getSelectedPreset())
		{
			return selectedPreset->launchOpts;
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
		if (Preset * selectedPreset = getSelectedPreset())
		{
			return selectedPreset->multOpts;
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
		if (Preset * selectedPreset = getSelectedPreset())
		{
			return selectedPreset->gameOpts;
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
		if (Preset * selectedPreset = getSelectedPreset())
		{
			return selectedPreset->compatOpts;
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
		if (Preset * selectedPreset = getSelectedPreset())
		{
			return selectedPreset->videoOpts;
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
		if (Preset * selectedPreset = getSelectedPreset())
		{
			return selectedPreset->audioOpts;
		}
	}

	// We always have to save somewhere, because generateLaunchCommand() might read stored values from it.
	// In case the optsStorage == DontStore, we will just skip serializing it to JSON.
	return audioOpts;
}

/// Stores value to a preset if it's selected.
#define STORE_TO_CURRENT_PRESET( presetMember, value ) [&]()\
{\
	bool valueChanged = false; \
	if (Preset * selectedPreset = getSelectedPreset()) \
	{\
		valueChanged = selectedPreset->presetMember != (value); \
		selectedPreset->presetMember = (value); \
	}\
	return valueChanged; \
}()

/// Stores value to a preset if it's selected and if it's safe to do.
#define STORE_TO_CURRENT_PRESET_IF_SAFE( presetMember, value ) [&]()\
{\
	bool valueChanged = false; \
	/* Only store the value to the preset if we are not restoring from it, */ \
	/* otherwise we would overwrite a stored value we want to restore later. */ \
	if (!restoringPresetInProgress) \
	{\
		if (Preset * selectedPreset = getSelectedPreset()) \
		{\
			valueChanged = selectedPreset->presetMember != (value); \
			selectedPreset->presetMember = (value); \
		}\
	}\
	return valueChanged; \
}()

/// Stores value to a global storage if it's safe to do.
#define STORE_TO_GLOBAL_STORAGE_IF_SAFE( globalMember, value ) [&]()\
{\
	bool valueChanged = false; \
	/* Only store the value to the global options if we are not restoring from it, */ \
	/* otherwise we would overwrite a stored value we want to restore later. */ \
	if (!restoringOptionsInProgress) \
	{\
		valueChanged = (globalMember != (value)); \
		globalMember = (value); \
	}\
	return valueChanged; \
}()

/// Stores value to a dynamically selected storage if it's possible and if it's safe to do.
#define STORE_TO_DYNAMIC_STORAGE_IF_SAFE( storageSetting, activeStorage, structMember, value ) [&]()\
{\
	bool valueChanged = false; \
	/* Only store the value to the global options or preset if we are not restoring from the same location, */ \
	/* otherwise we would overwrite a stored value we want to restore later. */ \
	bool preventSaving = (storageSetting == StoreGlobally && restoringOptionsInProgress) \
	                  || (storageSetting == StoreToPreset && restoringPresetInProgress); \
	if (!preventSaving) \
	{\
		valueChanged = (activeStorage.structMember != (value)); \
		activeStorage.structMember = (value); \
	}\
	return valueChanged; \
}()

#define STORE_LAUNCH_OPTION( structMember, value ) \
	STORE_TO_DYNAMIC_STORAGE_IF_SAFE( settings.launchOptsStorage, activeLaunchOptions(), structMember, value )

#define STORE_MULT_OPTION( structMember, value ) \
	STORE_TO_DYNAMIC_STORAGE_IF_SAFE( settings.launchOptsStorage, activeMultiplayerOptions(), structMember, value )

#define STORE_GAMEPLAY_OPTION( structMember, value ) \
	STORE_TO_DYNAMIC_STORAGE_IF_SAFE( settings.gameOptsStorage, activeGameplayOptions(), structMember, value )

#define STORE_COMPAT_OPTION( structMember, value ) \
	STORE_TO_DYNAMIC_STORAGE_IF_SAFE( settings.compatOptsStorage, activeCompatOptions(), structMember, value )

#define STORE_ALT_PATH( structMember, value ) \
	STORE_TO_CURRENT_PRESET_IF_SAFE( altPaths.structMember, value )

#define STORE_VIDEO_OPTION( structMember, value ) \
	STORE_TO_DYNAMIC_STORAGE_IF_SAFE( settings.videoOptsStorage, activeVideoOptions(), structMember, value )

#define STORE_AUDIO_OPTION( structMember, value ) \
	STORE_TO_DYNAMIC_STORAGE_IF_SAFE( settings.audioOptsStorage, activeAudioOptions(), structMember, value )

#define STORE_PRESET_OPTION( structMember, value ) \
	STORE_TO_CURRENT_PRESET_IF_SAFE( structMember, value )

#define STORE_GLOBAL_OPTION( structMember, value ) \
	STORE_TO_GLOBAL_STORAGE_IF_SAFE( globalOpts.structMember, value )


//======================================================================================================================
//  MainWindow

MainWindow::MainWindow()
:
	QMainWindow( nullptr ),
	DialogWithPaths(
		// All relative paths will internally be stored relative to the current working dir,
		// so that all file-system operations instantly work without the need to rebase the paths first.
		this, PathConvertor( QDir::current(), defaultPathStyle )
	),
	engineDataDirRebaser( {}, {}, defaultPathStyle ),  // rebase to current working dir (don't rebase at all) until engine is selected
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

	// make sure the correct widgets are enabled/disabled according to the default storage settings, until the settings are loaded
	togglePresetSubWidgets( false );

	// OS-specific initialization

 #if !IS_WINDOWS
	// On Windows the application icon is already set using the Windows resource system, so loading this resource
	// is unnecessary. But on Linux, we have to do it. See https://doc.qt.io/qt-6/appicon.html
	this->setWindowIcon( QIcon(":/DoomRunner.ico") );
 #endif

	// setup main menu actions

	connect( ui->initialSetupAction, &QAction::triggered, this, &thisClass::runSetupDialog );
	connect( ui->optionsStorageAction, &QAction::triggered, this, &thisClass::runOptsStorageDialog );
	connect( ui->exportPresetToScriptAction, &QAction::triggered, this, &thisClass::exportPresetToScript );
	connect( ui->exportPresetToShortcutAction, &QAction::triggered, this, &thisClass::exportPresetToShortcut );
	//connect( ui->importPresetAction, &QAction::triggered, this, &thisClass::importPreset );
	connect( ui->aboutAction, &QAction::triggered, this, &thisClass::runAboutDialog );
	connect( ui->exitAction, &QAction::triggered, this, &thisClass::close );

 #if !IS_WINDOWS  // Windows-only feature
	ui->exportPresetToShortcutAction->setEnabled( false );
 #endif

	// setup key shortcuts for switching focus

	addShortcut( { Qt::CTRL + Qt::Key_1 }, [ this ](){ ui->presetListView->setFocus(); } );
	addShortcut( { Qt::CTRL + Qt::Key_2 }, [ this ](){ ui->engineCmbBox->setFocus(); } );
	addShortcut( { Qt::CTRL + Qt::Key_3 }, [ this ](){ ui->iwadListView->setFocus(); } );     // shut-up clang, this is the official way to do it by Qt doc
	addShortcut( { Qt::CTRL + Qt::Key_4 }, [ this ](){ ui->mapDirView->setFocus(); } );
	addShortcut( { Qt::CTRL + Qt::Key_5 }, [ this ](){ ui->configCmbBox->setFocus(); } );
	addShortcut( { Qt::CTRL + Qt::Key_6 }, [ this ](){ ui->modListView->setFocus(); } );

	// setup main list views

	setupPresetList();
	setupIWADList();
	setupMapPackList();
	setupModList();
	setupEnvVarLists();

	// setup combo-boxes

	// we use custom model for engines, because we want to display the same list differently in different window
	ui->engineCmbBox->setModel( &engineModel );
	connect( ui->engineCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::onEngineSelected );
	connect( ui->engineDirBtn, &QToolButton::clicked, this, &thisClass::openEngineDataDir );

	configModel.append({""});  // always have an empty item, so that index doesn't have to switch between -1 and 0
	ui->configCmbBox->setModel( &configModel );
	connect( ui->configCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::onConfigSelected );
	connect( ui->configCloneBtn, &QToolButton::clicked, this, &thisClass::cloneConfig );

	ui->saveFileCmbBox->setModel( &saveModel );
	connect( ui->saveFileCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::onSavedGameSelected );

	ui->demoFileCmbBox_replay->setModel( &demoModel );
	connect( ui->demoFileCmbBox_replay, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::onDemoFileSelected_replay );

	connect( ui->mapCmbBox, &QComboBox::currentTextChanged, this, &thisClass::onMapChanged );
	connect( ui->mapCmbBox_demo, &QComboBox::currentTextChanged, this, &thisClass::onMapChanged_demo );

	connect( ui->demoFileLine_record, &QLineEdit::textChanged, this, &thisClass::onDemoFileChanged_record );

	ui->compatLevelCmbBox->addItem("");  // always have this empty item there, so that we can restore index 0

	// setup launch options callbacks

	// launch mode
	connect( ui->launchMode_default, &QRadioButton::clicked, this, &thisClass::onModeChosen_Default );
	connect( ui->launchMode_map, &QRadioButton::clicked, this, &thisClass::onModeChosen_LaunchMap );
	connect( ui->launchMode_savefile, &QRadioButton::clicked, this, &thisClass::onModeChosen_SavedGame );
	connect( ui->launchMode_recordDemo, &QRadioButton::clicked, this, &thisClass::onModeChosen_RecordDemo );
	connect( ui->launchMode_replayDemo, &QRadioButton::clicked, this, &thisClass::onModeChosen_ReplayDemo );

	// mutiplayer
	connect( ui->multiplayerGrpBox, &QGroupBox::toggled, this, &thisClass::onMultiplayerToggled );
	connect( ui->multRoleCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::onMultRoleSelected );
	connect( ui->hostnameLine, &QLineEdit::textChanged, this, &thisClass::onHostChanged );
	connect( ui->portSpinBox, QOverload<int>::of( &QSpinBox::valueChanged ), this, &thisClass::onPortChanged );
	connect( ui->netModeCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::onNetModeSelected );
	connect( ui->gameModeCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::onGameModeSelected );
	connect( ui->playerCountSpinBox, QOverload<int>::of( &QSpinBox::valueChanged ), this, &thisClass::onPlayerCountChanged );
	connect( ui->teamDmgSpinBox, QOverload<double>::of( &QDoubleSpinBox::valueChanged ), this, &thisClass::onTeamDamageChanged );
	connect( ui->timeLimitSpinBox, QOverload<int>::of( &QSpinBox::valueChanged ), this, &thisClass::onTimeLimitChanged );
	connect( ui->fragLimitSpinBox, QOverload<int>::of( &QSpinBox::valueChanged ), this, &thisClass::onFragLimitChanged );

	// gameplay
	connect( ui->skillCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::onSkillSelected );
	connect( ui->skillSpinBox, QOverload<int>::of( &QSpinBox::valueChanged ), this, &thisClass::onSkillNumChanged );
	connect( ui->noMonstersChkBox, &QCheckBox::toggled, this, &thisClass::onNoMonstersToggled );
	connect( ui->fastMonstersChkBox, &QCheckBox::toggled, this, &thisClass::onFastMonstersToggled );
	connect( ui->monstersRespawnChkBox, &QCheckBox::toggled, this, &thisClass::onMonstersRespawnToggled );
	connect( ui->gameOptsBtn, &QPushButton::clicked, this, &thisClass::runGameOptsDialog );
	connect( ui->compatOptsBtn, &QPushButton::clicked, this, &thisClass::runCompatOptsDialog );
	connect( ui->compatLevelCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::onCompatLevelSelected );
	connect( ui->allowCheatsChkBox, &QCheckBox::toggled, this, &thisClass::onAllowCheatsToggled );

	// alternative paths
	connect( ui->usePresetNameChkBox, &QCheckBox::toggled, this, &thisClass::onUsePresetNameToggled );
	connect( ui->saveDirLine, &QLineEdit::textChanged, this, &thisClass::onSaveDirChanged );
	connect( ui->screenshotDirLine, &QLineEdit::textChanged, this, &thisClass::onScreenshotDirChanged );
	connect( ui->saveDirBtn, &QPushButton::clicked, this, &thisClass::browseSaveDir );
	connect( ui->screenshotDirBtn, &QPushButton::clicked, this, &thisClass::browseScreenshotDir );

	// video
	loadMonitorInfo( ui->monitorCmbBox );
	connect( ui->monitorCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::onMonitorSelected );
	connect( ui->resolutionXLine, &QLineEdit::textChanged, this, &thisClass::onResolutionXChanged );
	connect( ui->resolutionYLine, &QLineEdit::textChanged, this, &thisClass::onResolutionYChanged );
	connect( ui->showFpsChkBox, &QCheckBox::toggled, this, &thisClass::onShowFpsToggled );

	// audio
	connect( ui->noSoundChkBox, &QCheckBox::toggled, this, &thisClass::onNoSoundToggled );
	connect( ui->noSfxChkBox, &QCheckBox::toggled, this, &thisClass::onNoSFXToggled);
	connect( ui->noMusicChkBox, &QCheckBox::toggled, this, &thisClass::onNoMusicToggled );

	// setup the rest of widgets

	connect( ui->presetCmdArgsLine, &QLineEdit::textChanged, this, &thisClass::onPresetCmdArgsChanged );
	connect( ui->globalCmdArgsLine, &QLineEdit::textChanged, this, &thisClass::onGlobalCmdArgsChanged );
	connect( ui->launchBtn, &QPushButton::clicked, this, &thisClass::launch );
}

void MainWindow::adjustUi()
{
	// align the tool buttons with the combo-boxes
	auto engineCmbBoxHeight = ui->engineCmbBox->height();
	ui->engineDirBtn->setMinimumSize({ engineCmbBoxHeight, engineCmbBoxHeight });
	auto configCmbBoxHeight = ui->configCmbBox->height();
	ui->configCloneBtn->setMinimumSize({ configCmbBoxHeight, configCmbBoxHeight });

	// hide it by default, it's shown on startup
	presetSearchPanel->collapse();
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
	ui->presetListView->toggleListModifications( true );
	connect( &presetModel, &QAbstractListModel::dataChanged, this, &thisClass::onPresetDataChanged );

	// set drag&drop behaviour
	ui->presetListView->toggleIntraWidgetDragAndDrop( true );
	ui->presetListView->toggleInterWidgetDragAndDrop( false );
	ui->presetListView->toggleExternalFileDragAndDrop( false );
	connect( ui->presetListView, QOverload< int, int >::of( &EditableListView::itemsDropped ), this, &thisClass::onPresetsReordered );

	// set reaction when an item is selected
	connect( ui->presetListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &thisClass::onPresetToggled );

	// setup reaction to key shortcuts and right click
	ui->presetListView->toggleContextMenu( true );
	ui->presetListView->enableItemCloning();
	ui->presetListView->enableInsertSeparator();
	connect( ui->presetListView->addItemAction, &QAction::triggered, this, &thisClass::presetAdd );
	connect( ui->presetListView->deleteItemAction, &QAction::triggered, this, &thisClass::presetDelete );
	connect( ui->presetListView->cloneItemAction, &QAction::triggered, this, &thisClass::presetClone );
	connect( ui->presetListView->moveItemUpAction, &QAction::triggered, this, &thisClass::presetMoveUp );
	connect( ui->presetListView->moveItemDownAction, &QAction::triggered, this, &thisClass::presetMoveDown );
	connect( ui->presetListView->insertSeparatorAction, &QAction::triggered, this, &thisClass::presetInsertSeparator );

	// setup buttons
	connect( ui->presetBtnAdd, &QToolButton::clicked, this, &thisClass::presetAdd );
	connect( ui->presetBtnDel, &QToolButton::clicked, this, &thisClass::presetDelete );
	connect( ui->presetBtnClone, &QToolButton::clicked, this, &thisClass::presetClone );
	connect( ui->presetBtnUp, &QToolButton::clicked, this, &thisClass::presetMoveUp );
	connect( ui->presetBtnDown, &QToolButton::clicked, this, &thisClass::presetMoveDown );

	// setup search
	presetSearchPanel = new SearchPanel( ui->searchShowBtn, ui->searchLine, ui->caseSensitiveChkBox, ui->regexChkBox );
	ui->presetListView->enableFinding();
	connect( ui->presetListView->findItemAction, &QAction::triggered, presetSearchPanel, &SearchPanel::expand );
	connect( presetSearchPanel, &SearchPanel::searchParamsChanged, this, &thisClass::searchPresets );
}

void MainWindow::setupIWADList()
{
	// connect the view with model
	ui->iwadListView->setModel( &iwadModel );

	// set selection rules
	ui->iwadListView->setSelectionMode( QAbstractItemView::SingleSelection );

	// set reaction when an item is clicked or double-clicked
	connect( ui->iwadListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &thisClass::onIWADToggled );
	connect( ui->iwadListView, &QListView::doubleClicked, this, &thisClass::showIWADDesc );
}

void MainWindow::setupMapPackList()
{
	// connect the view with model
	ui->mapDirView->setModel( &mapModel );

	// set selection rules
	ui->mapDirView->setSelectionMode( QAbstractItemView::ExtendedSelection );

	// set item filters
	mapModel.setFilter( QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks );
	mapModel.setNameFilters( doom::getModFileSuffixes() );
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

	// set reaction when an item is clicked or double-clicked
	connect( ui->mapDirView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &thisClass::onMapPackToggled );
	connect( ui->mapDirView, &QTreeView::doubleClicked, this, &thisClass::showMapPackDesc );

	// QFileSystemModel updates its content asynchronously in a separate thread. For this reason,
	// when the model is set to display a certain directory, we cannot select items from the view right away,
	// but must wait until the list is populated.
	connect( &mapModel, &QFileSystemModel::directoryLoaded, this, &thisClass::onMapDirUpdated );
}

void MainWindow::setupModList()
{
	// connect the view with model
	ui->modListView->setModel( &modModel );

	// set selection rules
	ui->modListView->setSelectionMode( QAbstractItemView::ExtendedSelection );

	// setup item checkboxes
	modModel.toggleCheckableItems( true );

	// setup editing and separators
	modModel.toggleEditing( true );
	ui->modListView->toggleNameEditing( true );
	ui->modListView->toggleListModifications( true );

	// give the model our path convertor, it will need it for converting paths dropped from directory
	modModel.setPathContext( &pathConvertor );

	// set drag&drop behaviour
	ui->modListView->toggleIntraWidgetDragAndDrop( true );
	ui->modListView->toggleInterWidgetDragAndDrop( true );
	ui->modListView->toggleExternalFileDragAndDrop( true );
	connect( ui->modListView, QOverload< int, int >::of( &EditableListView::itemsDropped ), this, &thisClass::onModsDropped );

	// set reaction when an item is checked or unchecked
	connect( &modModel, &QAbstractListModel::dataChanged, this, &thisClass::onModDataChanged );
	connect( ui->modListView, &QListView::doubleClicked, this, &thisClass::showModDesc );

	// setup reaction to key shortcuts and right click
	ui->modListView->toggleContextMenu( true );
	addCmdArgAction = ui->modListView->addAction( "Add command line argument", { Qt::CTRL + Qt::Key_Asterisk } );
	ui->modListView->enableInsertSeparator();
	ui->modListView->enableOpenFileLocation();
	connect( ui->modListView->addItemAction, &QAction::triggered, this, &thisClass::modAdd );
	connect( addCmdArgAction, &QAction::triggered, this, &thisClass::modAddArg );
	connect( ui->modListView->deleteItemAction, &QAction::triggered, this, &thisClass::modDelete );
	connect( ui->modListView->moveItemUpAction, &QAction::triggered, this, &thisClass::modMoveUp );
	connect( ui->modListView->moveItemDownAction, &QAction::triggered, this, &thisClass::modMoveDown );
	connect( ui->modListView->insertSeparatorAction, &QAction::triggered, this, &thisClass::modInsertSeparator );

	// setup buttons
	connect( ui->modBtnAdd, &QToolButton::clicked, this, &thisClass::modAdd );
	connect( ui->modBtnAddDir, &QToolButton::clicked, this, &thisClass::modAddDir );
	connect( ui->modBtnAddArg, &QToolButton::clicked, this, &thisClass::modAddArg );
	connect( ui->modBtnDel, &QToolButton::clicked, this, &thisClass::modDelete );
	connect( ui->modBtnUp, &QToolButton::clicked, this, &thisClass::modMoveUp );
	connect( ui->modBtnDown, &QToolButton::clicked, this, &thisClass::modMoveDown );

	// setup icons
	ui->modListView->enableTogglingIcons();  // allow the icons to be toggled via context-menu
	ui->modListView->toggleIcons( true );  // we need to do this instead of modModel.toggleIcons() in order to update the action text
	connect( ui->modListView->toggleIconsAction, &QAction::triggered, this, &thisClass::modToggleIcons );
}

void MainWindow::setupEnvVarLists()
{
	// setup buttons
	connect( ui->presetEnvVarBtnAdd, &QToolButton::clicked, this, &thisClass::presetEnvVarAdd );
	connect( ui->presetEnvVarBtnDel, &QToolButton::clicked, this, &thisClass::presetEnvVarDelete );
	connect( ui->globalEnvVarBtnAdd, &QToolButton::clicked, this, &thisClass::globalEnvVarAdd );
	connect( ui->globalEnvVarBtnDel, &QToolButton::clicked, this, &thisClass::globalEnvVarDelete );

	// setup edit callbacks
	connect( ui->presetEnvVarTable, &QTableWidget::cellChanged, this, &thisClass::onPresetEnvVarDataChanged );
	connect( ui->globalEnvVarTable, &QTableWidget::cellChanged, this, &thisClass::onGlobalEnvVarDataChanged );
}

void MainWindow::loadMonitorInfo( QComboBox * box )
{
	const auto monitors = os::listMonitors();
	for (const os::MonitorInfo & monitor : monitors)
	{
		QString monitorDescription;
		QTextStream descStream( &monitorDescription, QIODevice::WriteOnly );

		descStream << monitor.name << " - " << monitor.width << 'x' << monitor.height;
		if (monitor.isPrimary)
			descStream << " (primary)";

		descStream.flush();
		box->addItem( std::move(monitorDescription) );
	}
}

void MainWindow::updateOptionsGrpBoxTitles( const StorageSettings & storageSettings )
{
	static const char * const optsStorageStrings [] =
	{
		"not stored",
		"stored globally",
		"stored in preset"
	};

	auto updateGroupBoxTitle = []( QGroupBox * grpBox, OptionsStorage storage )
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

// Backward compatibility: Older versions stored options file in config dir.
// We need to look if the options file is in the old directory and if it is, move it to the new one.
void MainWindow::moveOptionsFromOldDir( QDir oldOptionsDir, QDir newOptionsDir, QString optionsFileName )
{
	if (newOptionsDir.exists( optionsFileName ))
	{
		return;  // always prefer the new one, if it already exists
	}

	if (oldOptionsDir.exists( optionsFileName ))
	{
		QString newOptionsFilePath = newOptionsDir.filePath( optionsFileName );

		reportInformation( nullptr, "Migrating options file",
			"DoomRunner changed the location of "%optionsFileName%", where it stores your presets and other options. "
			%optionsFileName%" has been found in the old data directory \""%oldOptionsDir.path()%"\""
			" and will be automatically moved to the new data directory \""%newOptionsDir.path()%"\""
		);
		logInfo() <<
			"NOTICE: Found "%optionsFileName%" in the old data directory \""%oldOptionsDir.path()%"\". "
			"Moving it to the new data directory \""%newOptionsDir.path()%"\"";

		oldOptionsDir.rename( optionsFileName, newOptionsFilePath );
	}
}

void MainWindow::initAppDataDir()
{
	// create a directory for application data, if it doesn't exist already
	appDataDir.setPath( os::getThisAppDataDir() );
	if (!appDataDir.exists())
	{
		appDataDir.mkpath(".");
	}

	// backward compatibility
	moveOptionsFromOldDir( os::getThisAppConfigDir(), appDataDir, defaultOptionsFileName );

	cacheFilePath = appDataDir.filePath( defaultCacheFileName );
	optionsFilePath = appDataDir.filePath( defaultOptionsFileName );
}

// This is called when the window layout is initialized and widget sizes calculated,
// but before the window is physically shown (drawn for the first time).
void MainWindow::showEvent( QShowEvent * event )
{
	initAppDataDir();

	// Options loading is now split into two phases.
	// The reason is that appearance settings and geometry need to be applied before the window is drawn
	// for the first time (so that the window does not appear white and then changes to dark or is moved/resized),
	// while the rest of the settings need to be loaded after the appearance settings are already visibly applied
	// (the window is already drawn with the loaded settings) so that the possible error message boxes are drawn
	// using the already changed appearance and over already changed main window.
	// That's why here, before the window appears for the first time, we read and parse the JSON file, extract
	// the appearance settings and geometry and apply it.
	// Then we let the window show with the modified settings, and only then load and apply the rest of the options,
	// and possibly open the initial setup dialog.

	if (fs::isValidFile( optionsFilePath ))
	{
		// read and parse the file
		parsedOptionsDoc = readOptions( optionsFilePath );

		// phase 1 - load app appearance and window geometry
		if (parsedOptionsDoc && parsedOptionsDoc->isValid())
		{
			loadAppearance( *parsedOptionsDoc, /*loadGeometry*/ true );
		}
	}

	// This must be called after the appearance settings are already loaded and applied,
	// because they might change application style, and that might change widget sizes.
	// Calling it in the onWindowShown() on the other hand, causes the window to briefly appear with the original layout
	// and then switch to the adjusted layout in the next frame.
	adjustUi();

	superClass::showEvent( event );

	// This will be called after the window is fully initialized and physically shown (drawn for the first time).
	QMetaObject::invokeMethod( this, &thisClass::onWindowShown, Qt::ConnectionType::QueuedConnection );
}

// This is called after the window is fully initialized and physically shown (drawn for the first time).
void MainWindow::onWindowShown()
{
	// The potentially most expensive file-system operations like reading exe files are done here,
	// just to make sure the application doesn't hang before even showing anything.
	// It would be best to do this asynchronously, but there has not been any reported problems with is,
	// so it's probably fine.

	// cache needs to be loaded first, because loadOptions() already needs it
	if (fs::isValidFile( cacheFilePath ))
	{
		loadCache( cacheFilePath );
	}

	auto optionsDocDeleter = atScopeEndDo( [ this ](){ parsedOptionsDoc.reset(); } );  // delete when no longer needed

	if (fs::isValidFile( optionsFilePath ))
	{
		// phase 2 - load the rest of options
		if (parsedOptionsDoc && parsedOptionsDoc->isValid())
		{
			loadTheRestOfOptions( *parsedOptionsDoc );
		}
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
		wdg::appendItem( ui->presetListView, presetModel, { "Default" } );

		// automatically select those essential items (engine, IWAD, ...) that are alone in their list
		// This is done to make it as easy as possible for beginners who just downloaded GZDoom and Brutal Doom
		// and want to start it as easily as possible with no interest about all the other possibilities.
		autoselectItems();
	}

	// make keyboard actions control the preset list by default
	ui->presetListView->setFocus();

 #if IS_WINDOWS
	// Qt on Windows does not automatically follow OS preferences, so we have to monitor the OS settings for changes
	// and manually change our theme when it does.
	// Rather start this after loading options, because the MainWindow is blocked (is not updating) during the whole
	// options loading including any potential error messages, which if the theme is switched in the middle
	// might show in some kind of half-switched state.
	systemThemeWatcher.start();
 #endif

	if (settings.checkForUpdates)
	{
		updateChecker.checkForUpdates_async(
			/* result callback */[ this ]( UpdateChecker::Result result, QString /*errorDetail*/, QStringVec versionInfo )
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

void MainWindow::timerEvent( QTimerEvent * event )  // called once per second
{
	superClass::timerEvent( event );

	tickCount++;

 #if IS_DEBUG_BUILD
	constexpr uint dirUpdateDelay = 8;
 #else
	constexpr uint dirUpdateDelay = 2;
 #endif

	if (tickCount % dirUpdateDelay == 0)
	{
		updateListsFromDirs();
	}

	if (tickCount % 10 == 0)
	{
		if (optionsNeedUpdate   // don't do unnecessary file writes when nothing has changed
		 && !optionsCorrupted)  // don't overwrite existing file with empty data, just because there was a syntax error
		{
			saveOptions( optionsFilePath );
			optionsNeedUpdate = false;
		}

		if (isCacheDirty())
		{
			saveCache( cacheFilePath );
		}
	}
}

void MainWindow::closeEvent( QCloseEvent * event )
{
	if (!optionsCorrupted)  // don't overwrite existing file with empty data, just because there was a syntax error
		saveOptions( optionsFilePath );

	if (isCacheDirty())
		saveCache( cacheFilePath );

 #if IS_WINDOWS
	systemThemeWatcher.stop(500);
 #endif

	superClass::closeEvent( event );
}

MainWindow::~MainWindow()
{
	delete ui;
}


//----------------------------------------------------------------------------------------------------------------------
//  saving and loading user data

bool MainWindow::saveOptions( const QString & filePath )
{
	// This memeber is not updated regularly, because it is only needed for saving app state. Update it now.
	appearance.geometry = this->geometry();

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
		presetModel.fullList(),
		wdg::getSelectedItemIndex( ui->presetListView ),

		// global settings
		engineSettings,
		iwadSettings,
		mapSettings,
		modSettings,
		settings,

		appearance,
	};

	QJsonDocument jsonDoc = serializeOptionsToJsonDoc( opts );

	return writeJsonToFile( jsonDoc, filePath, "options" );
}

std::unique_ptr< JsonDocumentCtx > MainWindow::readOptions( const QString & filePath )
{
	auto jsonDoc = readJsonFromFile( filePath, "options" );
	if (fs::isValidFile( filePath ) && (!jsonDoc || !jsonDoc->isValid()))  // file exists but couldn't be read or is not a valid JSON
	{
		optionsCorrupted = true;   // don't overwrite it, give user chance to fix it
	}
	return jsonDoc;
}

bool MainWindow::reloadOptions( const QString & filePath )
{
	auto jsonDoc = readOptions( filePath );
	if (!jsonDoc || !jsonDoc->isValid())
	{
		return false;
	}

	// Load appearance as last, so that possible loading errors are still displayed
	// using the current application style and colors to prevent unreadable error messages.

	loadTheRestOfOptions( *jsonDoc );

	loadAppearance( *jsonDoc, /*loadGeometry*/ false );

	return true;
}

void MainWindow::loadAppearance( const JsonDocumentCtx & optionsDoc, bool loadGeometry )
{
	AppearanceToLoad opts
	{
		appearance,
	};

	deserializeAppearanceFromJsonDoc( optionsDoc, opts, loadGeometry );

	restoreAppearance( opts.appearance, loadGeometry );
}

void MainWindow::loadTheRestOfOptions( const JsonDocumentCtx & optionsDoc )
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
		engineSettings,
		iwadSettings,
		mapSettings,
		modSettings,
		settings,
	};

	deserializeOptionsFromJsonDoc( optionsDoc, opts );

	restoreLoadedOptions( std::move(opts) );
}

bool MainWindow::isCacheDirty() const
{
	return os::g_cachedExeInfo.isDirty();
	//	|| doom::g_cachedWadInfo.isDirty();
}

bool MainWindow::saveCache( const QString & filePath )
{
	QJsonObject jsRoot;
	jsRoot["exe_info"] = os::g_cachedExeInfo.serialize();
	//jsRoot["wad_info"] = doom::g_cachedWadInfo.serialize();  // not needed, WAD parsing is probably faster than JSON parsing

	QJsonDocument jsonDoc( jsRoot );
	return writeJsonToFile( jsonDoc, filePath, "file-info cache" );
}

bool MainWindow::loadCache( const QString & filePath )
{
	auto jsonDoc = readJsonFromFile( filePath, "file-info cache", IgnoreEmpty );
	if (!jsonDoc || !jsonDoc->isValid())
	{
		return false;
	}

	const JsonObjectCtx & jsRoot = jsonDoc->rootObject();
	if (JsonObjectCtx jsExeCache = jsRoot.getObject("exe_info"))
		os::g_cachedExeInfo.deserialize( jsExeCache );
	//if (JsonObjectCtx jsWadCache = jsRoot.getObject("wad_info"))
	//	doom::g_cachedWadInfo.deserialize( jsWadCache );  // not needed, WAD parsing is probably faster than JSON parsing

	return true;
}


//----------------------------------------------------------------------------------------------------------------------
//  restoring stored options into the UI

void MainWindow::restoreLoadedOptions( OptionsToLoad && opts )
{
	// Restoring any stored options is tricky.
	// Every change of a selection, like  cmbBox->setCurrentIndex( stored.idx )  or  selectItemByIdx( stored.idx )
	// causes the corresponding callbacks to be called which in turn might cause some other stored value to be changed.
	// For example  ui->engineCmbBox->setCurrentIndex( idx )  calls  selectEngine( idx )  which might indirectly call
	// selectConfig( idx )  which will overwrite the stored selected config and we will then restore a wrong one.
	// The least complicated workaround seems to be simply setting a flag indicating that we are in the middle of
	// restoring saved options, and then prevent storing values when this flag is set.
	restoringOptionsInProgress = true;

	// preset must be deselected first, so that the cleared selections don't save in the selected preset
	wdg::deselectAllAndUnsetCurrent( ui->presetListView );

	// engines
	{
		ui->engineCmbBox->setCurrentIndex( -1 );  // if something is selected, this invokes the callback, which updates the dependent widgets

		disableSelectionCallbacks = true;  // prevent updating engine-dependent widgets back and forth

		engineModel.startCompleteUpdate();
		engineModel.assignList( std::move(opts.engines) );
		fillDerivedEngineInfo( engineModel );  // fill the derived fields of EngineInfo
		engineModel.finishCompleteUpdate();       // if the list is not empty, this changes the engine index from -1 to 0,
		ui->engineCmbBox->setCurrentIndex( -1 );  // but we need it to stay -1

		disableSelectionCallbacks = false;

		// mark the default engine, if chosen
		int defaultIdx = findSuch( engineModel, [&]( const Engine & e ){ return e.getID() == engineSettings.defaultEngine; } );
		if (defaultIdx >= 0)
		{
			engineModel[ defaultIdx ].textColor = themes::getCurrentPalette().defaultEntryText;
		}
		else if (!engineSettings.defaultEngine.isEmpty())
		{
			reportUserError( nullptr, "Default engine no longer exists",
				"Engine that was marked as default ("%engineSettings.defaultEngine%") no longer exists. Please select another one." );
		}
	}

	// IWADs
	{
		wdg::deselectAllAndUnsetCurrent( ui->iwadListView );  // if something is selected, this invokes the callback, which updates the dependent widgets

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

		// mark the default IWAD, if chosen
		int defaultIdx = findSuch( iwadModel, [&]( const IWAD & i ){ return i.getID() == iwadSettings.defaultIWAD; } );
		if (defaultIdx >= 0)
		{
			iwadModel[ defaultIdx ].textColor = themes::getCurrentPalette().defaultEntryText;
		}
		else if (!iwadSettings.defaultIWAD.isEmpty())
		{
			reportUserError( nullptr, "Default IWAD no longer exists",
				"IWAD that was marked as default ("%iwadSettings.defaultIWAD%") no longer exists. Please select another one." );
		}
	}

	// maps
	{
		wdg::deselectAllAndUnsetCurrent( ui->mapDirView );

		resetMapDirModelAndView();  // populates the list from mapSettings.dir (asynchronously)
	}

	// mods
	{
		wdg::deselectAllAndUnsetCurrent( ui->modListView );

		modModel.startCompleteUpdate();
		modModel.clear();
		// mods will be restored to the UI, when a preset is selected
		modModel.finishCompleteUpdate();

		ui->modListView->toggleIcons( modSettings.showIcons );
	}

	// presets
	{
		wdg::deselectAllAndUnsetCurrent( ui->presetListView );  // this invokes the callback, which disables the dependent widgets

		presetModel.startCompleteUpdate();
		presetModel.assignList( std::move(opts.presets) );
		presetModel.finishCompleteUpdate();
	}

	// make sure all paths loaded from JSON are stored in correct format
	togglePathStyle( settings.pathStyle );

	// make sure the correct widgets are enabled/disabled according to the loaded storage settings, until a preset is selected
	togglePresetSubWidgets( false );

	// load the last selected preset
	if (!opts.selectedPreset.isEmpty())
	{
		int selectedPresetIdx = findSuch( presetModel, [&]( const Preset & preset )
		                                               { return preset.name == opts.selectedPreset; } );
		if (selectedPresetIdx >= 0)
		{
			// This invokes the callback, which enables the dependent widgets and calls restorePreset(...)
			wdg::selectSetCurrentAndScrollTo( ui->presetListView, selectedPresetIdx );
		}
		else
		{
			reportUserError( nullptr, "Preset no longer exists",
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

	restoreSelectedEngine( preset );
	restoreSelectedConfig( preset );
	restoreSelectedIWAD( preset );
	restoreSelectedMods( preset );

	// Beware that when calling this from restoreLoadedOptions, the mapModel might not have been populated yet,
	// because it's done asynchronously in a separate thread. In that case this needs to be called again
	// in a callback connected to QFileSystem event, when the mapModel is finally populated.
	restoreSelectedMapPacks( preset );

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

	restoreEnvVars( preset.envVars, ui->presetEnvVarTable );

	restoringPresetInProgress = false;

	// if "Use preset name as directory" is enabled, overwrite the preset custom directories with its name
	// do it with restoringInProgress == false to update the current preset with the new overwriten dirs.
	if (globalOpts.usePresetNameAsDir)
	{
		setAlternativeDirs( fs::sanitizePath( preset.name ) );
	}

	updateLaunchCommand();
}

void MainWindow::restoreSelectedEngine( Preset & preset )
{
	int origIdx = ui->engineCmbBox->currentIndex();
	disableSelectionCallbacks = true;  // prevent unnecessary widget updates, they will be done in the end in a single step

	ui->engineCmbBox->setCurrentIndex( -1 );

	if (!preset.selectedEnginePath.isEmpty())  // the engine combo box might have been empty when creating this preset
	{
		int engineIdx = findSuch( engineModel, [&]( const Engine & engine )
												   { return engine.executablePath == preset.selectedEnginePath; } );
		if (engineIdx >= 0)
		{
			ui->engineCmbBox->setCurrentIndex( engineIdx );

			if (!fs::isValidFile( preset.selectedEnginePath ))
			{
				reportUserError( this, "Engine no longer exists",
					"Engine selected for this preset ("%preset.selectedEnginePath%") no longer exists. "
					"Please update its path in Menu -> Initial Setup, or select another one."
				);
				highlightInvalidListItem( engineModel[ engineIdx ] );
			}
		}
		else
		{
			reportUserError( this, "Engine no longer exists",
				"Engine selected for this preset ("%preset.selectedEnginePath%") was removed from engine list. "
				"Please select another one."
			);
			preset.selectedEnginePath.clear();
			preset.compatOpts.compatLevel = -1;  // compat level is engine-specific so the previous level is no longer valid
		}
	}

	disableSelectionCallbacks = false;
	int newIdx = ui->engineCmbBox->currentIndex();

	// manually notify our class about the change, so that the preset and dependent widgets get updated
	// This is needed before configs, saves and demos are restored, so that the entries are ready to be selected from.
	if (newIdx != origIdx)
	{
		onEngineSelected( newIdx );
	}
}

void MainWindow::restoreSelectedConfig( Preset & preset )
{
	int origIdx = ui->configCmbBox->currentIndex();
	disableSelectionCallbacks = true;  // prevent unnecessary widget updates, they will be done in the end in a single step

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
			reportUserError( this, "Config no longer exists",
				"Config file selected for this preset ("%preset.selectedConfig%") no longer exists. "
				"Please select another one."
			);
			preset.selectedConfig.clear();
		}
	}

	disableSelectionCallbacks = false;
	int newIdx = ui->configCmbBox->currentIndex();

	// manually notify our class about the change, so that the preset and dependent widgets get updated
	if (newIdx != origIdx)
	{
		onConfigSelected( newIdx );
	}
}

void MainWindow::restoreSelectedIWAD( Preset & preset )
{
	int origIwadIdx = wdg::getSelectedItemIndex( ui->iwadListView );
	disableSelectionCallbacks = true;  // prevent unnecessary widget updates, they will be done in the end in a single step

	wdg::deselectAllAndUnsetCurrent( ui->iwadListView );

	if (!preset.selectedIWAD.isEmpty())  // the IWAD may have not been selected when creating this preset
	{
		int iwadIdx = findSuch( iwadModel, [&]( const IWAD & iwad ) { return iwad.path == preset.selectedIWAD; } );
		if (iwadIdx >= 0)
		{
			wdg::selectSetCurrentAndScrollTo( ui->iwadListView, iwadIdx );

			if (!fs::isValidFile( preset.selectedIWAD ))
			{
				reportUserError( this, "IWAD no longer exists",
					"IWAD selected for this preset ("%preset.selectedIWAD%") no longer exists. "
					"Please select another one."
				);
				highlightInvalidListItem( iwadModel[ iwadIdx ] );
			}
		}
		else
		{
			reportUserError( this, "IWAD no longer exists",
				"IWAD selected for this preset ("%preset.selectedIWAD%") no longer exists. "
				"Please select another one."
			);
			preset.selectedIWAD.clear();
		}
	}

	disableSelectionCallbacks = false;
	int newIwadIdx = wdg::getSelectedItemIndex( ui->iwadListView );

	// manually notify our class about the change, so that the preset and dependent widgets get updated
	// This is needed before launch options are restored, so that the map names are ready to be selected.
	if (newIwadIdx != origIwadIdx)
	{
		onIWADToggled( QItemSelection(), QItemSelection()/*TODO*/ );
	}
}

void MainWindow::restoreSelectedMapPacks( Preset & preset )
{
	auto origSelection = ui->mapDirView->selectionModel()->selection();
	disableSelectionCallbacks = true;  // prevent unnecessary widget updates, they will be done in the end in a single step

	QDir mapRootDir = mapModel.rootDirectory();

	wdg::deselectAllAndUnsetCurrent( ui->mapDirView );

	const QStringVec mapPacksCopy = preset.selectedMapPacks;
	preset.selectedMapPacks.clear();  // clear the list in the preset and let it repopulate only with valid items
	for (const QString & path : mapPacksCopy)
	{
		QModelIndex mapIdx = mapModel.index( path );
		if (mapIdx.isValid() && fs::isInsideDir( path, mapRootDir ))
		{
			if (fs::isValidEntry( path ))
			{
				preset.selectedMapPacks.append( path );  // put back only items that are valid
				wdg::selectSetCurrentAndScrollTo( ui->mapDirView, mapIdx );
			}
			else
			{
				reportUserError( this, "Map file no longer exists",
					"Map file selected for this preset ("%path%") no longer exists."
				);
			}
		}
		else
		{
			reportUserError( this, "Map file no longer exists",
				"Map file selected for this preset ("%path%") couldn't be found in the map directory ("%mapRootDir.path()%")."
			);
		}
	}

	disableSelectionCallbacks = false;
	auto newSelection = ui->mapDirView->selectionModel()->selection();

	// manually notify our class about the change, so that the preset and dependent widgets get updated
	if (newSelection != origSelection)
	{
		onMapPackToggled( newSelection, QItemSelection()/*TODO*/ );
	}
}

void MainWindow::restoreSelectedMods( Preset & preset )
{
	wdg::deselectAllAndUnsetCurrent( ui->modListView );  // this actually doesn't call a toggle callback, because the list is checkbox-based

	modModel.startCompleteUpdate();
	modModel.clear();
	for (Mod & mod : preset.mods)
	{
		modModel.append( mod );
		if ((!mod.isSeparator && !mod.isCmdArg) && !fs::isValidEntry( mod.path ))
		{
			// Let's just highlight it now, we will show warning when the user tries to launch it.
			//reportUserError( this, "Mod no longer exists",
			//	"A mod file \""%mod.path%"\" from this preset no longer exists. Please update it." );
			highlightInvalidListItem( modModel.last() );
		}
	}
	modModel.finishCompleteUpdate();
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
			reportUserError( this, "Save file no longer exists",
				"Save file \""%launchOpts.saveFile%"\" no longer exists. Please select another one."
			);
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
			reportUserError( this, "Demo file no longer exists",
				"Demo file \""%launchOpts.demoFile_replay%"\" no longer exists. Please select another one."
			);
			launchOpts.demoFile_replay.clear();  // if previous index was -1, callback is not called, so we clear the invalid item manually
		}
	}

	// multiplayer
	ui->multiplayerGrpBox->setChecked( multOpts.isMultiplayer );
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
	ui->skillCmbBox->setCurrentIndex( opts.skillIdx - 1 );
	if (opts.skillIdx < Skill::Custom)
		ui->skillSpinBox->setValue( opts.skillIdx );
	else
		ui->skillSpinBox->setValue( opts.skillNum );

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
		// engine might have been removed, or its family was changed by the user
		logLogicError() << "stored compat level ("<<compatLevelIdx<<") is out of bounds of the current combo-box content";
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

	restoreEnvVars( opts.envVars, ui->globalEnvVarTable );
}

void MainWindow::restoreEnvVars( const EnvVars & envVars, QTableWidget * table )
{
	disableEnvVarsCallbacks = true;

	for (const auto & envVar : envVars)
	{
		int newRowIdx = table->rowCount();
		table->insertRow( newRowIdx );
		table->setItem( newRowIdx, VarNameColumn, new QTableWidgetItem( envVar.name ) );
		table->setItem( newRowIdx, VarValueColumn, new QTableWidgetItem( envVar.value ) );
	}

	disableEnvVarsCallbacks = false;
}

void MainWindow::restoreAppearance( const AppearanceSettings & appearance, bool restoreGeometry )
{
	// set style first, on Windows it may be overriden by color scheme
	if (!appearance.appStyle.isNull())
		themes::setAppStyle( appearance.appStyle );

	if (IS_WINDOWS || appearance.colorScheme != ColorScheme::SystemDefault)
		themes::setAppColorScheme( appearance.colorScheme );

	if (restoreGeometry)
	{
		restoreWindowGeometry( appearance.geometry );
	}
}

void MainWindow::restoreWindowGeometry( const WindowGeometry & geometry )
{
	auto newGeometry = this->geometry();  // start with the current geometry and update it with the values we have

	if (geometry.x != INT_MIN && geometry.y != INT_MIN)  // our internal marker that the coordinates have not been read properly
	{
		if (areScreenCoordinatesValid( geometry.x, geometry.y ))  // move the window only if the coordinates make sense
		{
			newGeometry.moveTo( geometry.x, geometry.y );
		}
		else
		{
			logInfo() << "invalid coordinates detected ("<<geometry.x<<","<<geometry.y<<") leaving window at the default position";
		}
	}

	if (geometry.width > 0 && geometry.height > 0)
	{
		newGeometry.setSize({ geometry.width, geometry.height });
	}

	this->setGeometry( newGeometry );
}


//----------------------------------------------------------------------------------------------------------------------
//  dialogs

void MainWindow::runAboutDialog()
{
	AboutDialog dialog( this, settings.checkForUpdates );

	dialog.exec();

	scheduleSavingOptions( settings.checkForUpdates != dialog.checkForUpdates );
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
		pathConvertor,
		engineSettings,
		engineModel.list(),
		iwadSettings,
		iwadModel.list(),
		mapSettings,
		modSettings,
		settings,
		appearance
	);

	int code = dialog.exec();

	// update the data only if user clicked Ok
	if (code == QDialog::Accepted)
	{
		// write down the previously selected items
		auto currentEngine = wdg::getCurrentItemID( ui->engineCmbBox, engineModel );
		auto currentIWAD = wdg::getCurrentItemID( ui->iwadListView, iwadModel );
		auto selectedIWAD = wdg::getSelectedItemID( ui->iwadListView, iwadModel );

		// prevent unnecessary updates when an engine or IWAD is deselected and then the same one selected again.
		disableSelectionCallbacks = true;

		// deselect the items
		ui->engineCmbBox->setCurrentIndex( -1 );
		wdg::deselectAllAndUnsetCurrent( ui->iwadListView );

		// make sure all data and indexes are invalidated and no longer used
		engineModel.startCompleteUpdate();
		iwadModel.startCompleteUpdate();

		// update our data from the dialog
		engineSettings = std::move( dialog.engineSettings );
		engineModel.assignList( std::move( dialog.engineModel.list() ) );
		//fillDerivedEngineInfo( engineModel );  // fill the derived fields of EngineInfo
		iwadSettings = std::move( dialog.iwadSettings );
		iwadModel.assignList( std::move( dialog.iwadModel.list() ) );
		mapSettings = std::move( dialog.mapSettings );
		modSettings = std::move( dialog.modSettings );
		settings = std::move( dialog.settings );
		appearance = std::move( dialog.appearance );

		// update all stored paths
		togglePathStyle( settings.pathStyle );
		currentEngine = pathConvertor.convertPath( currentEngine );
		currentIWAD = pathConvertor.convertPath( currentIWAD );
		selectedIWAD = pathConvertor.convertPath( selectedIWAD );

		// notify the widgets to re-draw their content
		engineModel.finishCompleteUpdate();
		iwadModel.finishCompleteUpdate();
		resetMapDirModelAndView();

		// select back the previously selected items
		wdg::setCurrentItemByID( ui->engineCmbBox, engineModel, currentEngine );
		wdg::setCurrentItemByID( ui->iwadListView, iwadModel, currentIWAD );
		wdg::selectItemByID( ui->iwadListView, iwadModel, selectedIWAD );

		disableSelectionCallbacks = false;

		// Regardless whether the index of the selected items actually changed or whether they still exist,
		// we have to call these callbacks, because their content (e.g. config dir) might have changed,
		// and we need to update all the widgets dependent on that content.
		onEngineSelected( ui->engineCmbBox->currentIndex() );
		onIWADToggled( QItemSelection(), QItemSelection()/*TODO*/ );

		scheduleSavingOptions();
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

		scheduleSavingOptions();

		updateOptionsGrpBoxTitles( settings );

		bool isPresetSelected = getSelectedPreset() != nullptr;
		ui->launchModeGrpBox->setEnabled( isPresetSelected || settings.launchOptsStorage != StoreToPreset );
		ui->gameplayGrpBox->setEnabled( isPresetSelected || settings.gameOptsStorage != StoreToPreset );
		ui->compatGrpBox->setEnabled( isPresetSelected || settings.compatOptsStorage != StoreToPreset );
		ui->multiplayerGrpBox->setEnabled( isPresetSelected || settings.launchOptsStorage != StoreToPreset );
		ui->videoGrpBox->setEnabled( isPresetSelected || settings.videoOptsStorage != StoreToPreset );
		ui->audioGrpBox->setEnabled( isPresetSelected || settings.audioOptsStorage != StoreToPreset );
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
		scheduleSavingOptions();
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
		scheduleSavingOptions();
		updateLaunchCommand();
	}
}

void MainWindow::openEngineDataDir()
{
	const EngineInfo * selectedEngine = getSelectedEngine();
	if (!selectedEngine)
	{
		reportUserError( this, "No engine selected", "You haven't selected any engine." );
		return;
	}

	os::openDirectoryWindow( selectedEngine->dataDir );  // errors are handled inside
}

void MainWindow::cloneConfig()
{
	const ConfigFile * selectedConfig = getSelectedConfig();
	if (!selectedConfig)
	{
		reportUserError( this, "No config selected", "You haven't selected any config file to be cloned." );
		return;
	}

	QString configDirStr = getConfigDir();  // config dir cannot be empty, otherwise currentConfigFileName would be empty
	QDir configDir( configDirStr );
	QFileInfo oldConfig( configDir.filePath( selectedConfig->fileName ) );

	if (!oldConfig.exists())  // it can't be a directory, because the combox is only filled with files
	{
		reportUserError( this, "Invalid config selected", "This config file no longer exists, please select another one." );
		return;
	}

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
		reportRuntimeError( this, "Error copying file", "Couldn't create file \""%newConfigPath%"\". Check permissions." );
		return;
	}

	// regenerate config list so that we can select it
	updateConfigFilesFromDir( &configDirStr );

	// select the new file automatically for convenience
	QString newConfigName = fs::getFileNameFromPath( newConfigPath );
	int newConfigIdx = findSuch( configModel, [&]( const ConfigFile & cfg ) { return cfg.fileName == newConfigName; } );
	if (newConfigIdx >= 0)
	{
		ui->configCmbBox->setCurrentIndex( newConfigIdx );
	}
}


//----------------------------------------------------------------------------------------------------------------------
//  item selection

/// Automatically selects essential items (like engine of IWAD) marked as default, or those that are alone in their list.
void MainWindow::autoselectItems()
{
	if (ui->engineCmbBox->currentIndex() < 0)
	{
		if (engineModel.size() == 1)
		{
			// automatically select engine that is alone in the list
			// This is done to make it as easy as possible for beginners who just downloaded GZDoom and Brutal Doom
			// and want to start it as easily as possible with no interest about all the other possibilities.
			ui->engineCmbBox->setCurrentIndex( 0 );
		}
		else
		{
			// select engine marked as default
			int defaultIdx = findSuch( engineModel, [&]( const Engine & e ){ return e.getID() == engineSettings.defaultEngine; } );
			if (defaultIdx >= 0)
				ui->engineCmbBox->setCurrentIndex( defaultIdx );
		}
	}

	if (!wdg::isSomethingSelected( ui->iwadListView ))
	{
		if (iwadModel.size() == 1)
		{
			wdg::selectAndSetCurrentByIndex( ui->iwadListView, 0 );
		}
		else
		{
			// select IWAD marked as default
			int defaultIdx = findSuch( iwadModel, [&]( const IWAD & i ){ return i.getID() == iwadSettings.defaultIWAD; } );
			if (defaultIdx >= 0)
				wdg::selectAndSetCurrentByIndex( ui->iwadListView, defaultIdx );
		}
	}
}

void MainWindow::onPresetToggled( const QItemSelection & /*selected*/, const QItemSelection & /*deselected*/ )
{
	int selectedPresetIdx = wdg::getSelectedItemIndex( ui->presetListView );
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
	ui->engineDirBtn->setEnabled( enabled );
	ui->configCmbBox->setEnabled( enabled );
	ui->configCloneBtn->setEnabled( enabled );
	ui->iwadListView->setEnabled( enabled );
	ui->mapDirView->setEnabled( enabled );
	ui->modListView->setEnabled( enabled );
	ui->modBtnAdd->setEnabled( enabled );
	ui->modBtnAddDir->setEnabled( enabled );
	ui->modBtnAddArg->setEnabled( enabled );
	ui->modBtnDel->setEnabled( enabled );
	ui->modBtnUp->setEnabled( enabled );
	ui->modBtnDown->setEnabled( enabled );
	ui->presetCmdArgsLine->setEnabled( enabled );
	ui->presetEnvVarTable->setEnabled( enabled );
	ui->presetEnvVarBtnAdd->setEnabled( enabled );
	ui->presetEnvVarBtnDel->setEnabled( enabled );

	ui->launchModeGrpBox->setEnabled( enabled || settings.launchOptsStorage != StoreToPreset );
	ui->gameplayGrpBox->setEnabled( enabled || settings.gameOptsStorage != StoreToPreset);
	ui->compatGrpBox->setEnabled( enabled || settings.compatOptsStorage != StoreToPreset);
	ui->multiplayerGrpBox->setEnabled( enabled || settings.launchOptsStorage != StoreToPreset );
	ui->videoGrpBox->setEnabled( enabled || settings.videoOptsStorage != StoreToPreset);
	ui->audioGrpBox->setEnabled( enabled || settings.audioOptsStorage != StoreToPreset);
	ui->altPathsGrpBox->setEnabled( enabled );
	ui->presetEnvVarsGrpBox->setEnabled( enabled );
}

void MainWindow::clearPresetSubWidgets()
{
	ui->engineCmbBox->setCurrentIndex( -1 );
	// map names, saves and demos will be cleared and deselected automatically

	wdg::deselectAllAndUnsetCurrent( ui->iwadListView );
	wdg::deselectAllAndUnsetCurrent( ui->mapDirView );
	wdg::deselectAllAndUnsetCurrent( ui->modListView );

	modModel.startCompleteUpdate();
	modModel.clear();
	modModel.finishCompleteUpdate();

	ui->presetCmdArgsLine->clear();
}

void MainWindow::onEngineSelected( int index )
{
	if (disableSelectionCallbacks)
		return;

	const EngineInfo * selectedEngine = index >= 0 ? &engineModel[ index ] : nullptr;
	const QString & enginePath = selectedEngine ? selectedEngine->executablePath : emptyString;

	bool storageModified = STORE_TO_CURRENT_PRESET_IF_SAFE( selectedEnginePath, enginePath );

	// engine's data dir has changed -> from now on rebase engine data paths to the new dir, if empty it will rebase to "."
	engineDataDirRebaser.setOutputBaseDir( selectedEngine ? selectedEngine->dataDir : emptyString );

	// only allow editing, if the engine supports specifying map by its name
	bool supportsCustomMapNames = selectedEngine ? selectedEngine->supportsCustomMapNames() : false;
	ui->mapCmbBox->setEditable( supportsCustomMapNames );
	ui->mapCmbBox_demo->setEditable( supportsCustomMapNames );

	// automatic alt dirs depend on the engine traits, which has now changed, so this needs to be refreshed
	if (globalOpts.usePresetNameAsDir)
	{
		const Preset * selectedPreset = getSelectedPreset();
		const QString & presetName = selectedPreset ? selectedPreset->name : emptyString;
		setAlternativeDirs( fs::sanitizePath( presetName ) );
	}

	// update related UI elements
	ui->engineDirBtn->setEnabled( selectedEngine != nullptr );

	updateConfigFilesFromDir();
	updateSaveFilesFromDir();
	updateDemoFilesFromDir();
	updateCompatLevels();

	scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onConfigSelected( int index )
{
	if (disableSelectionCallbacks)
		return;

	const ConfigFile * selectedConfig = index > 0 ? &configModel[ index ] : nullptr;  // at index 0 there is an empty placeholder to allow deselecting config
	const QString & configFileName = selectedConfig ? selectedConfig->fileName : emptyString;

	bool storageModified = STORE_TO_CURRENT_PRESET_IF_SAFE( selectedConfig, configFileName );

	/*if (selectedConfig)
	{
		QString configPath = fs::getPathFromFileName( getConfigDir(), configFileName );
	}*/

	// update related UI elements
	ui->configCloneBtn->setEnabled( selectedConfig != nullptr );

	scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onIWADToggled( const QItemSelection & /*selected*/, const QItemSelection & /*deselected*/ )
{
	if (disableSelectionCallbacks)
		return;

	const IWAD * selectedIWAD = getSelectedIWAD();
	const QString & iwadPath = selectedIWAD ? selectedIWAD->path : emptyString;

	bool storageModified = STORE_TO_CURRENT_PRESET_IF_SAFE( selectedIWAD, iwadPath );

	updateMapsFromSelectedWADs();

	scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onMapPackToggled( const QItemSelection & /*selected*/, const QItemSelection & /*deselected*/ )
{
	if (disableSelectionCallbacks)
		return;

	QStringVec selectedMapPacks = getSelectedMapPacks();

	/*bool storageModified =*/ STORE_TO_CURRENT_PRESET_IF_SAFE( selectedMapPacks, selectedMapPacks );

	// expand the parent directory nodes that are collapsed
	const auto selectedRows = wdg::getSelectedRows( ui->mapDirView );
	for (const QModelIndex & index : selectedRows)
		wdg::expandParentsOfNode( ui->mapDirView, index );

	updateMapsFromSelectedWADs( &selectedMapPacks );

	// if this is a known map pack, that starts at different level than the first one, automatically select it
	if (selectedMapPacks.size() >= 1 && !fs::isDirectory( selectedMapPacks[0] ))
	{
		// if there is multiple of them, there isn't really any better solution than to just take the first one
		QString wadFileName = fs::getFileNameFromPath( selectedMapPacks[0] );
		QString startingMap = doom::getStartingMap( wadFileName );
		if (!startingMap.isEmpty())
		{
			if (ui->mapCmbBox->findText( startingMap ) >= 0)
			{
				ui->mapCmbBox->setCurrentText( startingMap );
				ui->mapCmbBox_demo->setCurrentText( startingMap );
			}
			else
			{
				reportRuntimeError( this, "Cannot set starting map",
					"Starting map "%startingMap%" was not found in the \""%wadFileName%"\"." );
			}
		}
	}

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onPresetDataChanged( const QModelIndex & topLeft, const QModelIndex &, const QVector<int> & roles )
{
	int editedIdx = topLeft.row();  // there cannot be more than 1 items edited at a time

	if (roles.contains( Qt::EditRole ))
	{
		// automatic alt dirs are derived from preset name, which has now changed, so this needs to be refreshed
		if (globalOpts.usePresetNameAsDir && wdg::isSelectedIndex( ui->presetListView, editedIdx ))
		{
			const QString & presetName = presetModel[ editedIdx ].name;
			setAlternativeDirs( fs::sanitizePath( presetName ) );
		}
	}

	scheduleSavingOptions( true );  // there's no way to determine if the name has changed, because the value in the model is already modified.
}

void MainWindow::onModDataChanged( const QModelIndex & topLeft, const QModelIndex & bottomRight, const QVector<int> & roles )
{
	int topModIdx = topLeft.row();
	int bottomModIdx = bottomRight.row();

	auto updateEachModifiedModInPreset = [ this, topModIdx, bottomModIdx ]( const auto & updateMod )
	{
		Preset * selectedPreset = getSelectedPreset();
		if (selectedPreset && !restoringPresetInProgress)
		{
			for (int idx = topModIdx; idx <= bottomModIdx; idx++)
			{
				updateMod( selectedPreset->mods[ idx ], modModel[ idx ] );
			}
		}
	};

	if (roles.contains( Qt::CheckStateRole ))  // check state of some checkboxes changed
	{
		// update the current preset
		updateEachModifiedModInPreset( []( Mod & presetMod, const Mod & changedMod )
		{
			presetMod.checked = changedMod.checked;
		});
	}
	else if (roles.contains( Qt::EditRole ))  // name of separator or data of custom cmd argument changed
	{
		// update the current preset
		updateEachModifiedModInPreset( []( Mod & presetMod, const Mod & changedMod )
		{
			presetMod.fileName = changedMod.fileName;
		});
	}

	scheduleSavingOptions( true );  // we can assume options storage was modified, otherwise this callback wouldn't be called
	updateLaunchCommand();
}

void MainWindow::showTxtDescriptionFor( const QString & filePath, const QString & contentType )
{
	QFileInfo dataFileInfo( filePath );

	if (!dataFileInfo.isFile())  // user could click on a directory
	{
		return;
	}

	// get the corresponding file with txt suffix
	QFileInfo descFileInfo( fs::replaceFileSuffix( dataFileInfo.filePath(), "txt" ) );
	if (!descFileInfo.isFile())
	{
		// try TXT in case we are in a case-sensitive file-system such as Linux
		descFileInfo = QFileInfo( fs::replaceFileSuffix( dataFileInfo.filePath(), "TXT" ) );
		if (!descFileInfo.isFile())
		{
			reportUserError( this, "Cannot open "%contentType,
				capitalize( contentType )%" file \""%descFileInfo.fileName()%"\" does not exist" );
			return;
		}
	}

	QFile descFile( descFileInfo.filePath() );
	if (!descFile.open( QIODevice::Text | QIODevice::ReadOnly ))
	{
		reportRuntimeError( this, "Cannot open "%contentType,
			"Failed to open map "%contentType%" \""%descFileInfo.fileName()%"\" ("%descFile.errorString()%")" );
		return;
	}

	QByteArray desc = descFile.readAll();

	QDialog descDialog( this );
	descDialog.setObjectName( "FileDescription" );
	descDialog.setWindowTitle( descFileInfo.fileName() );
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

void MainWindow::showIWADDesc( const QModelIndex & index )
{
	showTxtDescriptionFor( iwadModel[ index.row() ].path, "IWAD description" );
}

void MainWindow::showMapPackDesc( const QModelIndex & index )
{
	showTxtDescriptionFor( mapModel.filePath( index ), "map description" );
}

void MainWindow::showModDesc( const QModelIndex & index )
{
	showTxtDescriptionFor( modModel[ index.row() ].path, "mod description" );
}

void MainWindow::onMapDirUpdated( const QString & path )
{
	// the QFileSystemModel mapModel has finally updated its content from mapSettings.dir
	if (path == pathConvertor.getAbsolutePath( mapSettings.dir ))
	{
		// idiotic workaround, because Qt never stops surprising me with its stupidity.
		//
		// The QFileSystemModel::directoryLoaded signal is supposed to indicate the mapModel is fully loaded,
		// but still when we call mapModel.index( path ), it returns a zeroed-out model index with incorrect row number.
		// This is a desperate attempt to call restoreSelectedMapPacks() when the mapModel.index( path ) finally starts
		// returning valid indexes so that we can select the right items in the view.
		QTimer::singleShot( 0, nullptr, [ this ]()
		{
			if (Preset * preset = getSelectedPreset())
			{
				// now we can finally select the right items in the map pack view
				restoreSelectedMapPacks( *preset );
			}
		});
	}
}


//----------------------------------------------------------------------------------------------------------------------
//  preset list manipulation

static uint getHighestDefaultPresetNameIndex( const QList< Preset > & presetList )
{
	uint maxIndex = 0;
	static const QRegularExpression defaultPresetRegex("Preset(\\d+)");
	for (const Preset & preset : presetList)
	{
		auto match = defaultPresetRegex.match( preset.name );
		if (match.hasMatch())
		{
			bool isInt;
			uint index = match.captured(1).toUInt( &isInt );
			if (isInt && index > maxIndex)
				maxIndex = index;
		}
	}
	return maxIndex;
}

void MainWindow::presetAdd()
{
	uint presetNum = getHighestDefaultPresetNameIndex( presetModel.fullList() ) + 1;

	int appendedIdx = wdg::appendItem( ui->presetListView, presetModel, { "Preset"+QString::number( presetNum ) } );

	// clear the widgets to represent an empty preset
	// the widgets must be cleared AFTER the new preset is added and selected, otherwise it will clear the prev preset
	clearPresetSubWidgets();

	// try to select the essential items automatically
	autoselectItems();

	// open edit mode so that user can name the preset
	wdg::editItemAtIndex( ui->presetListView, appendedIdx );

	scheduleSavingOptions();
}

void MainWindow::presetDelete()
{
	const Preset * selectedPreset = getSelectedPreset();
	if (selectedPreset && !selectedPreset->isSeparator)
	{
		QMessageBox::StandardButton reply = QMessageBox::question( this,
			"Delete preset?", "Are you sure you want to delete preset "%selectedPreset->name%"?",
			QMessageBox::Yes | QMessageBox::No
		);
		if (reply != QMessageBox::Yes)
			return;
	}

	int deletedIdx = wdg::deleteSelectedItem( ui->presetListView, presetModel );
	if (deletedIdx < 0)  // no item was selected
		return;

	selectedPreset = nullptr;  // no longer exists

	int nextPresetIdx = wdg::getSelectedItemIndex( ui->presetListView );
	if (nextPresetIdx >= 0)
	{
		restorePreset( nextPresetIdx );  // select the next preset
	}
	else
	{
		togglePresetSubWidgets( false );  // disable the widgets so that user can't enter data that would not be saved anywhere
		clearPresetSubWidgets();
	}

	scheduleSavingOptions();
}

void MainWindow::presetClone()
{
	int selectedIdx = wdg::cloneSelectedItem( ui->presetListView, presetModel );

	if (selectedIdx >= 0)
	{
		// open edit mode so that user can name the preset
		wdg::editItemAtIndex( ui->presetListView, presetModel.size() - 1 );

		scheduleSavingOptions();
	}
}

void MainWindow::presetMoveUp()
{
	int selectedIdx = wdg::moveUpSelectedItem( ui->presetListView, presetModel );

	if (selectedIdx >= 0)
	{
		scheduleSavingOptions();
	}
}

void MainWindow::presetMoveDown()
{
	int selectedIdx = wdg::moveDownSelectedItem( ui->presetListView, presetModel );

	if (selectedIdx >= 0)
	{
		scheduleSavingOptions();
	}
}

void MainWindow::presetInsertSeparator()
{
	Preset separator;
	separator.isSeparator = true;
	separator.name = "New Separator";

	int selectedIdx = wdg::getSelectedItemIndex( ui->presetListView );
	int insertIdx = selectedIdx < 0 ? presetModel.size() : selectedIdx;  // append if none

	wdg::insertItem( ui->presetListView, presetModel, separator, insertIdx );

	// open edit mode so that user can name the preset
	wdg::editItemAtIndex( ui->presetListView, insertIdx );

	scheduleSavingOptions();
}

void MainWindow::onPresetsReordered()
{
	scheduleSavingOptions();
}

void MainWindow::searchPresets( const QString & phrase, bool caseSensitive, bool useRegex )
{
	if (phrase.length() > 0)
	{
		// remember which preset was selected before the search results were shown
		if (wdg::isSomethingSelected( ui->presetListView ))
		{
			selectedPresetBeforeSearch = wdg::getSelectedItemID( ui->presetListView, presetModel );
		}

		// filter the model data
		presetModel.startCompleteUpdate();
		presetModel.search( phrase, caseSensitive, useRegex );
		presetModel.finishCompleteUpdate();

		// try to re-select the same preset as before
		if (!selectedPresetBeforeSearch.isEmpty())
		{
			wdg::selectItemByID( ui->presetListView, presetModel, selectedPresetBeforeSearch );
		}

		// disable UI actions that are not allowed in a filtered list
		ui->presetBtnAdd->setEnabled( false );
		ui->presetBtnClone->setEnabled( false );
		ui->presetBtnUp->setEnabled( false );
		ui->presetBtnDown->setEnabled( false );
		ui->presetListView->toggleListModifications( false );
		ui->presetListView->toggleIntraWidgetDragAndDrop( false );
	}
	else
	{
		// remember which preset was selected before the search results were deleted
		if (wdg::isSomethingSelected( ui->presetListView ))
		{
			selectedPresetBeforeSearch = wdg::getSelectedItemID( ui->presetListView, presetModel );
		}

		// restore the model data
		presetModel.startCompleteUpdate();
		presetModel.restore();
		presetModel.finishCompleteUpdate();

		// try to re-select the same preset as before
		if (!selectedPresetBeforeSearch.isEmpty())
		{
			wdg::selectItemByID( ui->presetListView, presetModel, selectedPresetBeforeSearch );
		}

		// re-enable all UI actions
		ui->presetBtnAdd->setEnabled( true );
		ui->presetBtnClone->setEnabled( true );
		ui->presetBtnUp->setEnabled( true );
		ui->presetBtnDown->setEnabled( true );
		ui->presetListView->toggleListModifications( true );
		ui->presetListView->toggleIntraWidgetDragAndDrop( true );
	}
}


//----------------------------------------------------------------------------------------------------------------------
//  mod list manipulation

void MainWindow::modAdd()
{
	QStringList paths = OwnFileDialog::getOpenFileNames( this, "Locate the mod file", modSettings.dir,
		  makeFileFilter( "Doom mod files", doom::pwadSuffixes )
		+ makeFileFilter( "DukeNukem data files", doom::dukeSuffixes )
		+ "All files (*)"
	);
	if (paths.isEmpty())  // user probably clicked cancel
		return;

	// the path comming out of the file dialog is always absolute
	if (pathConvertor.usingRelativePaths())
		for (QString & path : paths)
			path = pathConvertor.getRelativePath( path );

	Preset * selectedPreset = getSelectedPreset();

	for (const QString & path : paths)
	{
		Mod mod( QFileInfo( path ), true );

		wdg::appendItem( ui->modListView, modModel, mod );

		// add it also to the current preset
		if (selectedPreset)
		{
			selectedPreset->mods.append( mod );
		}
	}

	scheduleSavingOptions();
	updateLaunchCommand();
}

void MainWindow::modAddDir()
{
	QString path = OwnFileDialog::getExistingDirectory( this, "Locate the mod directory", modSettings.dir );

	if (path.isEmpty())  // user probably clicked cancel
		return;

	// the path comming out of the file dialog is always absolute
	if (pathConvertor.usingRelativePaths())
		path = pathConvertor.getRelativePath( path );

	Mod mod;
	mod.path = path;
	mod.fileName = fs::getFileNameFromPath( path );
	mod.checked = true;

	wdg::appendItem( ui->modListView, modModel, mod );

	// add it also to the current preset
	if (Preset * selectedPreset = getSelectedPreset())
	{
		selectedPreset->mods.append( mod );
	}

	scheduleSavingOptions();
	updateLaunchCommand();
}

void MainWindow::modAddArg()
{
	Mod mod;
	mod.isCmdArg = true;
	mod.checked = true;

	int appendedIdx = wdg::appendItem( ui->modListView, modModel, mod );

	// add it also to the current preset
	if (Preset * selectedPreset = getSelectedPreset())
	{
		selectedPreset->mods.append( mod );
	}

	// open edit mode so that user can enter the command line argument
	wdg::editItemAtIndex( ui->modListView, appendedIdx );

	scheduleSavingOptions();
	updateLaunchCommand();
}

void MainWindow::modDelete()
{
	QVector<int> deletedIndexes = wdg::deleteSelectedItems( ui->modListView, modModel );

	if (deletedIndexes.isEmpty())  // no item was selected
		return;

	// remove it also from the current preset
	if (Preset * selectedPreset = getSelectedPreset())
	{
		int deletedCnt = 0;
		for (int deletedIdx : deletedIndexes)
		{
			selectedPreset->mods.removeAt( deletedIdx - deletedCnt );
			deletedCnt++;
		}
	}

	scheduleSavingOptions();
	updateLaunchCommand();
}

void MainWindow::modMoveUp()
{
	restoringPresetInProgress = true;  // prevent onModDataChanged() from updating our preset too early and incorrectly

	QVector<int> movedIndexes = wdg::moveUpSelectedItems( ui->modListView, modModel );

	restoringPresetInProgress = false;

	if (movedIndexes.isEmpty())  // no item was selected or they were already at the top
		return;

	// move it up also in the preset
	if (Preset * selectedPreset = getSelectedPreset())
	{
		for (int movedIdx : movedIndexes)
		{
			selectedPreset->mods.move( movedIdx, movedIdx - 1 );
		}
	}

	scheduleSavingOptions();
	updateLaunchCommand();
}

void MainWindow::modMoveDown()
{
	restoringPresetInProgress = true;  // prevent onModDataChanged() from updating our preset too early and incorrectly

	QVector<int> movedIndexes = wdg::moveDownSelectedItems( ui->modListView, modModel );

	restoringPresetInProgress = false;

	if (movedIndexes.isEmpty())  // no item was selected or they were already at the bottom
		return;

	// move it down also in the preset
	if (Preset * selectedPreset = getSelectedPreset())
	{
		for (int movedIdx : movedIndexes)
		{
			selectedPreset->mods.move( movedIdx, movedIdx + 1 );
		}
	}

	scheduleSavingOptions();
	updateLaunchCommand();
}

void MainWindow::modInsertSeparator()
{
	Mod separator;
	separator.isSeparator = true;
	separator.fileName = "New Separator";

	QVector<int> selectedIndexes = wdg::getSelectedItemIndexes( ui->modListView );
	int insertIdx = selectedIndexes.empty() ? modModel.size() : selectedIndexes[0];  // append if none

	wdg::insertItem( ui->modListView, modModel, separator, insertIdx );

	// insert it also to the current preset
	if (Preset * selectedPreset = getSelectedPreset())
	{
		selectedPreset->mods.insert( insertIdx, separator );
	}

	// open edit mode so that user can name the preset
	wdg::editItemAtIndex( ui->modListView, insertIdx );

	scheduleSavingOptions();
}

void MainWindow::modToggleIcons()
{
	modSettings.showIcons = modModel.areIconsEnabled();

	scheduleSavingOptions();
}

void MainWindow::onModsDropped( int dropRow, int count )
{
	// update the preset
	if (Preset * selectedPreset = getSelectedPreset())
	{
		selectedPreset->mods = modModel.list();  // not the most optimal way, but the size of the list will be always small
	}

	// if these files were dragged here from the map pack list, deselect them there
	QDir mapRootDir = mapModel.rootDirectory();
	for (int row = dropRow; row < dropRow + count; ++row)
	{
		const Mod & mod = modModel[ row ];
		if (fs::isInsideDir( mod.path, mapRootDir ))
		{
			QModelIndex mapPackIdx = mapModel.index( mod.path );
			if (mapPackIdx.isValid())
			{
				wdg::deselectItemByIndex( ui->mapDirView, mapPackIdx );
			}
		}
	}

	scheduleSavingOptions();
	updateLaunchCommand();
}


//----------------------------------------------------------------------------------------------------------------------
//  launch mode

void MainWindow::onModeChosen_Default()
{
	/*bool storageModified =*/ STORE_LAUNCH_OPTION( mode, Default );

	ui->mapCmbBox->setEnabled( false );
	ui->saveFileCmbBox->setEnabled( false );
	ui->mapCmbBox_demo->setEnabled( false );
	ui->demoFileLine_record->setEnabled( false );
	ui->demoFileCmbBox_replay->setEnabled( false );

	toggleSkillSubwidgets( false );
	toggleOptionsSubwidgets( !ui->multiplayerGrpBox->isChecked() );

	if (ui->multiplayerGrpBox->isChecked() && ui->multRoleCmbBox->currentIndex() == Server)
	{
		ui->multRoleCmbBox->setCurrentIndex( Client );   // only client can use default launch mode
	}

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onModeChosen_LaunchMap()
{
	/*bool storageModified =*/ STORE_LAUNCH_OPTION( mode, LaunchMap );

	ui->mapCmbBox->setEnabled( true );
	ui->saveFileCmbBox->setEnabled( false );
	ui->mapCmbBox_demo->setEnabled( false );
	ui->demoFileLine_record->setEnabled( false );
	ui->demoFileCmbBox_replay->setEnabled( false );

	toggleSkillSubwidgets( true );
	toggleOptionsSubwidgets( true );

	if (ui->multiplayerGrpBox->isChecked() && ui->multRoleCmbBox->currentIndex() == Client)
	{
		ui->multRoleCmbBox->setCurrentIndex( Server );   // only server can select a map
	}

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onModeChosen_SavedGame()
{
	/*bool storageModified =*/ STORE_LAUNCH_OPTION( mode, LoadSave );

	ui->mapCmbBox->setEnabled( false );
	ui->saveFileCmbBox->setEnabled( true );
	ui->mapCmbBox_demo->setEnabled( false );
	ui->demoFileLine_record->setEnabled( false );
	ui->demoFileCmbBox_replay->setEnabled( false );

	toggleSkillSubwidgets( false );
	toggleOptionsSubwidgets( false );

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onModeChosen_RecordDemo()
{
	/*bool storageModified =*/ STORE_LAUNCH_OPTION( mode, RecordDemo );

	ui->mapCmbBox->setEnabled( false );
	ui->saveFileCmbBox->setEnabled( false );
	ui->mapCmbBox_demo->setEnabled( true );
	ui->demoFileLine_record->setEnabled( true );
	ui->demoFileCmbBox_replay->setEnabled( false );

	toggleSkillSubwidgets( true );
	toggleOptionsSubwidgets( true );

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onModeChosen_ReplayDemo()
{
	/*bool storageModified =*/ STORE_LAUNCH_OPTION( mode, ReplayDemo );

	ui->mapCmbBox->setEnabled( false );
	ui->saveFileCmbBox->setEnabled( false );
	ui->mapCmbBox_demo->setEnabled( false );
	ui->demoFileLine_record->setEnabled( false );
	ui->demoFileCmbBox_replay->setEnabled( true );

	toggleSkillSubwidgets( false );
	toggleOptionsSubwidgets( false );

	ui->multiplayerGrpBox->setChecked( false );   // no multiplayer when replaying demo

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::toggleSkillSubwidgets( bool enabled )
{
	// skillIdx is an index in the combo-box, which starts from 0, but Doom skill numbers actually start from 1
	int skillIdx = ui->skillCmbBox->currentIndex() + 1;

	ui->skillCmbBox->setEnabled( enabled );
	ui->skillSpinBox->setEnabled( enabled && skillIdx == Skill::Custom );
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

void MainWindow::onMapChanged( const QString & mapName )
{
	if (disableSelectionCallbacks)
		return;

	/*bool storageModified =*/ STORE_LAUNCH_OPTION( mapName, mapName );

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onMapChanged_demo( const QString & mapName )
{
	if (disableSelectionCallbacks)
		return;

	/*bool storageModified =*/ STORE_LAUNCH_OPTION( mapName_demo, mapName );

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onSavedGameSelected( int saveIdx )
{
	if (disableSelectionCallbacks)
		return;

	const QString & saveFileName = saveIdx >= 0 ? saveModel[ saveIdx ].fileName : emptyString;

	/*bool storageModified =*/ STORE_LAUNCH_OPTION( saveFile, saveFileName );

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onDemoFileChanged_record( const QString & fileName )
{
	if (disableSelectionCallbacks)
		return;

	/*bool storageModified =*/ STORE_LAUNCH_OPTION( demoFile_record, fileName );

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onDemoFileSelected_replay( int demoIdx )
{
	if (disableSelectionCallbacks)
		return;

	const QString & demoFileName = demoIdx >= 0 ? demoModel[ demoIdx ].fileName : emptyString;

	/*bool storageModified =*/ STORE_LAUNCH_OPTION( demoFile_replay, demoFileName );

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}


//----------------------------------------------------------------------------------------------------------------------
//  gameplay options

void MainWindow::onSkillSelected( int comboBoxIdx )
{
	// skillIdx is an index in the combo-box which starts from 0, but Doom skill number actually starts from 1
	int skillIdx = comboBoxIdx + 1;

	/*bool storageModified =*/ STORE_GAMEPLAY_OPTION( skillIdx, skillIdx );

	LaunchMode launchMode = getLaunchModeFromUI();

	ui->skillSpinBox->setEnabled( skillIdx == Skill::Custom && (launchMode == LaunchMap || launchMode == RecordDemo) );
	if (skillIdx < Skill::Custom)
		ui->skillSpinBox->setValue( skillIdx );

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onSkillNumChanged( int skillNum )
{
	bool storageModified = STORE_GAMEPLAY_OPTION( skillNum, skillNum );

	scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onNoMonstersToggled( bool checked )
{
	bool storageModified = STORE_GAMEPLAY_OPTION( noMonsters, checked );

	scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onFastMonstersToggled( bool checked )
{
	bool storageModified = STORE_GAMEPLAY_OPTION( fastMonsters, checked );

	scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onMonstersRespawnToggled( bool checked )
{
	bool storageModified = STORE_GAMEPLAY_OPTION( monstersRespawn, checked );

	scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onAllowCheatsToggled( bool checked )
{
	bool storageModified = STORE_GAMEPLAY_OPTION( allowCheats, checked );

	scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}


//----------------------------------------------------------------------------------------------------------------------
//  compatibility

void MainWindow::onCompatLevelSelected( int compatLevel )
{
	if (disableSelectionCallbacks)
		return;

	bool storageModified = STORE_COMPAT_OPTION( compatLevel, compatLevel - 1 );  // first item is reserved for indicating no selection

	scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}


//----------------------------------------------------------------------------------------------------------------------
//  multiplayer

void MainWindow::onMultiplayerToggled( bool checked )
{
	/*bool storageModified =*/ STORE_MULT_OPTION( isMultiplayer, checked );

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
			ui->multRoleCmbBox->setCurrentIndex( Server );
		}
		if (launchMode == Default && multRole == Server)  // server cannot start as default, only client can
		{
			ui->multRoleCmbBox->setCurrentIndex( Client );
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

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onMultRoleSelected( int role )
{
	/*bool storageModified =*/ STORE_MULT_OPTION( multRole, MultRole(role) );

	bool multEnabled = ui->multiplayerGrpBox->isChecked();
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

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onHostChanged( const QString & hostName )
{
	/*bool storageModified =*/ STORE_MULT_OPTION( hostName, hostName );

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onPortChanged( int port )
{
	/*bool storageModified =*/ STORE_MULT_OPTION( port, uint16_t(port) );

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onNetModeSelected( int netMode )
{
	/*bool storageModified =*/ STORE_MULT_OPTION( netMode, NetMode(netMode) );

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onGameModeSelected( int gameMode )
{
	/*bool storageModified =*/ STORE_MULT_OPTION( gameMode, GameMode(gameMode) );

	bool multEnabled = ui->multiplayerGrpBox->isChecked();
	int multRole = ui->multRoleCmbBox->currentIndex();
	bool isDeathMatch = gameMode >= Deathmatch && gameMode <= AltTeamDeathmatch;
	bool isTeamPlay = gameMode == TeamDeathmatch || gameMode == AltTeamDeathmatch || gameMode == Cooperative;

	ui->teamDmgSpinBox->setEnabled( multEnabled && multRole == Server && isTeamPlay );
	ui->timeLimitSpinBox->setEnabled( multEnabled && multRole == Server && isDeathMatch );
	ui->fragLimitSpinBox->setEnabled( multEnabled && multRole == Server && isDeathMatch );

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onPlayerCountChanged( int count )
{
	/*bool storageModified =*/ STORE_MULT_OPTION( playerCount, uint( count ) );

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onTeamDamageChanged( double damage )
{
 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Wfloat-equal"
	/*bool storageModified =*/ STORE_MULT_OPTION( teamDamage, damage );
 #pragma GCC diagnostic pop

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onTimeLimitChanged( int timeLimit )
{
	/*bool storageModified =*/ STORE_MULT_OPTION( timeLimit, uint(timeLimit) );

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onFragLimitChanged( int fragLimit )
{
	/*bool storageModified =*/ STORE_MULT_OPTION( fragLimit, uint(fragLimit) );

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}


//----------------------------------------------------------------------------------------------------------------------
//  alternative paths

void MainWindow::setAlternativeDirs( const QString & dirName )
{
	QString newSaveDir;
	QString newScreenshotDir;

	const EngineInfo * selectedEngine = getSelectedEngine();
	if (selectedEngine)
	{
		// the paths in saveDirLine and screenshotDirLine are relative to the engine's data dir by convention,
		// no need for prepending them with engine data dir.

		newSaveDir = dirName;
		// Do not set screenshot_dir for engines that don't support it,
		// some of them are bitchy and won't start if you supply them with unknown command line parameter.
		if (selectedEngine->hasScreenshotDirParam())
			newScreenshotDir = dirName;
	}

	// Don't clear the lines first and then set new text, instead do it in one call and one callback.
	ui->saveDirLine->setText( newSaveDir );
	ui->screenshotDirLine->setText( newScreenshotDir );
}

void MainWindow::onUsePresetNameToggled( bool checked )
{
	bool storageModified = STORE_GLOBAL_OPTION( usePresetNameAsDir, checked );

	ui->saveDirLine->setEnabled( !checked );
	ui->saveDirBtn->setEnabled( !checked );
	ui->screenshotDirLine->setEnabled( !checked );
	ui->screenshotDirBtn->setEnabled( !checked );

	if (checked)
	{
		const Preset * selectedPreset = getSelectedPreset();
		const QString & presetName = selectedPreset ? selectedPreset->name : emptyString;

		// overwrite whatever is currently in the preset or global launch options
		setAlternativeDirs( fs::sanitizePath( presetName ) );
	}

	scheduleSavingOptions( storageModified );
}

void MainWindow::onSaveDirChanged( const QString & rebasedDir )
{
	// the path in saveDirLine is relative to the engine's data dir by convention, need to rebase it to the current working dir
	QString trueDirPath = engineDataDirRebaser.rebasePathBack( rebasedDir );

	bool storageModified = false;
	if (globalOpts.usePresetNameAsDir)
	{
		// dir is being chosen automatically by the launcher, delete user overrides
		storageModified = STORE_TO_CURRENT_PRESET( altPaths.saveDir, QString() );
	}
	else
	{
		// dir is being edited manually by the user, store his value
		storageModified = STORE_ALT_PATH( saveDir, rebasedDir );
	}

	highlightDirPathIfFileOrCanBeCreated( ui->saveDirLine, trueDirPath );  // non-existing dir is ok becase it will be created automatically

	updateSaveFilesFromDir();

	scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onScreenshotDirChanged( const QString & rebasedDir )
{
	// the path in screenshotDirLine is relative to the engine's data dir by convention, need to rebase it to the current working dir
	QString trueDirPath = engineDataDirRebaser.rebasePathBack( rebasedDir );

	bool storageModified = false;
	if (globalOpts.usePresetNameAsDir)
	{
		// dir is being chosen automatically by the launcher, delete user overrides
		storageModified = STORE_TO_CURRENT_PRESET( altPaths.screenshotDir, QString() );
	}
	else
	{
		// dir is being edited manually by the user, store his value
		storageModified = STORE_ALT_PATH( screenshotDir, rebasedDir );
	}

	highlightDirPathIfFileOrCanBeCreated( ui->screenshotDirLine, trueDirPath );  // non-existing dir is ok becase it will be created automatically

	scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::browseSaveDir()
{
	QString currentSaveDir = getSaveDir();

	QString newSaveDir = DialogWithPaths::browseDir( this, "with saves", currentSaveDir );
	if (newSaveDir.isEmpty())  // user probably clicked cancel
		return;

	// the path in saveDirLine is relative to the engine's data dir by convention,
	// but the path from the file dialog is relative to the current working dir -> need to rebase it
	ui->saveDirLine->setText( engineDataDirRebaser.rebasePath( newSaveDir ) );
}

void MainWindow::browseScreenshotDir()
{
	QString currentScreenshotDir = getScreenshotDir();

	QString newScreenshotDir = DialogWithPaths::browseDir( this, "for screenshots", currentScreenshotDir );
	if (newScreenshotDir.isEmpty())  // user probably clicked cancel
		return;

	// the path in screenshotDirLine is relative to the engine's data dir by convention,
	// but the path from the file dialog is relative to the current working dir -> need to rebase it
	ui->screenshotDirLine->setText( engineDataDirRebaser.rebasePath( newScreenshotDir ) );
}


//----------------------------------------------------------------------------------------------------------------------
//  video options

void MainWindow::onMonitorSelected( int index )
{
	/*bool storageModified =*/ STORE_VIDEO_OPTION( monitorIdx, index );

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onResolutionXChanged( const QString & xStr )
{
	bool storageModified = STORE_VIDEO_OPTION( resolutionX, xStr.toUInt() );

	scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onResolutionYChanged( const QString & yStr )
{
	bool storageModified = STORE_VIDEO_OPTION( resolutionY, yStr.toUInt() );

	scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onShowFpsToggled( bool checked )
{
	bool storageModified = STORE_VIDEO_OPTION( showFPS, checked );

	scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}


//----------------------------------------------------------------------------------------------------------------------
//  audio options

void MainWindow::onNoSoundToggled( bool checked )
{
	/*bool storageModified =*/ STORE_AUDIO_OPTION( noSound, checked );

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onNoSFXToggled( bool checked )
{
	/*bool storageModified =*/ STORE_AUDIO_OPTION( noSFX, checked );

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onNoMusicToggled( bool checked )
{
	/*bool storageModified =*/ STORE_AUDIO_OPTION( noMusic, checked );

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}


//----------------------------------------------------------------------------------------------------------------------
//  environment variables

void MainWindow::presetEnvVarAdd()
{
	disableEnvVarsCallbacks = true;
	int appendedIdx = wdg::appendRow( ui->presetEnvVarTable );
	disableEnvVarsCallbacks = false;

	// also add it to the preset
	Preset * preset = getSelectedPreset();
	if (preset)
	{
		preset->envVars.append( os::EnvVar{} );
	}

	// open edit mode so that user can start writing the variable name
	wdg::editCellAtIndex( ui->presetEnvVarTable, appendedIdx, VarNameColumn );

	//scheduleSavingOptions();
}

void MainWindow::presetEnvVarDelete()
{
	disableEnvVarsCallbacks = true;
	int deletedIdx = wdg::deleteSelectedRow( ui->presetEnvVarTable );
	disableEnvVarsCallbacks = false;
	if (deletedIdx < 0)  // no item was selected
		return;

	// also delete it from the preset
	Preset * preset = getSelectedPreset();
	if (preset)
	{
		preset->envVars.removeAt( deletedIdx );
	}

	scheduleSavingOptions();
}

void MainWindow::globalEnvVarAdd()
{
	disableEnvVarsCallbacks = true;
	int appendedIdx = wdg::appendRow( ui->globalEnvVarTable );
	disableEnvVarsCallbacks = false;

	// also add it to the global options
	globalOpts.envVars.append( os::EnvVar{} );

	// open edit mode so that user can start writing the variable name
	wdg::editCellAtIndex( ui->globalEnvVarTable, appendedIdx, VarNameColumn );

	//scheduleSavingOptions();
}

void MainWindow::globalEnvVarDelete()
{
	disableEnvVarsCallbacks = true;
	int deletedIdx = wdg::deleteSelectedRow( ui->globalEnvVarTable );
	disableEnvVarsCallbacks = false;
	if (deletedIdx < 0)  // no item was selected
		return;

	// also delete it from the global options
	globalOpts.envVars.removeAt( deletedIdx );

	scheduleSavingOptions();
}

static void swapEnvVarTableRows( QTableWidget * table, int row1, int row2 )
{
	QTableWidgetItem * nameItem1  = table->takeItem( row1, VarNameColumn );
	QTableWidgetItem * valueItem1 = table->takeItem( row1, VarValueColumn );
	QTableWidgetItem * nameItem2  = table->takeItem( row2, VarNameColumn );
	QTableWidgetItem * valueItem2 = table->takeItem( row2, VarValueColumn );
	table->setItem( row1, VarNameColumn,  nameItem2 );
	table->setItem( row1, VarValueColumn, valueItem2 );
	table->setItem( row2, VarNameColumn,  nameItem1 );
	table->setItem( row2, VarValueColumn, valueItem1 );
}

void MainWindow::moveEnvVarToKeepTableSorted( QTableWidget * table, EnvVars * envVars, int rowIdx )
{
	QString newVarName = table->item( rowIdx, VarNameColumn )->text();

	// This must be done, otherwise table->takeItem() / table->setItem() will call onPresetEnvVarDataChanged(),
	// and this function will be recursively called again.
	disableEnvVarsCallbacks = true;

	wdg::deselectAllAndUnsetCurrentRow( table );

	while (rowIdx > 0 && newVarName < table->item( rowIdx - 1, VarNameColumn )->text())
	{
		swapEnvVarTableRows( table, rowIdx, rowIdx - 1 );
		if (envVars)
			std::swap( (*envVars)[ rowIdx ], (*envVars)[ rowIdx - 1 ] );
		rowIdx -= 1;
	}
	while (rowIdx < table->rowCount() - 1 && newVarName > table->item( rowIdx + 1, VarNameColumn )->text())
	{
		swapEnvVarTableRows( table, rowIdx, rowIdx + 1 );
		if (envVars)
			std::swap( (*envVars)[ rowIdx ], (*envVars)[ rowIdx + 1 ] );
		rowIdx += 1;
	}

	wdg::selectAndSetCurrentRowByIndex( table, rowIdx );

	disableEnvVarsCallbacks = false;
}

void MainWindow::onPresetEnvVarDataChanged( int row, int column )
{
	if (disableEnvVarsCallbacks)
		return;

	Preset * preset = getSelectedPreset();
	if (preset)
	{
		if (column == VarNameColumn)
			preset->envVars[ row ].name = ui->presetEnvVarTable->item( row, column )->text();
		else if (column == VarValueColumn)
			preset->envVars[ row ].value = ui->presetEnvVarTable->item( row, column )->text();
	}

	if (column == VarNameColumn)
	{
		// the sort key has changed, move the entry so that the table remains sorted
		moveEnvVarToKeepTableSorted( ui->presetEnvVarTable, preset ? &preset->envVars : nullptr, row );
	}
}

void MainWindow::onGlobalEnvVarDataChanged( int row, int column )
{
	if (disableEnvVarsCallbacks)
		return;

	if (column == VarNameColumn)
		globalOpts.envVars[ row ].name = ui->globalEnvVarTable->item( row, column )->text();
	else if (column == VarValueColumn)
		globalOpts.envVars[ row ].value = ui->globalEnvVarTable->item( row, column )->text();

	if (column == VarNameColumn)
	{
		// the sort key has changed, move the entry so that the table remains sorted
		moveEnvVarToKeepTableSorted( ui->globalEnvVarTable, &globalOpts.envVars, row );
	}
}


//----------------------------------------------------------------------------------------------------------------------
//  additional command line arguments

void MainWindow::onPresetCmdArgsChanged( const QString & text )
{
	/*bool storageModified =*/ STORE_PRESET_OPTION( cmdArgs, text );

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}

void MainWindow::onGlobalCmdArgsChanged( const QString & text )
{
	/*bool storageModified =*/ STORE_GLOBAL_OPTION( cmdArgs, text );

	//scheduleSavingOptions( storageModified );
	updateLaunchCommand();
}


//----------------------------------------------------------------------------------------------------------------------
//  misc

void MainWindow::togglePathStyle( PathStyle style )
{
	bool styleChanged = style != pathConvertor.pathStyle();

	pathConvertor.setPathStyle( style );

	for (Engine & engine : engineModel)
	{
		engine.executablePath = pathConvertor.convertPath( engine.executablePath );
		engine.configDir = pathConvertor.convertPath( engine.configDir );
	}

	iwadSettings.dir = pathConvertor.convertPath( iwadSettings.dir );
	for (IWAD & iwad : iwadModel)
	{
		iwad.path = pathConvertor.convertPath( iwad.path );
	}

	mapSettings.dir = pathConvertor.convertPath( mapSettings.dir );
	mapModel.setRootPath( mapSettings.dir );

	modSettings.dir = pathConvertor.convertPath( modSettings.dir );
	for (Mod & mod : modModel)
	{
		mod.path = pathConvertor.convertPath( mod.path );
	}

	for (Preset & preset : presetModel)
	{
		preset.selectedEnginePath = pathConvertor.convertPath( preset.selectedEnginePath );
		preset.selectedIWAD = pathConvertor.convertPath( preset.selectedIWAD );
		for (QString & selectedMapPack : preset.selectedMapPacks)
		{
			selectedMapPack = pathConvertor.convertPath( selectedMapPack );
		}
		for (Mod & mod : preset.mods)
		{
			mod.path = pathConvertor.convertPath( mod.path );
		}
	}

	ui->saveDirLine->setText( convertRebasedEngineDataPath( ui->saveDirLine->text() ) );
	ui->screenshotDirLine->setText( convertRebasedEngineDataPath( ui->screenshotDirLine->text() ) );

	scheduleSavingOptions( styleChanged );
}

void MainWindow::fillDerivedEngineInfo( DirectList< EngineInfo > & engines )
{
	for (EngineInfo & engine : engines)
	{
		if (!engine.hasAppInfo())
			engine.loadAppInfo( engine.executablePath );
		if (!engine.hasFamilyTraits())
			engine.assignFamilyTraits( engine.family );
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
	// workaround (read the big comment above)
	int origIwadIdx = wdg::getSelectedItemIndex( ui->iwadListView );
	disableSelectionCallbacks = true;

	wdg::updateListFromDir( iwadModel, ui->iwadListView, iwadSettings.dir, iwadSettings.searchSubdirs, pathConvertor, doom::isIWAD );

	if (!iwadSettings.defaultIWAD.isEmpty())
	{
		// the default item marking was lost during the update, mark it again
		int defaultIdx = findSuch( iwadModel, [&]( const IWAD & i ){ return i.getID() == iwadSettings.defaultIWAD; } );
		if (defaultIdx >= 0)
			markItemAsDefault( iwadModel[ defaultIdx ] );
	}

	disableSelectionCallbacks = false;
	int newIwadIdx = wdg::getSelectedItemIndex( ui->iwadListView );

	if (newIwadIdx != origIwadIdx)
	{
		// selection changed while the callbacks were disabled, we need to call them manually
		onIWADToggled( QItemSelection(), QItemSelection()/*TODO*/ );
	}
}

/** NOTE: The content of the model is updated asynchronously (in a separate thread)
  * so it will most likely not be ready yet when this function returns. */
void MainWindow::resetMapDirModelAndView()
{
	// We use QFileSystemModel which updates the data from directory automatically.
	// But when the directory is changed, the model and view needs to be reset.
	QModelIndex newRootIdx = mapModel.setRootPath( mapSettings.dir );
	ui->mapDirView->setRootIndex( newRootIdx );
}

void MainWindow::updateConfigFilesFromDir( const QString * callersConfigDir )
{
	QString configDir = callersConfigDir ? *callersConfigDir : getConfigDir();

	// workaround (read the big comment above)
	int origConfigIdx = ui->configCmbBox->currentIndex();
	disableSelectionCallbacks = true;

	wdg::updateComboBoxFromDir( configModel, ui->configCmbBox, configDir, /*recursively*/false, /*emptyItem*/true, pathConvertor,
		/*isDesiredFile*/[]( const QFileInfo & file ) { return doom::configFileSuffixes.contains( file.suffix().toLower() ); }
	);

	disableSelectionCallbacks = false;
	int newConfigIdx = ui->configCmbBox->currentIndex();

	if (newConfigIdx != origConfigIdx)
	{
		// selection changed while the callbacks were disabled, we need to call them manually
		onConfigSelected( newConfigIdx );
	}
}

void MainWindow::updateSaveFilesFromDir( const QString * callersSaveDir )
{
	QString saveDir = callersSaveDir ? *callersSaveDir : getSaveDir();

	// workaround (read the big comment above)
	int origSaveIdx = ui->saveFileCmbBox->currentIndex();
	disableSelectionCallbacks = true;

	wdg::updateComboBoxFromDir( saveModel, ui->saveFileCmbBox, saveDir, /*recursively*/false, /*emptyItem*/false, pathConvertor,
		/*isDesiredFile*/[]( const QFileInfo & file ) { return file.suffix().toLower() == doom::saveFileSuffix; }
	);

	disableSelectionCallbacks = false;
	int newSaveIdx = ui->saveFileCmbBox->currentIndex();

	if (newSaveIdx != origSaveIdx)
	{
		// selection changed while the callbacks were disabled, we need to call them manually
		onSavedGameSelected( newSaveIdx );
	}
}

void MainWindow::updateDemoFilesFromDir( const QString * callersDemoDir )
{
	QString demoDir = callersDemoDir ? *callersDemoDir : getDemoDir();

	// workaround (read the big comment above)
	int origDemoIdx = ui->demoFileCmbBox_replay->currentIndex();
	disableSelectionCallbacks = true;

	wdg::updateComboBoxFromDir( demoModel, ui->demoFileCmbBox_replay, demoDir, /*recursively*/false, /*emptyItem*/false, pathConvertor,
		/*isDesiredFile*/[]( const QFileInfo & file ) { return file.suffix().toLower() == doom::demoFileSuffix; }
	);

	disableSelectionCallbacks = false;
	int newDemoIdx = ui->demoFileCmbBox_replay->currentIndex();

	if (newDemoIdx != origDemoIdx)
	{
		// selection changed while the callbacks were disabled, we need to call them manually
		onDemoFileSelected_replay( newDemoIdx );
	}
}

/// Updates the compat level combo-box according to the currently selected engine.
void MainWindow::updateCompatLevels()
{
	const EngineInfo * selectedEngine = getSelectedEngine();
	CompatLevelStyle currentCompLvlStyle = selectedEngine ? selectedEngine->compatLevelStyle() : CompatLevelStyle::None;

	if (currentCompLvlStyle != lastCompLvlStyle)
	{
		disableSelectionCallbacks = true;  // workaround (read the big comment above)

		ui->compatLevelCmbBox->setCurrentIndex( -1 );

		// automatically initialize compat level combox fox according to the selected engine (its compat level options)
		ui->compatLevelCmbBox->clear();
		ui->compatLevelCmbBox->addItem("");  // keep one empty item to allow explicitly deselecting
		if (currentCompLvlStyle != CompatLevelStyle::None)
		{
			ui->compatLevelCmbBox->addItems( getCompatLevels( currentCompLvlStyle ) );
		}

		ui->compatLevelCmbBox->setCurrentIndex( 0 );

		disableSelectionCallbacks = false;

		// available compat levels changed -> reset the compat level index, because the previous one is no longer valid
		onCompatLevelSelected( 0 );

		// keep the widget enabled only if the engine supports compatibility levels
		LaunchMode launchMode = getLaunchModeFromUI();
		ui->compatLevelCmbBox->setEnabled(
			currentCompLvlStyle != CompatLevelStyle::None && launchMode != LoadSave && launchMode != ReplayDemo
		);

		lastCompLvlStyle = currentCompLvlStyle;
	}
}

// this is not called regularly, but only when an IWAD or map WAD is selected or deselected
void MainWindow::updateMapsFromSelectedWADs( const QStringVec * selectedMapPacks )
{
	const IWAD * selectedIWAD = getSelectedIWAD();
	const QString & selectedIwadPath = selectedIWAD ? selectedIWAD->path : emptyString;

	// optimization: it the caller already has them, use his ones instead of getting them again
	QStringVec localSelectedMapPacks;
	if (!selectedMapPacks)
	{
		localSelectedMapPacks = getSelectedMapPacks();
		selectedMapPacks = &localSelectedMapPacks;
	}

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

		if (!selectedIWAD)
		{
			break;  // if no IWAD is selected, let's leave this empty, it cannot be launched anyway
		}

		auto selectedWADs = QStringVec{ selectedIwadPath } + *selectedMapPacks;

		// read the map names from the selected files and merge them so that entries are not duplicated
		auto uniqueMapNames = getUniqueMapNamesFromWADs( selectedWADs );

		// fill the combox-box
		if (!uniqueMapNames.isEmpty())
		{
			auto mapNames = uniqueMapNames;
			ui->mapCmbBox->addItems( mapNames );
			ui->mapCmbBox_demo->addItems( mapNames );
		}
		else  // if we haven't found any map names in the WADs, fallback to the standard names based on IWAD name
		{
			auto mapNames = doom::getStandardMapNames( fs::getFileNameFromPath( selectedIwadPath ) );
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
		onMapChanged( ui->mapCmbBox->currentText() );
	}
	if (ui->mapCmbBox_demo->currentText() != origText_demo)
	{
		// selection changed while the callbacks were disabled, we need to call them manually
		onMapChanged_demo( ui->mapCmbBox->currentText() );
	}
}


//----------------------------------------------------------------------------------------------------------------------
//  command export

void MainWindow::exportPresetToScript()
{
	if (!wdg::isSomethingSelected( ui->presetListView ))
	{
		reportUserError( this, "No preset selected", "Select a preset from the preset list." );
		return;
	}

	QString scriptFilePath = OwnFileDialog::getSaveFileName( this, "Export preset", lastUsedDir, scriptFileSuffix );
	if (scriptFilePath.isEmpty())  // user probably clicked cancel
	{
		return;
	}

	QFileInfo scriptFileInfo( scriptFilePath );
	QString scriptDir = scriptFileInfo.path();

	PathStyle cmdPathStyle = pathConvertor.pathStyle();

	// Both the executable and the paths in the arguments need to be relative to the directory of the script file,
	// because the script will set the working directory to it's own directory.
	auto cmd = generateLaunchCommand(
		scriptDir, cmdPathStyle, scriptDir, cmdPathStyle, QuotePaths, DontVerifyPaths
	);

	lastUsedDir = std::move(scriptDir);

	QFile scriptFile( scriptFilePath );
	if (!scriptFile.open( QIODevice::WriteOnly | QIODevice::Text ))
	{
		reportRuntimeError( this, "Cannot open file", "Cannot open file for writing ("%scriptFile.errorString()%")" );
		return;
	}

	QTextStream stream( &scriptFile );

	// make sure the working directory is set to the script file's directory
 #if IS_WINDOWS
	stream << "cd \"%~dp0\"\n";
 #else
	stream << "#!/bin/bash\n\n";
	stream << "cd $(dirname $(readlink -f \"$0\"))\n";
 #endif

	stream << cmd.executable << " " << cmd.arguments.join(' ') << '\n';

	scriptFile.close();
}

void MainWindow::exportPresetToShortcut()
{
 #if IS_WINDOWS

	const Preset * selectedPreset = getSelectedPreset();
	if (!selectedPreset)
	{
		reportUserError( this, "No preset selected", "Select a preset from the preset list." );
		return;
	}

	const EngineInfo * selectedEngine = getSelectedEngine();
	if (!selectedEngine)
	{
		reportUserError( this, "No engine selected", "No Doom engine is selected." );
		return;  // no sense to generate a command when we don't even know the engine
	}

	QString shortcutPath = OwnFileDialog::getSaveFileName( this, "Export preset", lastUsedDir, shortcutFileSuffix );
	if (shortcutPath.isEmpty())  // user probably clicked cancel
	{
		return;
	}

	lastUsedDir = fs::getDirOfFile( shortcutPath );

	QString currentWorkingDir = pathConvertor.workingDir().path();
	QString engineWorkingDir = fs::getAbsoluteDirOfFile( selectedEngine->executablePath );

	// - The paths in the arguments need to be relative to the engine's directory
	//   because the engine will be executed with the working directory set to engine's directory,
	// - But the executable itself must be either absolute or relative to the current working dir
	//   so that it is correctly saved to the shortcut.
	// - Paths need to be quoted because Windows accepts a single string with all arguments concatenated instead of a list.
	auto cmd = generateLaunchCommand(
		currentWorkingDir, PathStyle::Absolute, engineWorkingDir, pathConvertor.pathStyle(), QuotePaths, DontVerifyPaths
	);

	bool success = win::createShortcut( shortcutPath, cmd.executable, cmd.arguments, engineWorkingDir, selectedPreset->name );
	if (!success)
	{
		reportRuntimeError( this, "Cannot create shortcut", "Failed to create a shortcut. Check errors.txt for details." );
		return;
	}

 #else

	reportUserError( this, "Not supported", "This feature only works on Windows." );

 #endif
}

void MainWindow::importPresetFromScript()
{
	reportUserError( this, "Not implemented", "Sorry, this feature is not implemented yet." );
}


//----------------------------------------------------------------------------------------------------------------------
//  launch command generation

void MainWindow::updateLaunchCommand()
{
	// optimization: don't regenerate the command when we're about to make more changes right away
	if (restoringOptionsInProgress || restoringPresetInProgress)
		return;

	//static uint callCnt = 1;
	//logDebug() << "updateLaunchCommand() " << callCnt++;

	const EngineInfo * selectedEngine = getSelectedEngine();
	if (!selectedEngine)
	{
		ui->commandLine->clear();
		return;  // no sense to generate a command when we don't even know the engine
	}

	QString currentCommand = ui->commandLine->text();

	QString engineDir = fs::getDirOfFile( selectedEngine->executablePath );

	PathStyle cmdPathStyle = pathConvertor.pathStyle();

	// The paths in the arguments need to be relative to the engine's directory,
	// because the engine will be executed with the working directory set to engine's directory.
	// The relative path of the executable does not matter, because here it is for displaying only.
	auto cmd = generateLaunchCommand(
		engineDir, cmdPathStyle, engineDir, cmdPathStyle, QuotePaths, DontVerifyPaths
	);

	QString newCommand = cmd.executable % ' ' % cmd.arguments.join(' ');

	// Don't replace the line widget's content if there is no change. It would just annoy a user who is trying to select
	// and copy part of the line, by constantly reseting his selection.
	if (newCommand != currentCommand)
	{
		//static int updateCnt = 1;
		//logDebug() << "    updating " << updateCnt++;
		ui->commandLine->setText( newCommand );
	}
}

/// Generates a command to be run, displayed or saved to a script file, according to the specified options.
/**
  * \param parentWorkingDir Working directory when the command is executed by the parent process.
  *                         This will determine the relative path of the engine executable in the command.
  * \param engineWorkingDir Working directory for the engine process that will be started.
  *                         This will determine the relative paths of the file or directory arguments passed to the engine.
  * \param enginePathStyle Path style to be used for the engine executable.
  * \param argPathStyle Path style to be used for the paths in the command line arguments.
  * \param quotePaths Surround each path in the command with quotes.
  *                   Required for displaying the command or saving it to a script file.
  * \param verifyPaths Verify that each path in the command is valid and leads to the correct entry type (file or directory).
  *                    If invalid path is found, display a message box with an error description.
  */
os::ShellCommand MainWindow::generateLaunchCommand(
	const QString & parentWorkingDir, PathStyle enginePathStyle, const QString & engineWorkingDir, PathStyle argPathStyle,
	bool quotePaths, bool verifyPaths
){
	os::ShellCommand cmd;

	// The stored engine path is relative to DoomRunner's directory, but we need it relative to parentWorkingDir.
	PathRebaser parentDirRebaser( pathConvertor.workingDir(), parentWorkingDir, enginePathStyle, quotePaths );
	// All stored paths are relative to DoomRunner's directory, but we need them relative to engineWorkingDir.
	PathRebaser engineDirRebaser( pathConvertor.workingDir(), engineWorkingDir, argPathStyle, quotePaths );

	PathChecker p( this, verifyPaths );

	//-- engine --------------------------------------------------------------------

	const EngineInfo * selectedEngine = getSelectedEngine();
	if (!selectedEngine)
	{
		return {};  // no point in generating a command if we don't even know the engine, it determines everything
	}

	const EngineInfo & engine = *selectedEngine;  // non-const so that we can change color of invalid paths

	{
		p.checkItemFilePath( engine, "the selected engine", "Please update its path in Menu -> Initial Setup, or select another one." );

		// get the beginning of the launch command based on OS and installation type
		cmd = os::getRunCommand( engine.executablePath, parentDirRebaser, getDirsToBeAccessed() );
	}

	//-- engine's config -----------------------------------------------------------

	if (const ConfigFile * selectedConfig = getSelectedConfig())
	{
		// at this point the configDir cannot be empty, otherwise the configCmbBox would be empty and there would not be any selected config
		QString configPath = fs::getPathFromFileName( engine.configDir, selectedConfig->fileName );

		p.checkFilePath( configPath, "the selected config", "Please update the config dir in Menu -> Initial Setup, or select another one." );
		cmd.arguments << "-config" << engineDirRebaser.rebaseAndQuotePath( configPath );
	}

	//-- game data files -----------------------------------------------------------

	// IWAD
	if (const IWAD * selectedIWAD = getSelectedIWAD())
	{
		p.checkItemFilePath( *selectedIWAD, "selected IWAD", "Please select another one." );
		cmd.arguments << "-iwad" << engineDirRebaser.rebaseAndQuotePath( selectedIWAD->path );
	}

	// This part is tricky.
	// Older engines only accept single -file parameter, so all the regular map/mod files must be listed together.
	// But the user is allowed to intersperse the regular files with deh/bex files or custom cmd arguments.
	// So we must somehow build an ordered sequence of mod files and custom arguments in which all the regular files are
	// grouped together, and the easiest option seems to be by using a placeholder item.

	QStringVec modArguments;
	QStringVec fileList;

	auto addFileAccordingToSuffix = [&]( const QString & filePath )
	{
		QString suffix = QFileInfo( filePath ).suffix().toLower();
		if (suffix == "deh" || suffix == "hhe") {
			modArguments << "-deh" << engineDirRebaser.rebaseAndQuotePath( filePath );
		} else if (suffix == "bex") {
			modArguments << "-bex" << engineDirRebaser.rebaseAndQuotePath( filePath );
		} else {
			if (fileList.isEmpty())
				modArguments << "-file" << "<file_list>";  // insert placeholder where all the files will be together
			fileList.append( engineDirRebaser.rebaseAndQuotePath( filePath ) );
		}
	};

	auto appendCustomArguments = [&]( QStringVec & args, const QString & customArgsStr )
	{
		auto splitArgs = splitCommandLineArguments( customArgsStr );
		for (const auto & arg : splitArgs)
		{
			if (quotePaths && arg.wasQuoted)
				args << quoted( arg.str );
			else
				args << arg.str;
		}
	};

	// map files
	forEachSelectedMapPack( [&]( const QString & mapFilePath )
	{
		p.checkAnyPath( mapFilePath, "the selected map pack", "Please select another one." );
		addFileAccordingToSuffix( mapFilePath );
	});

	// mod files
	for (const Mod & mod : modModel)
	{
		if (!mod.isSeparator && mod.checked)
		{
			if (mod.isCmdArg) {  // this is not a file but a custom command line argument
				appendCustomArguments( modArguments, mod.fileName );  // the fileName holds the argument value
			} else {
				p.checkItemAnyPath( mod, "the selected mod", "Please update the mod list." );
				addFileAccordingToSuffix( mod.path );
			}
		}
	}

	// output the final sequence to the cmd.arguments
	for (QString & modArgument : modArguments)
	{
		if (modArgument == "<file_list>") {
			// replace the placeholder with the actual list
			for (QString & filePath : fileList)
				cmd.arguments << std::move(filePath);
		} else {
			cmd.arguments << std::move(modArgument);
		}
	}

	//-- alternative directories ---------------------------------------------------
	// Rather set them before the launch parameters, because some of the parameters
	// (e.g. -loadgame) can be relative to these alternative directories.

	if (!ui->saveDirLine->text().isEmpty())
	{
		QString saveDirPath = getSaveDir();
		p.checkNotAFile( saveDirPath, "the save dir", {} );
		cmd.arguments << engine.saveDirParam() << engineDirRebaser.rebaseAndQuotePath( saveDirPath );
	}
	if (!ui->screenshotDirLine->text().isEmpty())
	{
		QString screenshotDirPath = getScreenshotDir();
		p.checkNotAFile( screenshotDirPath, "the screenshot dir", {} );
		cmd.arguments << "+screenshot_dir" << engineDirRebaser.rebaseAndQuotePath( screenshotDirPath );
	}

	//-- launch mode and parameters ------------------------------------------------
	// Beware that while -record and -playdemo are either absolute or relative to the current working dir
	// -loadgame might need to be relative to -savedir, depending on the engine and its version

	LaunchMode launchMode = getLaunchModeFromUI();
	if (launchMode == LaunchMap)
	{
		cmd.arguments << engine.getMapArgs( ui->mapCmbBox->currentIndex(), ui->mapCmbBox->currentText() );
	}
	else if (launchMode == LoadSave && !ui->saveFileCmbBox->currentText().isEmpty())
	{
		QString saveDir = getSaveDir();  // save dir cannot be empty, otherwise the saveFileCmbBox would be empty
		QString trueSavePath = fs::getPathFromFileName( saveDir, ui->saveFileCmbBox->currentText() );
		p.checkFilePath( trueSavePath, "the selected save file", "Please select another one." );
		cmd.arguments << "-loadgame" << rebaseSaveFilePath( trueSavePath, engineDirRebaser, &engine );
	}
	else if (launchMode == RecordDemo && !ui->demoFileLine_record->text().isEmpty())
	{
		QString demoDir = getDemoDir();  // if demo dir is empty (saveDirLine is empty and engine.configDir is not set), then
		QString demoPath = fs::getPathFromFileName( demoDir, ui->demoFileLine_record->text() );  // the demoFileLine will be used as is
		cmd.arguments << "-record" << engineDirRebaser.rebaseAndQuotePath( demoPath );
		cmd.arguments << engine.getMapArgs( ui->mapCmbBox_demo->currentIndex(), ui->mapCmbBox_demo->currentText() );
	}
	else if (launchMode == ReplayDemo && !ui->demoFileCmbBox_replay->currentText().isEmpty())
	{
		QString demoDir = getDemoDir();  // demo dir cannot be empty, otherwise the demoFileCmbBox_replay would be empty
		QString demoPath = fs::getPathFromFileName( demoDir, ui->demoFileCmbBox_replay->currentText() );
		p.checkFilePath( demoPath, "the selected demo", "Please select another one." );
		cmd.arguments << "-playdemo" << engineDirRebaser.rebaseAndQuotePath( demoPath );
	}

	//-- gameplay and compatibility options ----------------------------------------

	const GameplayOptions & activeGameOpts = activeGameplayOptions();
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

	const CompatibilityOptions & activeCompatOpts = activeCompatOptions();
	if (ui->compatLevelCmbBox->isEnabled() && activeCompatOpts.compatLevel >= 0)
		cmd.arguments << engine.getCompatLevelArgs( activeCompatOpts.compatLevel );
	if (ui->compatOptsBtn->isEnabled() && !compatOptsCmdArgs.isEmpty())
		cmd.arguments << compatOptsCmdArgs;
	if (ui->allowCheatsChkBox->isChecked())
		cmd.arguments << "+sv_cheats" << "1";

	//-- multiplayer options -------------------------------------------------------

	if (ui->multiplayerGrpBox->isChecked())
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
				reportLogicError( this, "Invalid game mode index", "The game mode index is out of range." );
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
			reportLogicError( this, "Invalid multiplayer role index", "The multiplayer role index is out of range." );
		}
	}

	//-- output options ------------------------------------------------------------

	// On Windows ZDoom doesn't log its output to stdout by default.
	// Force it to do so, so that our ProcessOutputWindow displays something.
	if (settings.showEngineOutput && engine.needsStdoutParam())
		cmd.arguments << "-stdout";

	// video options
	if (ui->monitorCmbBox->currentIndex() > 0)
	{
		int monitorIndex = ui->monitorCmbBox->currentIndex() - 1;  // the first item is a placeholder for leaving it default
		cmd.arguments << "+vid_adapter" << engine.getCmdMonitorIndex( monitorIndex );  // some engines index monitors from 1 and others from 0
	}
	if (!ui->resolutionXLine->text().isEmpty())
		cmd.arguments << "-width" << ui->resolutionXLine->text();
	if (!ui->resolutionYLine->text().isEmpty())
		cmd.arguments << "-height" << ui->resolutionYLine->text();
	if (ui->showFpsChkBox->isChecked())
		cmd.arguments << "+vid_fps" << "1";

	// audio options
	if (ui->noSoundChkBox->isChecked())
		cmd.arguments << "-nosound";
	if (ui->noSfxChkBox->isChecked())
		cmd.arguments << "-nosfx";
	if (ui->noMusicChkBox->isChecked())
		cmd.arguments << "-nomusic";

	//-- additional custom command line arguments ----------------------------------

	if (!ui->globalCmdArgsLine->text().isEmpty())
		appendCustomArguments( cmd.arguments, ui->globalCmdArgsLine->text() );

	if (!ui->presetCmdArgsLine->text().isEmpty())
		appendCustomArguments( cmd.arguments, ui->presetCmdArgsLine->text() );

	//------------------------------------------------------------------------------

	return p.gotSomeInvalidPaths() ? os::ShellCommand() : cmd;
}

int MainWindow::askForExtraPermissions( const EngineInfo & selectedEngine, const QStringVec & permissions )
{
	auto engineName = fs::getFileNameFromPath( selectedEngine.executablePath );
	auto sandboxName = selectedEngine.sandboxEnvName();

	QMessageBox messageBox( QMessageBox::Question, "Extra permissions needed",
		engineName%" requires extra permissions to be able to access files outside of its "%sandboxName%" environment. "
		"Are you ok with these permissions to be granted to "%engineName%"?",
		QMessageBox::Yes | QMessageBox::No,
		this
	);

	messageBox.setDetailedText( permissions.join('\n') );
	messageBox.setCheckBox( new QCheckBox( "don't ask again" ) );

	int answer = messageBox.exec();

	settings.askForSandboxPermissions = !messageBox.checkBox()->isChecked();

	return answer;
}

bool MainWindow::startDetached(
	const QString & executable, const QStringVec & arguments, const QString & workingDir, const EnvVars & envVars
){
	QString executableName = fs::getFileNameFromPath( executable );

	QProcess process;

	process.setProgram( executable );
	process.setArguments( arguments.toList() );
	process.setWorkingDirectory( workingDir );

	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	for (const auto & envVar : envVars)
	{
		env.insert( envVar.name, envVar.value );
	}
	process.setProcessEnvironment(env);

	bool success = process.startDetached();
	if (!success)
	{
		reportRuntimeError( this, "Process start error", "Failed to start \""%executableName%"\" ("%process.errorString()%")" );
	}

	return success;
}

void MainWindow::launch()
{
	const EngineInfo * selectedEngine = getSelectedEngine();
	if (!selectedEngine)
	{
		reportUserError( this, "No engine selected", "No Doom engine is selected." );
		return;
	}

	QString currentWorkingDir = pathConvertor.workingDir().path();
	QString engineWorkingDir = fs::getAbsoluteDirOfFile( selectedEngine->executablePath );

	// Re-run the command construction, but display error message and abort when there is invalid path.
	// - The engine must be launched using absolute path, because some engines cannot handle being started
	//   from another directory with relative executable path (looking at you Crispy Doom, fix your shit!).
	// - All paths will be relative to the engine's dir, because working dir will be set to the engine's dir when started.
	// - When sending arguments to the process directly and skipping the shell parsing, the quotes are undesired.
	auto cmd = generateLaunchCommand(
		currentWorkingDir, PathStyle::Absolute, engineWorkingDir, pathConvertor.pathStyle(), DontQuotePaths, VerifyPaths
	);
	if (cmd.executable.isNull())
	{
		return;  // errors are already shown during the generation
	}

	logDebug().quote() << cmd.executable << ' ' << cmd.arguments;

	// If extra permissions are needed to run the engine inside its sandbox environment, better ask the user.
	if (settings.askForSandboxPermissions && !cmd.extraPermissions.isEmpty())
	{
		int answer = askForExtraPermissions( *selectedEngine, cmd.extraPermissions );
		if (answer != QMessageBox::Yes)
		{
			return;
		}
	}

	// Make sure the alternative save dir exists, because engine will not create it if demo file path points there.
	QString saveDirPath = getSaveDir();
	bool saveDirExists = fs::createDirIfDoesntExist( saveDirPath );
	if (!saveDirExists)
	{
		reportRuntimeError( this, "Error creating directory", "Failed to create directory \""%saveDirPath%"\". Check permissions." );
		// we can continue without this directory, it will just not save demos
	}

	// We need to start the process with the working dir set to the engine's dir,
	// because some engines search for their own files in the working dir and would fail if started from elsewhere.
	// The command paths are always generated relative to the engine's dir.
	const QString & processWorkingDir = engineWorkingDir;

	// merge optional environment variables defined globally and defined for this preset
	EnvVars envVars = globalOpts.envVars;
	if (const Preset * preset = getSelectedPreset())
		envVars += preset->envVars;

	if (settings.showEngineOutput)
	{
		ProcessOutputWindow processWindow( this );
		processWindow.runProcess( cmd.executable, cmd.arguments, processWorkingDir, envVars );
		//int resultCode = processWindow.result();
	}
	else
	{
		bool success = startDetached( cmd.executable, cmd.arguments, processWorkingDir, envVars );

		if (success && settings.closeOnLaunch)
		{
			this->close();
		}
	}
}
