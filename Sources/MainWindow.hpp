//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of the main window, including all its tabs
//======================================================================================================================

#ifndef MAIN_WINDOW_INCLUDED
#define MAIN_WINDOW_INCLUDED


#include "Dialogs/DialogCommon.hpp"

#include "Widgets/ListModel.hpp"
#include "Widgets/EditableListView.hpp"  // DnDType
#include "Widgets/SearchPanel.hpp"
#include "UserData.hpp"
#include "UpdateChecker.hpp"
#include "Themes.hpp"  // SystemThemeWatcher

#include <QMainWindow>
#include <QString>
#include <QFileInfo>
#include <QFileSystemModel>

class QTableWidget;
class QItemSelection;
class QComboBox;
class QLineEdit;
class QShortcut;

class JsonDocumentCtx;
struct OptionsToLoad;

namespace Ui {
	class MainWindow;
}


//======================================================================================================================

class MainWindow : public QMainWindow, private DialogWithPaths {

	Q_OBJECT

	using thisClass = MainWindow;
	using superClass = QMainWindow;

 public:

	explicit MainWindow();
	virtual ~MainWindow() override;

 private: // overridden methods

	virtual void showEvent( QShowEvent * event ) override;
	virtual void timerEvent( QTimerEvent * event ) override;
	virtual void closeEvent( QCloseEvent * event ) override;

 private slots:

	void onWindowShown();

	void onAboutActionTriggered();
	void onSetupActionTriggered();
	void onOptsStorageActionTriggered();
	void onExportToScriptTriggered();
	void onExportToShortcutTriggered();
	//void onImportFromScriptTriggered();

	void onEngineSelected( int index );
	void onConfigSelected( int index );
	void onPresetToggled( const QItemSelection & selected, const QItemSelection & deselected );
	void onIWADToggled( const QItemSelection & selected, const QItemSelection & deselected );
	void onMapPackToggled( const QItemSelection & selected, const QItemSelection & deselected );
	void onPresetDataChanged( const QModelIndex & topLeft, const QModelIndex & bottomRight, const QVector<int> & roles );
	void onModDataChanged( const QModelIndex & topLeft, const QModelIndex & bottomRight, const QVector<int> & roles );

	void onIWADDoubleClicked( const QModelIndex & index );
	void onMapPackDoubleClicked( const QModelIndex & index );
	void onModDoubleClicked( const QModelIndex & index );

	void onMapDirUpdated( const QString & path );

	void onEngineDirBtnClicked();
	void onCloneConfigBtnClicked();

	void presetAdd();
	void presetDelete();
	void presetClone();
	void presetMoveUp();
	void presetMoveDown();
	void presetInsertSeparator();
	void onPresetsReordered();

	void searchPresets( const QString & phrase, bool caseSensitive, bool useRegex );

	void modAdd();
	void modAddDir();
	void modAddArg();
	void modDelete();
	void modMoveUp();
	void modMoveDown();
	void modInsertSeparator();
	void onMapsAfterModsToggled( bool checked );
	void onModIconsToggled();
	void onModsDropped( int row, int count, DnDType type );

	void onModeChosen_Default();
	void onModeChosen_LaunchMap();
	void onModeChosen_SavedGame();
	void onModeChosen_RecordDemo();
	void onModeChosen_ReplayDemo();
	void onModeChosen_ResumeDemo();
	void onMapChanged( const QString & mapName );
	void onSavedGameSelected( int index );
	void onMapChanged_demo( const QString & mapName );
	void onDemoFileChanged_record( const QString & fileName );
	void onDemoFileSelected_replay( int index );
	void onDemoFileSelected_resume( int index );
	void onDemoFileChanged_resume( const QString & fileName );

	void onSkillSelected( int skillIdx );
	void onSkillNumChanged( int skillNum );
	void onNoMonstersToggled( bool checked );
	void onFastMonstersToggled( bool checked );
	void onMonstersRespawnToggled( bool checked );
	void onPistolStartToggled( bool checked );
	void onAllowCheatsToggled( bool checked );
	void onGameOptsBtnClicked();
	void onCompatOptsBtnClicked();
	void onCompatModeSelected( int compatMode );

	void onMultiplayerToggled( bool checked );
	void onMultRoleSelected( int role );
	void onHostChanged( const QString & host );
	void onPortChanged( int port );
	void onNetModeSelected( int mode );
	void onGameModeSelected( int mode );
	void onPlayerCountChanged( int count );
	void onTeamDamageChanged( double damage );
	void onTimeLimitChanged( int limit );
	void onFragLimitChanged( int limit );
	void onPlayerNameChanged( const QString & name );
	void onPlayerColorRightClicked();

