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
#include "Widgets/SearchPanel.hpp"
#include "UserData.hpp"
#include "UpdateChecker.hpp"
#include "Themes.hpp"  // SystemThemeWatcher
#include "Utils/JsonUtils.hpp"  // JsonDocumentCtx

#include <QMainWindow>
#include <QString>
#include <QFileInfo>
#include <QFileSystemModel>

class QTableWidget;
class QItemSelection;
class QComboBox;
class QLineEdit;
class OptionsToLoad;

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

	void runAboutDialog();
	void runSetupDialog();
	void runOptsStorageDialog();
	void runGameOptsDialog();
	void runCompatOptsDialog();

	void onEngineSelected( int index );
	void onConfigSelected( int index );
	void onPresetToggled( const QItemSelection & selected, const QItemSelection & deselected );
	void onIWADToggled( const QItemSelection & selected, const QItemSelection & deselected );
	void onMapPackToggled( const QItemSelection & selected, const QItemSelection & deselected );
	void onPresetDataChanged( const QModelIndex & topLeft, const QModelIndex & bottomRight, const QVector<int> & roles );
	void onModDataChanged( const QModelIndex & topLeft, const QModelIndex & bottomRight, const QVector<int> & roles );

	void showMapPackDesc( const QModelIndex & index );

	void onMapDirUpdated( const QString & path );

	void openEngineDataDir();
	void cloneConfig();

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
	void modToggleIcons();
	void onModsDropped( int row, int count );

	void onModeChosen_Default();
	void onModeChosen_LaunchMap();
	void onModeChosen_SavedGame();
	void onModeChosen_RecordDemo();
	void onModeChosen_ReplayDemo();
	void onMapChanged( const QString & mapName );
	void onSavedGameSelected( int index );
	void onMapChanged_demo( const QString & mapName );
	void onDemoFileChanged_record( const QString & fileName );
	void onDemoFileSelected_replay( int index );

	void onSkillSelected( int skillIdx );
	void onSkillNumChanged( int skillNum );
	void onNoMonstersToggled( bool checked );
	void onFastMonstersToggled( bool checked );
	void onMonstersRespawnToggled( bool checked );
	void onAllowCheatsToggled( bool checked );
	void onCompatLevelSelected( int compatLevel );

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

	void onUsePresetNameToggled( bool checked );
	void onSaveDirChanged( const QString & dir );
	void onScreenshotDirChanged( const QString & dir );
	void browseSaveDir();
	void browseScreenshotDir();

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

	void exportPresetToScript();
	void exportPresetToShortcut();
	void importPresetFromScript();

	void onPresetCmdArgsChanged( const QString & text );
	void onGlobalCmdArgsChanged( const QString & text );

	void launch();

 private: // methods

	void adjustUi();

	void setupPresetList();
	void setupIWADList();
	void setupMapPackList();
	void setupModList();

	void setupEnvVarLists();

	void updateOptionsGrpBoxTitles( const StorageSettings & storageSettings );

	void loadMonitorInfo( QComboBox * box );

	void initAppDataDir();
	static void moveOptionsFromOldDir( QDir oldOptionsDir, QDir newOptionsDir, QString optionsFileName );

	void togglePathStyle( PathStyle style );

	void fillDerivedEngineInfo( DirectList< EngineInfo > & engines );

	void autoselectItems();

	void setAlternativeDirs( const QString & dirName );

	void updateListsFromDirs();
	void updateIWADsFromDir();
	void resetMapDirModelAndView();
	void updateConfigFilesFromDir( const QString * configDir = nullptr );
	void updateSaveFilesFromDir( const QString * saveDir = nullptr );
	void updateDemoFilesFromDir( const QString * demoDir = nullptr );
	void updateCompatLevels();
	void updateMapsFromSelectedWADs( const QStringVec * selectedMapPacks = nullptr );

	void moveEnvVarToKeepTableSorted( QTableWidget * table, EnvVars * envVars, int rowIdx );

	void togglePresetSubWidgets( bool enabled );
	void clearPresetSubWidgets();

	void toggleSkillSubwidgets( bool enabled );
	void toggleOptionsSubwidgets( bool enabled );

	bool saveOptions( const QString & filePath );
	bool reloadOptions( const QString & filePath );
	std::unique_ptr< JsonDocumentCtx > readOptions( const QString & filePath );
	void loadAppearance( const JsonDocumentCtx & optionsDoc, bool loadGeometry );
	void loadTheRestOfOptions( const JsonDocumentCtx & optionsDoc );

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

	void updateLaunchCommand();
	os::ShellCommand generateLaunchCommand(
		const QString & parentWorkingDir, PathStyle enginePathStyle, const QString & engineWorkingDir, PathStyle argPathStyle,
		bool quotePaths, bool verifyPaths
	);

	int askForExtraPermissions( const EngineInfo & selectedEngine, const QStringVec & permissions );
	bool startDetached(
		const QString & executable, const QStringVec & arguments, const QString & workingDir = {}, const EnvVars & envVars = {}
	);

 private: // MainWindow-specific utils

	struct ConfigFile;

	Preset * getSelectedPreset() const;
	EngineInfo * getSelectedEngine() const;
	ConfigFile * getSelectedConfig() const;
	IWAD * getSelectedIWAD() const;

	template< typename Functor > void forEachSelectedMapPack( const Functor & loopBody ) const;
	QStringVec getSelectedMapPacks() const;

	QStringList getUniqueMapNamesFromWADs( const QVector<QString> & selectedWADs ) const;

	QString getConfigDir() const;
	QString getDataDir() const;
	QString getSaveDir() const;
	QString getScreenshotDir() const;
	QString getDemoDir() const;

	QString convertRebasedEngineDataPath( QString path ) const;
	QString rebaseSaveFilePath( const QString & filePath, const PathRebaser & workingDirRebaser, const EngineInfo * engine );

	LaunchMode getLaunchModeFromUI() const;

	template< typename Functor > void forEachDirToBeAccessed( const Functor & loopBody ) const;
	QStringVec getDirsToBeAccessed() const;

	void scheduleSavingOptions( bool storedOptionsModified = true );

	LaunchOptions & activeLaunchOptions();
	MultiplayerOptions & activeMultiplayerOptions();
	GameplayOptions & activeGameplayOptions();
	CompatibilityOptions & activeCompatOptions();
	VideoOptions & activeVideoOptions();
	AudioOptions & activeAudioOptions();

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

	PathRebaser engineDataDirRebaser;   ///< path convertor set up to rebase relative paths from the current working dir to the engine's data dir and back

	CompatLevelStyle lastCompLvlStyle = CompatLevelStyle::None;  ///< compat level style of the engine that was selected the last time

	QStringVec compatOptsCmdArgs;  ///< string with command line args created from compatibility options, cached so that it doesn't need to be regenerated on every command line update

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
