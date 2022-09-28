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
#include "NewConfigDialog.hpp"
#include "ProcessOutputWindow.hpp"
#include "GameOptsDialog.hpp"
#include "CompatOptsDialog.hpp"

#include "LangUtils.hpp"
#include "JsonUtils.hpp"
#include "WidgetUtils.hpp"
#include "FileSystemUtils.hpp"
#include "OSUtils.hpp"
#include "DoomUtils.hpp"
#include "EngineProperties.hpp"
#include "UpdateChecker.hpp"
#include "Version.hpp"

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
	static const QString shortcutFileSuffix = "*.lnk";
#else
	static const QString scriptFileSuffix = "*.sh";
#endif

static constexpr char defaultOptionsFile [] = "options.json";

static constexpr bool VerifyPaths = true;
static constexpr bool DontVerifyPaths = false;


//======================================================================================================================
//  local helpers

static QString getOptionsFilePath()
{
	return QDir( getAppDataDir() ).filePath( defaultOptionsFile );
}

static bool verifyPath( const QString & path, const QString & errorMessage )
{
	if (!QFileInfo::exists( path ))
	{
		QMessageBox::warning( nullptr, "File or directory no longer exists", errorMessage.arg( path ) );
		return false;
	}
	return true;
}

class FileNotFound {};

static void throwIfInvalid( bool doVerify, const QString & path, const QString & errorMessage )
{
	if (doVerify)
		if (!verifyPath( path, errorMessage ))
			throw FileNotFound();
}

LaunchOptions & MainWindow::activeLaunchOptions()
{
	if (opts.launchOptsStorage == StoreGlobally)
	{
		return launchOpts;
	}
	else if (opts.launchOptsStorage == StoreToPreset)
	{
		int selectedPresetIdx = getSelectedItemIndex( ui->presetListView );
		if (selectedPresetIdx >= 0)
		{
			return presetModel[ selectedPresetIdx ].launchOpts;
		}
	}

	// We always have to save somewhere, because generateLaunchCommand() is reading gameplay options from it.
	// In case the launchOptsStorage == DontStore, we will just skip serializing it to JSON.
	return launchOpts;
}


//======================================================================================================================
//  MainWindow