	void onUsePresetNameForConfigsToggled( bool checked );
	void onUsePresetNameForSavesToggled( bool checked );
	void onUsePresetNameForScreenshotsToggled( bool checked );
	void onAltConfigDirChanged( const QString & dir );
	void onAltSaveDirChanged( const QString & dir );
	void onAltScreenshotDirChanged( const QString & dir );
	void browseAltConfigDir();
	void browseAltSaveDir();
	void browseAltScreenshotDir();

	void onMonitorSelected( int index );
	void onResolutionXChanged( const QString & xStr );
	void onResolutionYChanged( const QString & yStr );
	void onShowFpsToggled( bool checked );

	void onNoSoundToggled( bool checked );
	void onNoSFXToggled( bool checked );
	void onNoMusicToggled( bool checked );

	void presetEnvVarAdd();
	void presetEnvVarDelete();
	void globalEnvVarAdd();
	void globalEnvVarDelete();

	void onPresetEnvVarDataChanged( int row, int column );
	void onGlobalEnvVarDataChanged( int row, int column );

	void onPresetCmdArgsChanged( const QString & text );
	void onGlobalCmdArgsChanged( const QString & text );
	void onCmdPrefixChanged( const QString & text );

	void onLaunchBtnClicked();

 private: // methods

	void adjustUi();

	void setupPresetList();
	void setupIWADList();
	void setupMapPackList();
	void setupModList();

	void setupEnvVarLists();

	void loadMonitorInfo( QComboBox * box );

	void updateOptionsGrpBoxTitles( const StorageSettings & storageSettings );

	void initAppDataDir();
	static void moveOptionsFromOldDir( QDir oldOptionsDir, QDir newOptionsDir, QString optionsFileName );

	bool saveOptions( const QString & filePath );
	bool reloadOptions( const QString & filePath );
	std::unique_ptr< JsonDocumentCtx > readOptions( const QString & filePath );
	void loadAppearance( const JsonDocumentCtx & optionsDoc, bool loadGeometry );
	void loadTheRestOfOptions( const JsonDocumentCtx & optionsDoc );
	int askForEngineInfoRefresh();

	bool isCacheDirty() const;
	bool saveCache( const QString & filePath );
	bool loadCache( const QString & filePath );

	void restoreLoadedOptions( OptionsToLoad && opts );
	void restorePreset( int index );

	void restoreSelectedEngine( Preset & preset );
	void restoreSelectedConfig( Preset & preset );
	void restoreSelectedIWAD( Preset & preset );
	void restoreSelectedMapPacks( Preset & preset );
	void restoreSelectedMods( Preset & preset );

	void restoreLaunchAndMultOptions( LaunchOptions & launchOpts, const MultiplayerOptions & multOpts );
	void restoreGameplayOptions( const GameplayOptions & opts );
	void restoreCompatibilityOptions( const CompatibilityOptions & opts );
	void restoreAlternativePaths( const AlternativePaths & opts );
	void restoreVideoOptions( const VideoOptions & opts );
	void restoreAudioOptions( const AudioOptions & opts );
	void restoreGlobalOptions( const GlobalOptions & opts );

	void restoreEnvVars( const EnvVars & envVars, QTableWidget * table );

	void restoreAppearance( const AppearanceSettings & appearance, bool restoreGeometry );
	void restoreWindowGeometry( const WindowGeometry & geometry );

	void togglePathStyle( PathStyle style );

	void fillDerivedEngineInfo( DirectList< EngineInfo > & engines, bool refreshAllAutoEngineInfo );

	void runAboutDialog();
	void runSetupDialog();
	void runOptsStorageDialog();
	void runGameOptsDialog();
	void runCompatOptsDialog();
	void runPlayerColorDialog();

	void showTxtDescriptionFor( const QString & filePath, const QString & contentType );

	void openCurrentEngineDataDir();
	void cloneCurrentEngineConfigFile();

	void exportPresetToScript();
	void exportPresetToShortcut();
	//void importPresetFromScript();

	void autoselectItems();

	void updateAlternativeDirs( const Preset * selectedPreset = nullptr );

	void updateListsFromDirs();
	void updateIWADsFromDir();
	void resetMapDirModelAndView();
	void updateConfigFilesFromDir( const QString * configDir = nullptr );
	void updateSaveFilesFromDir();
	void updateDemoFilesFromDir();
	void updateCompatModes();
	void updateMapsFromSelectedWADs( const QStringList * selectedMapPacks = nullptr );

	void moveEnvVarToKeepTableSorted( QTableWidget * table, EnvVars * envVars, int rowIdx );

	void togglePresetSubWidgets( const Preset * selectedPreset );
	void clearPresetSubWidgets();

	void toggleAndClearEngineDependentWidgets( const EngineInfo * selectedEngine );

	void toggleLaunchModeSubwidgets( LaunchMode mode );
	void toggleSkillSubwidgets( LaunchMode mode );
	void toggleOptionsSubwidgets( LaunchMode mode );

	struct LaunchCommandOptions
	{
		/// The currently selected engine.
		/** The command cannot be generated without one. */
		const EngineInfo & selectedEngine;

		/// Path style to be used for the engine executable.
		PathStyle exePathStyle;

		// Path style for the arguments is determined case by case.

		/// Working directory of the process that will run the command.
		/** This determines the relative path of the engine executable in the command. */
		const QString & runnersWorkingDir;

		/// Surround each path in the command with quotes.
		/** Required for displaying the command or saving it to a script file. */
		bool quotePaths;

		/// Verify that each path in the command is valid and leads to the correct entry type (file or directory).
		/** If invalid path is found, display a message box with an error description. */
		bool verifyPaths;
	};
	os::ShellCommand generateLaunchCommand( LaunchCommandOptions cmdOpts );

	void updateLaunchCommand();
	void executeLaunchCommand();
	int askForExtraPermissions( const EngineInfo & selectedEngine, const QStringList & permissions );

 private: // MainWindow-specific utils

	template< typename Func >
	void addShortcut( const QKeySequence & keys, const Func & shortcutAction );

	struct ConfigFile;

	Preset * getSelectedPreset() const;
	EngineInfo * getSelectedEngine() const;
	ConfigFile * getSelectedConfig() const;
	IWAD * getSelectedIWAD() const;

	template< typename Functor > void forEachSelectedMapPack( const Functor & loopBody ) const;
	QStringList getSelectedMapPacks() const;

	QStringList getUniqueMapNamesFromWADs( const QList<QString> & selectedWADs ) const;

	QString getEngineConfigDir() const;
	QString getEngineDataDir() const;
	QString getActiveConfigDir() const;
	QString getActiveSaveDir() const;
	QString getActiveScreenshotDir() const;
	QString getActiveDemoDir() const;

	template< typename Functor > void forEachDirToBeAccessed( const Functor & loopBody ) const;
	QStringList getDirsToBeAccessed() const;

	LaunchOptions & activeLaunchOptions();
	MultiplayerOptions & activeMultiplayerOptions();
	GameplayOptions & activeGameplayOptions();
	CompatibilityOptions & activeCompatOptions();
	VideoOptions & activeVideoOptions();
	AudioOptions & activeAudioOptions();

	static bool shouldEnableEngineDirBtn( const EngineInfo * selectedEngine );
	static bool shouldEnableConfigCmbBox( const EngineInfo * selectedEngine );
	static bool shouldEnableConfigCloneBtn( const EngineInfo * selectedEngine );
	static bool shouldEnableSkillSelector( LaunchMode mode );
	static bool shouldEnableGameOptsBtn( LaunchMode mode, const EngineInfo * selectedEngine );
	static bool shouldEnableCompatOptsBtn( LaunchMode mode, const EngineInfo * selectedEngine );
	static bool shouldEnableCompatModeCmbBox( LaunchMode mode, const EngineInfo * selectedEngine );
	static bool shouldEnableMultiplayerGrpBox( const StorageSettings & storage, const Preset * selectedPreset, const EngineInfo * selectedEngine );
	static bool shouldEnableNetModeCmbBox( bool multEnabled, int multRole, const EngineInfo * selectedEngine );
	static bool shouldEnablePlayerCount( bool multEnabled, int multRole, const EngineInfo * selectedEngine );
	static bool shouldEnablePlayerCustomization( bool multEnabled, const EngineInfo * selectedEngine );
	static bool shouldEnableAltConfigDir( const EngineInfo * selectedEngine, bool usePresetName );
	static bool shouldEnableAltSaveDir( const EngineInfo * selectedEngine, bool usePresetName );
	static bool shouldEnableAltScreenshotDir( const EngineInfo * selectedEngine, bool usePresetName );

	LaunchMode getLaunchModeFromUI() const;

	void scheduleSavingOptions( bool storedOptionsModified = true );

 private: // internal members

	Ui::MainWindow * ui = nullptr;
	SearchPanel * presetSearchPanel = nullptr;

	QAction * addCmdArgAction = nullptr;

	uint tickCount = 0;

	QDir appDataDir;   ///< directory where this application can store its data
	QString optionsFilePath;
	QString cacheFilePath;

	std::unique_ptr< JsonDocumentCtx > parsedOptionsDoc;  ///< result of first phase of options loading, kept for the second phase
	bool optionsNeedUpdate = false;  ///< indicates that the user has made a change and the options file needs to be updated
	bool optionsCorrupted = false;   ///< true if there was a critical error during parsing of the options file, such content should not be saved