MainWindow::MainWindow()
:
	QMainWindow( nullptr ),
	tickCount( 0 ),
	optionsCorrupted( false ),
	restoringInProgress( false ),
	disableSelectionCallbacks( false ),
	lastCompLvlStyle( CompatLevelStyle::None ),
	compatOptsCmdArgs(),
	pathContext( QApplication::applicationDirPath(), useAbsolutePathsByDefault ), // all relative paths will internally be stored relative to the application's dir
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
	)
{
	ui = new Ui::MainWindow;
	ui->setupUi( this );

	this->setWindowTitle( windowTitle() + ' ' + appVersion );

	// setup main menu actions

	connect( ui->setupPathsAction, &QAction::triggered, this, &thisClass::runSetupDialog );
	connect( ui->exportPresetToScriptAction, &QAction::triggered, this, &thisClass::exportPresetToScript );
	connect( ui->exportPresetToShortcutAction, &QAction::triggered, this, &thisClass::exportPresetToShortcut );
	//connect( ui->importPresetAction, &QAction::triggered, this, &thisClass::importPreset );
	connect( ui->aboutAction, &QAction::triggered, this, &thisClass::runAboutDialog );
	connect( ui->exitAction, &QAction::triggered, this, &thisClass::close );

 #ifndef _WIN32  // Windows-only feature
	ui->exportPresetToShortcutAction->setEnabled( false );
 #endif

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

	connect( ui->demoFileLine_record, &QLineEdit::textChanged, this, &thisClass::changeDemoFile_record );

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

	// alternative paths
	connect( ui->usePresetNameChkBox, &QCheckBox::toggled, this, &thisClass::toggleUsePresetName );
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
	presetModel.toggleSeparators( true );
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
	modModel.toggleSeparators( true );
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
	QDir appDataDir( getAppDataDir() );
	if (!appDataDir.exists())
	{
		appDataDir.mkpath(".");
	}

	// try to load last saved state
	QString optionsFilePath = appDataDir.filePath( defaultOptionsFile );
	if (QFileInfo::exists( optionsFilePath ))
	{
		loadOptions( optionsFilePath );
	}
	else  // this is a first run, perform an initial setup
	{
		runSetupDialog();
	}

	// if the presets are empty, add a default one so that users don't complain that they can't enter anything
	if (presetModel.isEmpty())
	{
		// This selects the new item, which invokes the callback, which enables the dependent widgets and calls restorePreset(...)
		appendItem( ui->presetListView, presetModel, { "Default" } );

		// automatically select those essential items (engine, IWAD, ...) that are alone in their list
		// This is done to make it as easy as possible for beginners who just downloaded GZDoom and Brutal Doom
		// and want to start it as easily as possible with no care about all the other features.
		autoselectLoneItems();
	}

	if (opts.checkForUpdates)
	{
		updateChecker.checkForUpdates_async(
			/* result callback */[ this ]( UpdateChecker::Result result, QString /*errorDetail*/, QStringList versionInfo )
			{
				if (result == UpdateChecker::UpdateAvailable)
				{
					opts.checkForUpdates = showUpdateNotification( this, versionInfo, /*checkbox*/true );
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
		if (iwadSettings.updateFromDir)
			updateIWADsFromDir();
		updateConfigFilesFromDir();
		updateSaveFilesFromDir();
		updateDemoFilesFromDir();
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
	AboutDialog dialog( this, opts.checkForUpdates );

	dialog.exec();

	opts.checkForUpdates = dialog.checkForUpdates;
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
		opts
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
		engineModel.updateList( dialog.engineModel.list() );
		iwadModel.updateList( dialog.iwadModel.list() );
		iwadSettings = dialog.iwadSettings;
		mapSettings = dialog.mapSettings;
		modSettings = dialog.modSettings;
		opts = dialog.opts;
		// update all stored paths
		pathContext.toggleAbsolutePaths( opts.useAbsolutePaths );
		toggleAbsolutePaths( opts.useAbsolutePaths );
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

void MainWindow::runGameOptsDialog()
{
	LaunchOptions & activeLaunchOpts = activeLaunchOptions();

	GameOptsDialog dialog( this, activeLaunchOpts.gameOpts );

	int code = dialog.exec();

	// update the data only if user clicked Ok
	if (code == QDialog::Accepted)
	{
		activeLaunchOpts.gameOpts = dialog.gameOpts;
		updateLaunchCommand();
	}
}

void MainWindow::runCompatOptsDialog()
{
	LaunchOptions & activeLaunchOpts = activeLaunchOptions();

	CompatOptsDialog dialog( this, activeLaunchOpts.compatOpts );

	int code = dialog.exec();

	// update the data only if user clicked Ok
	if (code == QDialog::Accepted)
	{
		activeLaunchOpts.compatOpts = dialog.compatOpts;
		// cache the command line args string, so that it doesn't need to be regenerated on every command line update
		compatOptsCmdArgs = CompatOptsDialog::getCmdArgsFromOptions( dialog.compatOpts );
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
//  preset loading

/// Loads the content of a preset into the other widgets.
void MainWindow::restorePreset( int presetIdx )
{
	// Restoring any stored options is tricky.
	// Every change of a selection, like  cmbBox->setCurrentIndex( stored.idx )  or  selectItemByIdx( stored.idx )
	// causes the corresponding callbacks to be called which in turn might cause some other stored value to be changed.
	// For example  ui->engineCmbBox->setCurrentIndex( idx )  calls  selectEngine( idx )  which might indirectly call
	// selectConfig( idx )  which will overwrite the stored selected config and we will then restore a wrong one.
	// The least complicated workaround seems to be simply setting a flag indicating that we are in the middle of
	// restoring saved options, and then prevent storing values when this flag is set.

	restoringInProgress = true;

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
				if (QFileInfo::exists( preset.selectedEnginePath ))
				{
					ui->engineCmbBox->setCurrentIndex( engineIdx );
				}
				else
				{
					QMessageBox::warning( this, "Engine no longer exists",
						"Engine selected for this preset ("%preset.selectedEnginePath%") no longer exists, please update the engines at Menu -> Initial Setup." );
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
				// No sense to verify if this config file exists, the configModel has just been updated during
				// selectEngine( ui->engineCmbBox->currentIndex() ), so there are only existing entries.
				ui->configCmbBox->setCurrentIndex( configIdx );
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
				if (QFileInfo::exists( preset.selectedIWAD ))
				{
					selectItemByIndex( ui->iwadListView, iwadIdx );
				}
				else
				{
					QMessageBox::warning( this, "IWAD no longer exists",
						"IWAD selected for this preset ("%preset.selectedIWAD%") no longer exists, please select another one." );
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

		const QList<QString> mapPacksCopy = preset.selectedMapPacks;
		preset.selectedMapPacks.clear();  // clear the list in the preset and let it repopulate only with valid items
		for (const QString & path : mapPacksCopy)
		{
			QModelIndex mapIdx = mapModel.index( path );
			if (mapIdx.isValid() && isInsideDir( path, mapRootDir ))
			{
				if (QFileInfo::exists( path ))
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
		const QList<Mod> modsCopy = preset.mods;
		preset.mods.clear();  // clear the list in the preset and let it repopulate only with valid items
		modModel.clear();
		for (const Mod & mod : modsCopy)
		{
			if (mod.isSeparator || QFileInfo::exists( mod.path ))
			{
				preset.mods.append( mod );  // put back only items that are valid
				modModel.append( mod );
			}
			else
			{
				QMessageBox::warning( this, "Mod no longer exists",
					"A mod from the preset ("%mod.path%") no longer exists. It will be removed from the list." );
			}
		}
		modModel.finishCompleteUpdate();
	}

	// restore launch options
	if (opts.launchOptsStorage == StoreToPreset)
	{
		restoreLaunchOptions( preset.launchOpts );  // this clears items that are invalid
	}

	// restore additional command line arguments
	ui->presetCmdArgsLine->setText( preset.cmdArgs );

	restoringInProgress = false;

	// if "Use preset name as directory" is enabled, overwrite the preset custom directories with its name
	// do it with restoringInProgress == false to update the current preset with the new overwriten dirs.
	if (globalOpts.usePresetNameAsDir)
	{
		setAltDirsRelativeToConfigs( preset.name );
	}

	updateLaunchCommand();
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

void MainWindow::selectEngine( int index )
{
	if (disableSelectionCallbacks)
		return;

	// update the current preset
	int selectedPresetIdx = getSelectedItemIndex( ui->presetListView );
	if (selectedPresetIdx >= 0 && !restoringInProgress)
	{
		if (index < 0)  // engine combo box was reset to "no engine selected" state
			presetModel[ selectedPresetIdx ].selectedEnginePath.clear();
		else
			presetModel[ selectedPresetIdx ].selectedEnginePath = engineModel[ index ].path;
	}

	MapParamStyle mapParamStyle = MapParamStyle::Warp;

	if (index >= 0)
	{
		const EngineProperties & properties = getEngineProperties( engineModel[ index ].family );
		mapParamStyle = properties.mapParamStyle;

		verifyPath( engineModel[ index ].path, "The selected engine (%1) no longer exists, please update the engines at Menu -> Initial Setup." );
	}

	ui->mapCmbBox->setEditable( mapParamStyle == MapParamStyle::Map );
	ui->mapCmbBox_demo->setEditable( mapParamStyle == MapParamStyle::Map );

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
	if (selectedPresetIdx >= 0 && !restoringInProgress)
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
		validConfigSelected = verifyPath( configPath, "The selected config (%1) no longer exists, please select another one." );
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
	if (selectedPresetIdx >= 0 && !restoringInProgress)
	{
		if (selectedIWADIdx < 0)
			presetModel[ selectedPresetIdx ].selectedIWAD.clear();
		else
			presetModel[ selectedPresetIdx ].selectedIWAD = iwadModel[ selectedIWADIdx ].path;
	}

	if (selectedIWADIdx >= 0)
	{
		verifyPath( iwadModel[ selectedIWADIdx ].path, "The selected IWAD (%1) no longer exists, please select another one." );
	}

	updateMapsFromSelectedWADs();

	updateLaunchCommand();
}

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

void MainWindow::toggleMapPack( const QItemSelection & /*selected*/, const QItemSelection & /*deselected*/ )
{
	if (disableSelectionCallbacks)
		return;

	QStringList selectedMapPacks = getSelectedMapPacks();

	// update the current preset
	int selectedPresetIdx = getSelectedItemIndex( ui->presetListView );
	if (selectedPresetIdx >= 0 && !restoringInProgress)
	{
		presetModel[ selectedPresetIdx ].selectedMapPacks = selectedMapPacks;
	}

	// expand the parent directory nodes that are collapsed
	const auto selectedRows = getSelectedRows( ui->mapDirView );
	for (const QModelIndex & index : selectedRows)
		for (QModelIndex currentIndex = index; currentIndex.isValid(); currentIndex = currentIndex.parent())
			if (!ui->mapDirView->isExpanded( currentIndex ))
				ui->mapDirView->expand( currentIndex );

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
	if (selectedPresetIdx >= 0 && !restoringInProgress)
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

	if (!QFileInfo::exists( mapDescFilePath ))
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
	// and want to start it as easily as possible with no care about all the other features.
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
	QStringList paths = QFileDialog::getOpenFileNames( this, "Locate the mod file", modSettings.dir,
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
	QString path = QFileDialog::getExistingDirectory( this, "Locate the mod directory", modSettings.dir );

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
	restoringInProgress = true;  // prevent modDataChanged() from updating our preset too early and incorrectly

	QVector<int> movedIndexes = moveUpSelectedItems( ui->modListView, modModel );

	restoringInProgress = false;

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
	restoringInProgress = true;  // prevent modDataChanged() from updating our preset too early and incorrectly

	QVector<int> movedIndexes = moveDownSelectedItems( ui->modListView, modModel );

	restoringInProgress = false;

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
//  launch mode

void MainWindow::modeDefault()
{
	if (!restoringInProgress)
		activeLaunchOptions().mode = Default;

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
	if (!restoringInProgress)
		activeLaunchOptions().mode = LaunchMap;

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
	if (!restoringInProgress)
		activeLaunchOptions().mode = LoadSave;

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
	if (!restoringInProgress)
		activeLaunchOptions().mode = RecordDemo;

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
	if (!restoringInProgress)
		activeLaunchOptions().mode = ReplayDemo;

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

	if (!restoringInProgress)
		activeLaunchOptions().mapName = mapName;

	updateLaunchCommand();
}

void MainWindow::changeMap_demo( const QString & mapName )
{
	if (disableSelectionCallbacks)
		return;

	if (!restoringInProgress)
		activeLaunchOptions().mapName_demo = mapName;

	updateLaunchCommand();
}

void MainWindow::selectSavedGame( int saveIdx )
{
	if (disableSelectionCallbacks)
		return;

	if (!restoringInProgress)
	{
		QString saveFileName = saveIdx >= 0 ? saveModel[ saveIdx ].fileName : "";
		activeLaunchOptions().saveFile = saveFileName;
	}

	updateLaunchCommand();
}

void MainWindow::changeDemoFile_record( const QString & fileName )
{
	if (disableSelectionCallbacks)
		return;

	if (!restoringInProgress)
		activeLaunchOptions().demoFile_record = fileName;

	updateLaunchCommand();
}

void MainWindow::selectDemoFile_replay( int demoIdx )
{
	if (disableSelectionCallbacks)
		return;

	if (!restoringInProgress)
	{
		QString demoFileName = demoIdx >= 0 ? demoModel[ demoIdx ].fileName : "";
		activeLaunchOptions().demoFile_replay = demoFileName;
	}

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
	if (!restoringInProgress)
		activeLaunchOptions().skillNum = uint( skillNum );

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
	if (!restoringInProgress)
		activeLaunchOptions().noMonsters = checked;

	updateLaunchCommand();
}

void MainWindow::toggleFastMonsters( bool checked )
{
	if (!restoringInProgress)
		activeLaunchOptions().fastMonsters = checked;

	updateLaunchCommand();
}

void MainWindow::toggleMonstersRespawn( bool checked )
{
	if (!restoringInProgress)
		activeLaunchOptions().monstersRespawn = checked;

	updateLaunchCommand();
}

void MainWindow::selectCompatLevel( int compatLevel )
{
	if (!restoringInProgress)
		activeLaunchOptions().compatLevel = compatLevel - 1;  // first item is reserved for indicating no selection

	updateLaunchCommand();
}

void MainWindow::toggleAllowCheats( bool checked )
{
	if (!restoringInProgress)
		activeLaunchOptions().allowCheats = checked;

	updateLaunchCommand();
}


//----------------------------------------------------------------------------------------------------------------------
//  multiplayer

void MainWindow::toggleMultiplayer( bool checked )
{
	if (!restoringInProgress)
		activeLaunchOptions().isMultiplayer = checked;

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
	if (!restoringInProgress)
		activeLaunchOptions().multRole = MultRole( role );

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
	if (!restoringInProgress)
		activeLaunchOptions().hostName = hostName;

	updateLaunchCommand();
}

void MainWindow::changePort( int port )
{
	if (!restoringInProgress)
		activeLaunchOptions().port = uint16_t( port );

	updateLaunchCommand();
}

void MainWindow::selectNetMode( int netMode )
{
	if (!restoringInProgress)
		activeLaunchOptions().netMode = NetMode( netMode );

	updateLaunchCommand();
}

void MainWindow::selectGameMode( int gameMode )
{
	if (!restoringInProgress)
		activeLaunchOptions().gameMode = GameMode( gameMode );

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
	if (!restoringInProgress)
		activeLaunchOptions().playerCount = uint( count );

	updateLaunchCommand();
}

void MainWindow::changeTeamDamage( double damage )
{
	if (!restoringInProgress)
		activeLaunchOptions().teamDamage = damage;

	updateLaunchCommand();
}

void MainWindow::changeTimeLimit( int timeLimit )
{
	if (!restoringInProgress)
		activeLaunchOptions().timeLimit = uint( timeLimit );

	updateLaunchCommand();
}

void MainWindow::changeFragLimit( int fragLimit )
{
	if (!restoringInProgress)
		activeLaunchOptions().fragLimit = uint( fragLimit );

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
		ui->screenshotDirLine->setText( dirPath );
	}
}

void MainWindow::toggleUsePresetName( bool checked )
{
	if (!restoringInProgress)
		globalOpts.usePresetNameAsDir = checked;

	ui->saveDirLine->setEnabled( !checked );
	ui->saveDirBtn->setEnabled( !checked );
	ui->screenshotDirLine->setEnabled( !checked );
	ui->screenshotDirBtn->setEnabled( !checked );

	if (checked)
	{
		int selectedPresetIdx = getSelectedItemIndex( ui->presetListView );
		QString presetName = selectedPresetIdx >= 0 ? presetModel[ selectedPresetIdx ].name : "";

		setAltDirsRelativeToConfigs( presetName );
	}
}

void MainWindow::changeSaveDir( const QString & dir )
{
	if (!restoringInProgress)
		activeLaunchOptions().saveDir = dir;

	if (isValidDir( dir ))
		updateSaveFilesFromDir();

	updateLaunchCommand();
}

void MainWindow::changeScreenshotDir( const QString & dir )
{
	if (!restoringInProgress)
		activeLaunchOptions().screenshotDir = dir;

	updateLaunchCommand();
}

void MainWindow::browseSaveDir()
{
	QString path = QFileDialog::getExistingDirectory( this, "Locate the directory with saves", pathContext.baseDir().path() );
	if (path.isEmpty())  // user probably clicked cancel
		return;

	// the path comming out of the file dialog is always absolute
	if (pathContext.usingRelativePaths())
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
	if (pathContext.usingRelativePaths())
		path = pathContext.getRelativePath( path );

	ui->screenshotDirLine->setText( path );
	// the rest is done in screenshotDirLine callback
}


//----------------------------------------------------------------------------------------------------------------------
//  output options

void MainWindow::selectMonitor( int index )
{
	if (!restoringInProgress)
		outputOpts.monitorIdx = index;

	updateLaunchCommand();
}

void MainWindow::changeResolutionX( const QString & xStr )
{
	if (!restoringInProgress)
		outputOpts.resolutionX = xStr.toUInt();

	updateLaunchCommand();
}

void MainWindow::changeResolutionY( const QString & yStr )
{
	if (!restoringInProgress)
		outputOpts.resolutionY = yStr.toUInt();

	updateLaunchCommand();
}

void MainWindow::toggleShowFps( bool checked )
{
	if (!restoringInProgress)
		outputOpts.showFPS = checked;

	updateLaunchCommand();
}

void MainWindow::toggleNoSound( bool checked )
{
	if (!restoringInProgress)
		outputOpts.noSound = checked;

	updateLaunchCommand();
}

void MainWindow::toggleNoSFX( bool checked )
{
	if (!restoringInProgress)
		outputOpts.noSFX = checked;

	updateLaunchCommand();
}

void MainWindow::toggleNoMusic( bool checked )
{
	if (!restoringInProgress)
		outputOpts.noMusic = checked;

	updateLaunchCommand();
}


//----------------------------------------------------------------------------------------------------------------------
//  additional command line arguments

void MainWindow::updatePresetCmdArgs( const QString & text )
{
	// update the current preset
	int selectedPresetIdx = getSelectedItemIndex( ui->presetListView );
	if (selectedPresetIdx >= 0 && !restoringInProgress)
	{
		presetModel[ selectedPresetIdx ].cmdArgs = text;
	}

	updateLaunchCommand();
}

void MainWindow::updateGlobalCmdArgs( const QString & text )
{
	if (!restoringInProgress)
		globalOpts.cmdArgs = text;

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
		for (Mod & mod : preset.mods)
		{
			mod.path = pathContext.convertPath( mod.path );
		}
		preset.selectedEnginePath = pathContext.convertPath( preset.selectedEnginePath );
		preset.selectedIWAD = pathContext.convertPath( preset.selectedIWAD );
	}

	ui->saveDirLine->setText( pathContext.convertPath( ui->saveDirLine->text() ) );
	ui->screenshotDirLine->setText( pathContext.convertPath( ui->screenshotDirLine->text() ) );

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
	                                         ? getEngineProperties( engineModel[ selectedEngineIdx ].family ).compLvlStyle
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

	serialize( jsRoot, opts );

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
			QJsonObject jsPreset = serialize( preset, opts.launchOptsStorage == StoreToPreset );
			jsPresetArray.append( jsPreset );
		}
		jsRoot["presets"] = jsPresetArray;
	}

	if (opts.launchOptsStorage == StoreGlobally)
	{
		jsRoot["launch_options"] = serialize( launchOpts );
	}

	jsRoot["output_options"] = serialize( outputOpts );

	serialize( jsRoot, globalOpts );

	int presetIdx = getSelectedItemIndex( ui->presetListView );
	jsRoot["selected_preset"] = presetIdx >= 0 ? presetModel[ presetIdx ].name : "";

	QJsonDocument jsonDoc( jsRoot );
	QByteArray rawData = jsonDoc.toJson();

	QString error = updateFile( filePath, rawData );
	if (!error.isEmpty())
	{
		QMessageBox::warning( this, "Error saving options", error );
	}
	// return error.isEmpty();
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

	// Restoring any stored options is tricky.
	// Every change of a selection, like  cmbBox->setCurrentIndex( stored.idx )  or  selectItemByIdx( stored.idx )
	// causes the corresponding callbacks to be called which in turn might cause some other stored value to be changed.
	// For example  ui->engineCmbBox->setCurrentIndex( idx )  calls  selectEngine( idx )  which might indirectly call
	// selectConfig( idx )  which will overwrite the stored selected config and we will then restore a wrong one.
	// The least complicated workaround seems to be simply setting a flag indicating that we are in the middle of
	// restoring saved options, and then prevent storing values when this flag is set.
	restoringInProgress = true;

	// preset must be deselected first, so that the cleared selections don't save in the selected preset
	deselectAllAndUnsetCurrent( ui->presetListView );

	// this must be loaded early, because we need to know whether to attempt loading the opts from the presets
	deserialize( jsRoot, opts );
	pathContext.toggleAbsolutePaths( opts.useAbsolutePaths );

	if (JsonObjectCtx jsEngines = jsRoot.getObject( "engines" ))
	{
		ui->engineCmbBox->setCurrentIndex( -1 );  // if something is selected, this invokes the callback, which updates the dependent widgets

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

				if (!verifyPath( engine.path, "An engine from the saved options (%1) no longer exists. It will be removed from the list." ))
					continue;

				engineModel.append( std::move( engine ) );
			}
		}

		engineModel.finishCompleteUpdate();
	}

	if (JsonObjectCtx jsIWADs = jsRoot.getObject( "IWADs" ))
	{
		deselectAllAndUnsetCurrent( ui->iwadListView );  // if something is selected, this invokes the callback, which updates the dependent widgets

		iwadModel.startCompleteUpdate();

		iwadModel.clear();

		deserialize( jsIWADs, iwadSettings );

		if (iwadSettings.updateFromDir)
		{
			verifyPath( iwadSettings.dir, "IWAD directory from the saved options (%1) no longer exists. Please update it in Menu -> Setup." );
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

					if (!verifyPath( iwad.path, "An IWAD from the saved options (%1) no longer exists. It will be removed from the list." ))
						continue;

					iwadModel.append( std::move( iwad ) );
				}
			}
		}

		iwadModel.finishCompleteUpdate();
	}

	if (JsonObjectCtx jsMaps = jsRoot.getObject( "maps" ))
	{
		deselectAllAndUnsetCurrent( ui->mapDirView );

		deserialize( jsMaps, mapSettings );

		verifyPath( mapSettings.dir, "Map directory from the saved options (%1) no longer exists. Please update it in Menu -> Setup." );
	}

	if (JsonObjectCtx jsMods = jsRoot.getObject( "mods" ))
	{
		deselectAllAndUnsetCurrent( ui->modListView );

		deserialize( jsMods, modSettings );

		verifyPath( modSettings.dir, "Mod directory from the saved options (%1) no longer exists. Please update it in Menu -> Setup." );
	}

	if (JsonArrayCtx jsPresetArray = jsRoot.getArray( "presets" ))
	{
		deselectAllAndUnsetCurrent( ui->presetListView );  // this invokes the callback, which disables the dependent widgets

		presetModel.startCompleteUpdate();

		presetModel.clear();

		for (int i = 0; i < jsPresetArray.size(); i++)
		{
			JsonObjectCtx jsPreset = jsPresetArray.getObject( i );
			if (!jsPreset)  // wrong type on position i - skip this entry
				continue;

			Preset preset;
			deserialize( jsPreset, preset, opts.launchOptsStorage == StoreToPreset );

			presetModel.append( std::move( preset ) );
		}

		presetModel.finishCompleteUpdate();
	}

	// launch options
	if (opts.launchOptsStorage == StoreGlobally)
	{
		if (JsonObjectCtx jsOptions = jsRoot.getObject( "launch_options" ))
		{
			deserialize( jsOptions, launchOpts );
		}
	}

	if (JsonObjectCtx jsOptions = jsRoot.getObject( "output_options" ))
	{
		deserialize( jsOptions, outputOpts );
	}

	deserialize( jsRoot, globalOpts );

	// make sure all paths loaded from JSON are stored in correct format
	toggleAbsolutePaths( opts.useAbsolutePaths );

	optionsCorrupted = false;

	file.close();

	// update the lists from which the restored preset will select items
	if (iwadSettings.updateFromDir)
		updateIWADsFromDir();
	refreshMapPacks();
	// the rest of the lists will get updated when an engine is restored from a preset

	// load the last selected preset
	QString selectedPreset = jsRoot.getString( "selected_preset" );
	if (!selectedPreset.isEmpty())
	{
		int selectedPresetIdx = findSuch( presetModel, [&]( const Preset & preset )
		                                               { return preset.name == selectedPreset; } );
		if (selectedPresetIdx >= 0)
		{
			// This invokes the callback, which enables the dependent widgets and calls restorePreset(...)
			selectAndSetCurrentByIndex( ui->presetListView, selectedPresetIdx );
		}
		else
		{
			QMessageBox::warning( nullptr, "Preset no longer exists",
				"Preset that was selected last time ("%selectedPreset%") no longer exists. Did you mess up with the options.json?" );
		}
	}

	// this must be done after the lists are already updated because we want to select existing items in combo boxes,
	//               and after preset loading because the preset will select IWAD which will fill the map combo box
	if (opts.launchOptsStorage == StoreGlobally)
	{
		restoreLaunchOptions( launchOpts );  // this clears items that are invalid
	}

	restoreOutputOptions( outputOpts );

	ui->usePresetNameChkBox->setChecked( globalOpts.usePresetNameAsDir );

	ui->globalCmdArgsLine->setText( globalOpts.cmdArgs );

	restoringInProgress = false;

	updateLaunchCommand();
}

void MainWindow::restoreLaunchOptions( LaunchOptions & opts )
{
	// launch mode
	switch (opts.mode)
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

	// alternative paths
	// This must be restored before the save combo-box, so that the combo-box is filled from the right directory
	// and the selected save file is present there.
	ui->saveDirLine->setText( opts.saveDir );
	ui->screenshotDirLine->setText( opts.screenshotDir );

	// details of launch mode
	ui->mapCmbBox->setCurrentIndex( ui->mapCmbBox->findText( opts.mapName ) );
	if (!opts.saveFile.isEmpty())
	{
		int saveFileIdx = findSuch( saveModel, [&]( const SaveFile & save )
		                                       { return save.fileName == opts.saveFile; } );
		if (saveFileIdx >= 0)
		{
			ui->saveFileCmbBox->setCurrentIndex( saveFileIdx );
		}
		else
		{
			QMessageBox::warning( this, "Save file no longer exists",
				"Save file \""%opts.saveFile%"\" no longer exists, please select another one." );
			ui->saveFileCmbBox->setCurrentIndex( -1 );
			opts.saveFile.clear();  // if previous index was -1, callback is not called, so we clear the invalid item manually
		}
	}
	ui->mapCmbBox_demo->setCurrentIndex( ui->mapCmbBox_demo->findText( opts.mapName_demo ) );
	ui->demoFileLine_record->setText( opts.demoFile_record );
	if (!opts.demoFile_replay.isEmpty())
	{
		int demoFileIdx = findSuch( demoModel, [&]( const DemoFile & demo )
		                                       { return demo.fileName == opts.demoFile_replay; } );
		if (demoFileIdx >= 0)
		{
			ui->demoFileCmbBox_replay->setCurrentIndex( demoFileIdx );
		}
		else
		{
			QMessageBox::warning( this, "Demo file no longer exists",
				"Demo file \""%opts.demoFile_replay%"\" no longer exists, please select another one." );
			ui->demoFileCmbBox_replay->setCurrentIndex( -1 );
			opts.demoFile_replay.clear();  // if previous index was -1, callback is not called, so we clear the invalid item manually
		}
	}

	// TODO: check combo-box boundaries

	// gameplay
	ui->skillSpinBox->setValue( int( opts.skillNum ) );
	ui->noMonstersChkBox->setChecked( opts.noMonsters );
	ui->fastMonstersChkBox->setChecked( opts.fastMonsters );
	ui->monstersRespawnChkBox->setChecked( opts.monstersRespawn );
	compatOptsCmdArgs = CompatOptsDialog::getCmdArgsFromOptions( opts.compatOpts );
	ui->compatLevelCmbBox->setCurrentIndex( opts.compatLevel + 1 );
	ui->allowCheatsChkBox->setChecked( opts.allowCheats );

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

void MainWindow::restoreOutputOptions( OutputOptions & opts )
{
	// video
	if (opts.monitorIdx < ui->monitorCmbBox->count())
		ui->monitorCmbBox->setCurrentIndex( opts.monitorIdx );
	if (opts.resolutionX > 0)
		ui->resolutionXLine->setText( QString::number( opts.resolutionX ) );
	if (opts.resolutionY > 0)
		ui->resolutionYLine->setText( QString::number( opts.resolutionY ) );
	ui->showFpsChkBox->setChecked( opts.showFPS );

	// audio
	ui->noSoundChkBox->setChecked( opts.noSound );
	ui->noSfxChkBox->setChecked( opts.noSFX );
	ui->noMusicChkBox->setChecked( opts.noMusic );
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

void MainWindow::exportPresetToScript()
{
	int selectedIdx = getSelectedItemIndex( ui->presetListView );
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
 #ifdef _WIN32

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

	QString filePath = QFileDialog::getSaveFileName( this, "Export preset", shortcutFileSuffix );
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
	return;

 #endif // _WIN32
}

void MainWindow::importPresetFromScript()
{
	QMessageBox::warning( this, "Not implemented", "Sorry, this feature is not implemented yet." );
/*
	QString filePath = QFileDialog::getOpenFileName( this, "Import preset", QString(), scriptFileExt );
	if (filePath.isEmpty()) {  // user probably clicked cancel
		return;
	}

	QFileInfo fileInfo( filePath );
	QString fileDir = fileInfo.path();

	QFile file( filePath );
	if (!file.open( QIODevice::ReadOnly | QIODevice::Text )) {
		QMessageBox::warning( this, "Cannot open file", "Cannot open file for reading. Check file permissions." );
		return;
	}

	QTextStream stream( &file );
	QString command = stream.readLine( 10 * 1024 );
	if (stream.status() == QTextStream::ReadCorruptData) {
		QMessageBox::warning( this, "Error reading file", "An error occured while reading the file. Check if the disk isn't disconnected." );
		return;
	}

	if (!stream.atEnd() && !stream.readLine( 1 ).isEmpty()) {
		QMessageBox::warning( this, "Problem with parsing file", "Only single line shorter than 10kB is supported, the rest will be ignored." );
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

void MainWindow::updateLaunchCommand( bool verifyPaths )
{
	// optimization - don't regenerate the command when we're about to make more changes right away
	if (restoringInProgress)
		return;

	//static uint count = 1;
	//qDebug() << "updateLaunchCommand()" << count++;

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
		//static int i = 0;
		//qDebug() << "updating " << i++;
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
	const QString executableName = getFileBasenameFromPath( selectedEngine.path );
	const EngineProperties & engineProperties = getEngineProperties( selectedEngine.family );

	{
		throwIfInvalid( verifyPaths, selectedEngine.path, "The selected engine (%1) no longer exists. Please update its path in Menu -> Setup." );

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
	#ifdef _WIN32
		if (opts.showEngineOutput)
			cmd.arguments << "-stdout";
	#endif

		const int configIdx = ui->configCmbBox->currentIndex();
		if (configIdx > 0)  // at index 0 there is an empty placeholder to allow deselecting config
		{
			QString configPath = getPathFromFileName( selectedEngine.configDir, configModel[ configIdx ].fileName );

			throwIfInvalid( verifyPaths, configPath, "The selected config (%1) no longer exists. Please update the config dir in Menu -> Setup" );
			cmd.arguments << "-config" << base.rebaseAndQuotePath( configPath );
		}
	}

	int selectedIwadIdx = getSelectedItemIndex( ui->iwadListView );
	if (selectedIwadIdx >= 0)
	{
		throwIfInvalid( verifyPaths, iwadModel[ selectedIwadIdx ].path, "The selected IWAD (%1) no longer exists. Please select another one." );
		cmd.arguments << "-iwad" << base.rebaseAndQuotePath( iwadModel[ selectedIwadIdx ].path );
	}

	QVector<QString> selectedFiles;

	const QStringList selectedMapPacks = getSelectedMapPacks();
	for (const QString & mapFilePath : selectedMapPacks)
	{
		throwIfInvalid( verifyPaths, mapFilePath, "The selected map pack (%1) no longer exists. Please select another one." );

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
			throwIfInvalid( verifyPaths, mod.path, "The selected mod (%1) no longer exists. Please update the mod list." );

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

	const LaunchOptions & activeLaunchOpts = activeLaunchOptions();

	LaunchMode launchMode = getLaunchModeFromUI();
	if (launchMode == LaunchMap)
	{
		cmd.arguments << getMapArgs( engineProperties.mapParamStyle, ui->mapCmbBox->currentIndex(), ui->mapCmbBox->currentText() );
	}
	else if (launchMode == LoadSave && !ui->saveFileCmbBox->currentText().isEmpty())
	{
		QString savePath = getPathFromFileName( getSaveDir(), ui->saveFileCmbBox->currentText() );
		throwIfInvalid( verifyPaths, savePath, "The selected save file (%1) no longer exists. Please select another one." );
		cmd.arguments << "-loadgame" << base.rebaseAndQuotePath( savePath );
	}
	else if (launchMode == RecordDemo && !ui->demoFileLine_record->text().isEmpty())
	{
		QString demoPath = getPathFromFileName( getSaveDir(), ui->demoFileLine_record->text() );
		cmd.arguments << "-record" << base.rebaseAndQuotePath( demoPath );
		cmd.arguments << getMapArgs( engineProperties.mapParamStyle, ui->mapCmbBox_demo->currentIndex(), ui->mapCmbBox_demo->currentText() );
	}
	else if (launchMode == ReplayDemo && !ui->demoFileCmbBox_replay->currentText().isEmpty())
	{
		QString demoPath = getPathFromFileName( getSaveDir(), ui->demoFileCmbBox_replay->currentText() );
		throwIfInvalid( verifyPaths, demoPath, "The selected demo file (%1) no longer exists. Please select another one." );
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
	if (ui->gameOptsBtn->isEnabled() && activeLaunchOpts.gameOpts.flags1 != 0)
		cmd.arguments << "+dmflags" << QString::number( activeLaunchOpts.gameOpts.flags1 );
	if (ui->gameOptsBtn->isEnabled() && activeLaunchOpts.gameOpts.flags2 != 0)
		cmd.arguments << "+dmflags2" << QString::number( activeLaunchOpts.gameOpts.flags2 );
	if (ui->compatLevelCmbBox->isEnabled() && activeLaunchOpts.compatLevel >= 0)
		cmd.arguments << getCompatLevelArgs( executableName, engineProperties.compLvlStyle, activeLaunchOpts.compatLevel );
	if (ui->compatOptsBtn->isEnabled() && !compatOptsCmdArgs.isEmpty())  // TODO
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
		cmd.arguments << engineProperties.saveDirParam << base.rebaseAndQuotePath( ui->saveDirLine->text() );
	if (!ui->screenshotDirLine->text().isEmpty())
		cmd.arguments << "+screenshot_dir" << base.rebaseAndQuotePath( ui->screenshotDirLine->text() );

	if (ui->monitorCmbBox->currentIndex() > 0)
	{
		int monitorIndex = ui->monitorCmbBox->currentIndex() - 1;  // the first item is a placeholder for leaving it default
		int engineMonitorIndex = getFirstMonitorIndex( executableName ) + monitorIndex;  // some engines index monitors from 1 and others from 0
		cmd.arguments << "+vid_adapter" << QString::number( engineMonitorIndex );
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
	catch (const FileNotFound &)
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

	// at the end of function restore the previous current dir
	auto guard = atScopeEndDo( [&](){ QDir::setCurrent( currentDir ); } );

	if (opts.showEngineOutput)
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

		if (opts.closeOnLaunch)
		{
			QApplication::quit();
		}
	}
}