	bool disableSelectionCallbacks = false;   ///< flag that temporarily disables callbacks like selectEngine(), selectConfig(), selectIWAD()
	bool disableEnvVarsCallbacks = false;     ///< flag that temporarily disables environment variable callbacks when the list is manually messed with
	bool restoringOptionsInProgress = false;  ///< flag used to temporarily prevent storing selected values to a preset or global launch options
	bool restoringPresetInProgress = false;   ///< flag used to temporarily prevent storing selected values to a preset or global launch options

	QString selectedPresetBeforeSearch;   ///< which preset was selected before the search results were displayed

	QString currentEngineSaveDir;         ///< cached path of the directory when the currently selected engine stores its save files by default, maintains path style of dataDir of the current engine
	QString currentEngineScreenshotDir;   ///< cached path of the directory when the currently selected engine stores its screenshots by default, maintains path style of dataDir of the current engine

	PathRebaser engineConfigDirRebaser;      ///< path convertor set up to rebase relative paths from the current working dir to the engine's config dir and back
	PathRebaser engineSaveDirRebaser;        ///< path convertor set up to rebase relative paths from the current working dir to the engine's save dir and back
	PathRebaser engineScreenshotDirRebaser;  ///< path convertor set up to rebase relative paths from the current working dir to the engine's screenshot dir and back

	CompatModeStyle lastCompLvlStyle = CompatModeStyle::None;  ///< compat mode style of the engine that was selected the last time

	QStringList compatOptsCmdArgs;  ///< string with command line args created from compatibility options, cached so that it doesn't need to be regenerated on every command line update

	UpdateChecker updateChecker;

 #if IS_WINDOWS
	SystemThemeWatcher systemThemeWatcher;
 #endif

 private: // user data

	// We use model-view design pattern for several widgets, because it allows us to organize the data in a way we need,
	// and have the widget (frontend) automatically mirror the underlying data (backend) without syncing them manually.
	//
	// You can read more about it here: https://doc.qt.io/qt-5/model-view-programming.html#model-subclassing-reference

	EngineSettings engineSettings;    ///< engine-related preferences (value returned by SetupDialog)
	ReadOnlyDirectListModel< EngineInfo > engineModel;    ///< user-ordered list of engines (managed by SetupDialog)

	struct ConfigFile : public ReadOnlyListModelItem
	{
		QString fileName;
		ConfigFile( const QString & fileName ) : fileName( fileName ) {}
		ConfigFile( const QFileInfo & file ) : fileName( file.fileName() ) {}
		QString getID() const { return fileName; }
	};
	ReadOnlyDirectListModel< ConfigFile > configModel;    ///< list of config files found in pre-defined directory

	struct SaveFile : public ReadOnlyListModelItem
	{
		QString fileName;
		SaveFile( const QString & fileName ) : fileName( fileName ) {}
		SaveFile( const QFileInfo & file ) : fileName( file.fileName() ) {}
		QString getID() const { return fileName; }
	};
	ReadOnlyDirectListModel< SaveFile > saveModel;    ///< list of save files found in pre-defined directory

	struct DemoFile : public ReadOnlyListModelItem
	{
		QString fileName;
		DemoFile( const QString & fileName ) : fileName( fileName ) {}
		DemoFile( const QFileInfo & file ) : fileName( file.fileName() ) {}
		QString getID() const { return fileName; }
	};
	ReadOnlyDirectListModel< DemoFile > demoModel;    ///< list of demo files found in pre-defined directory

	IwadSettings iwadSettings;    ///< IWAD-related preferences (value returned by SetupDialog)
	ReadOnlyDirectListModel< IWAD > iwadModel;    ///< user-ordered list of iwads (managed by SetupDialog)

	MapSettings mapSettings;    ///< map-related preferences (value returned by SetupDialog)
	QFileSystemModel mapModel;  ///< model representing a directory with map files

	ModSettings modSettings;    ///< mod-related preferences (value returned by SetupDialog)
	EditableDirectListModel< Mod > modModel;

	EditableFilteredListModel< Preset > presetModel;    ///< user-made presets, when one is selected from the list view, it applies its stored options to the other widgets

	LaunchOptions launchOpts;
	MultiplayerOptions multOpts;
	GameplayOptions gameOpts;
	CompatibilityOptions compatOpts;
	VideoOptions videoOpts;
	AudioOptions audioOpts;
	GlobalOptions globalOpts;

	LauncherSettings settings;
	AppearanceSettings appearance;

};


#endif // MAIN_WINDOW_INCLUDED
