//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of the main window, including all its tabs
//======================================================================================================================

#ifndef MAIN_WINDOW_INCLUDED
#define MAIN_WINDOW_INCLUDED


#include "Common.hpp"

#include "MiscUtils.hpp"  // DialogCommon
#include "UserData.hpp"
#include "ListModel.hpp"
#include "UpdateChecker.hpp"

#include <QMainWindow>
#include <QString>
#include <QFileInfo>
#include <QFileSystemModel>

#include <optional>

class QItemSelection;
class QComboBox;
class QLineEdit;
class OptionsToLoad;

namespace Ui {
	class MainWindow;
}


//======================================================================================================================

class MainWindow : public QMainWindow, private DialogCommon {

	Q_OBJECT

	using thisClass = MainWindow;

 public:

	explicit MainWindow();
	virtual ~MainWindow() override;

 private: // overridden methods

	virtual void timerEvent( QTimerEvent * event ) override;
	virtual void closeEvent( QCloseEvent * event ) override;

 private slots:

	void onWindowShown();

	void runAboutDialog();
	void runSetupDialog();
	void runOptsStorageDialog();
	void runGameOptsDialog();
	void runCompatOptsDialog();

	void autoselectLoneItems();

	void selectEngine( int index );
	void selectConfig( int index );
	void togglePreset( const QItemSelection & selected, const QItemSelection & deselected );
	void toggleIWAD( const QItemSelection & selected, const QItemSelection & deselected );
	void toggleMapPack( const QItemSelection & selected, const QItemSelection & deselected );
	void modDataChanged( const QModelIndex & topLeft, const QModelIndex & bottomRight, const QVector<int> & roles );

	void showMapPackDesc( const QModelIndex & index );

	void cloneConfig();

	void presetAdd();
	void presetDelete();
	void presetClone();
	void presetMoveUp();
	void presetMoveDown();
	void presetInsertSeparator();

	void modAdd();
	void modAddDir();
	void modDelete();
	void modMoveUp();
	void modMoveDown();
	void modInsertSeparator();
	void modsDropped( int row, int count );

	void modeDefault();
	void modeLaunchMap();
	void modeSavedGame();
	void modeRecordDemo();
	void modeReplayDemo();
	void changeMap( const QString & mapName );
	void selectSavedGame( int index );
	void changeMap_demo( const QString & mapName );
	void changeDemoFile_record( const QString & fileName );
	void selectDemoFile_replay( int index );

	void selectSkill( int skillIdx );
	void changeSkillNum( int skillNum );
	void toggleNoMonsters( bool checked );
	void toggleFastMonsters( bool checked );
	void toggleMonstersRespawn( bool checked );
	void toggleAllowCheats( bool checked );
	void selectCompatLevel( int compatLevel );

	void toggleMultiplayer( bool checked );
	void selectMultRole( int role );
	void changeHost( const QString & host );
	void changePort( int port );
	void selectNetMode( int mode );
	void selectGameMode( int mode );
	void changePlayerCount( int count );
	void changeTeamDamage( double damage );
	void changeTimeLimit( int limit );
	void changeFragLimit( int limit );

	void toggleUsePresetName( bool checked );
	void changeSaveDir( const QString & dir );
	void changeScreenshotDir( const QString & dir );
	void browseSaveDir();
	void browseScreenshotDir();

	void selectMonitor( int index );
	void changeResolutionX( const QString & xStr );
	void changeResolutionY( const QString & yStr );
	void toggleShowFps( bool checked );

	void toggleNoSound( bool checked );
	void toggleNoSFX( bool checked );
	void toggleNoMusic( bool checked );

	void exportPresetToScript();
	void exportPresetToShortcut();
	void importPresetFromScript();

	void updatePresetCmdArgs( const QString & text );
	void updateGlobalCmdArgs( const QString & text );

	void launch();

 private: // methods

	void setupPresetList();
	void setupIWADList();
	void setupMapPackList();
	void setupModList();

	void updateOptionsGrpBoxTitles( const StorageSettings & storageSettings );

	void loadMonitorInfo( QComboBox * box );

	void toggleAbsolutePaths( bool absolute );

	void updateEngineTraits();

	void setAltDirsRelativeToConfigs( const QString & dirName );

	void updateIWADsFromDir();
	void refreshMapPacks();
	void updateConfigFilesFromDir();
	void updateSaveFilesFromDir();
	void updateDemoFilesFromDir();
	void updateCompatLevels();
	void updateMapsFromSelectedWADs( std::optional< QStringList > selectedMapPacks = std::nullopt );

	void togglePresetSubWidgets( bool enabled );
	void clearPresetSubWidgets();

	void toggleSkillSubwidgets( bool enabled );
	void toggleOptionsSubwidgets( bool enabled );

	bool saveOptions( const QString & fileName );
	bool loadOptions( const QString & fileName );

	void restoreLoadedOptions( OptionsToLoad && opts );
	void restorePreset( int index );

	void restoreLaunchAndMultOptions( LaunchOptions & launchOpts, MultiplayerOptions & multOpts );
	void restoreGameplayOptions( GameplayOptions & opts );
	void restoreCompatibilityOptions( CompatibilityOptions & opts );
	void restoreAlternativePaths( AlternativePaths & opts );
	void restoreVideoOptions( VideoOptions & opts );
	void restoreAudioOptions( AudioOptions & opts );
	void restoreGlobalOptions( GlobalOptions & opts );

	struct ShellCommand
	{
		QString executable;
		QStringList arguments;
	};
	ShellCommand generateLaunchCommand( const QString & baseDir, bool verifyPaths, bool quotePaths );
	void updateLaunchCommand( bool verifyPaths = false );

 private: // MainWindow-specific utils

	QStringList getSelectedMapPacks() const;

	QString getConfigDir() const;
	QString getSaveDir() const;

	LaunchMode getLaunchModeFromUI() const;

	LaunchOptions & activeLaunchOptions();
	MultiplayerOptions & activeMultiplayerOptions();
	GameplayOptions & activeGameplayOptions();
	CompatibilityOptions & activeCompatOptions();

 private: // internal members

	Ui::MainWindow * ui = nullptr;

	uint tickCount = 0;

	QDir appDataDir;   ///< directory where this application can store its data
	QString optionsFilePath;

	bool optionsCorrupted = false;  ///< true when was a critical error during parsing of options file, such content should not be saved

	bool disableSelectionCallbacks = false;  ///< flag that temporarily disables callbacks like selectEngine(), selectConfig(), selectIWAD()
	bool restoringOptionsInProgress = false;  ///< flag used to temporarily prevent storing selected values to a preset or global launch options
	bool restoringPresetInProgress = false;  ///< flag used to temporarily prevent storing selected values to a preset or global launch options

	CompatLevelStyle lastCompLvlStyle = CompatLevelStyle::None;  ///< compat level style of the engine that was selected the last time

	QStringList compatOptsCmdArgs;  ///< string with command line args created from compatibility options, cached so that it doesn't need to be regenerated on every command line update

	UpdateChecker updateChecker;

 private: // user data

	// We use model-view design pattern for several widgets, because it allows us to organize the data in a way we need,
	// and have the widget (frontend) automatically mirror the underlying data (backend) without syncing them manually.
	//
	// You can read more about it here: https://doc.qt.io/qt-5/model-view-programming.html#model-subclassing-reference

	ReadOnlyListModel< Engine > engineModel;    ///< user-ordered list of engines (managed by SetupDialog)

	// We need to store extra information for each engine, but making it a part of Engine struct is not a good idea,
	// because this extra info is not user-defined and should not be serialized.
	// Locally extending the Engine struct would mean we would have to strip this info away everytime we would need to
	// pass it to the SetupDialog or OptionsDeserializer.
	// Only because the content of the engine list is constant during the loop of this window and only changed in
	// SetupDialog or OptionsDeserializer, we can afford to do this, otherwise painful manual sychronization between
	// engineModel and engineTraits would be needed.
	QList< EngineTraits > engineTraits;   ///< traits of each loaded engine, has same indexes as engineModel

	struct ConfigFile : public ReadOnlyListModelItem
	{
		QString fileName;
		ConfigFile( const QString & fileName ) : fileName( fileName ) {}
		ConfigFile( const QFileInfo & file ) : fileName( file.fileName() ) {}
	};
	ReadOnlyListModel< ConfigFile > configModel;    ///< list of config files found in pre-defined directory

	struct SaveFile : public ReadOnlyListModelItem
	{
		QString fileName;
		SaveFile( const QString & fileName ) : fileName( fileName ) {}
		SaveFile( const QFileInfo & file ) : fileName( file.fileName() ) {}
	};
	ReadOnlyListModel< SaveFile > saveModel;    ///< list of save files found in pre-defined directory

	struct DemoFile : public ReadOnlyListModelItem
	{
		QString fileName;
		DemoFile( const QString & fileName ) : fileName( fileName ) {}
		DemoFile( const QFileInfo & file ) : fileName( file.fileName() ) {}
	};
	ReadOnlyListModel< DemoFile > demoModel;    ///< list of demo files found in pre-defined directory

	IwadSettings iwadSettings;    ///< IWAD-related preferences (value returned by SetupDialog)
	ReadOnlyListModel< IWAD > iwadModel;    ///< user-ordered list of iwads (managed by SetupDialog)

	MapSettings mapSettings;    ///< map-related preferences (value returned by SetupDialog)
	QFileSystemModel mapModel;  ///< model representing a directory with map files

	ModSettings modSettings;    ///< mod-related preferences (value returned by SetupDialog)
	EditableListModel< Mod > modModel;

	EditableListModel< Preset > presetModel;    ///< user-made presets, when one is selected from the list view, it applies its stored options to the other widgets

	LaunchOptions launchOpts;
	MultiplayerOptions multOpts;
	GameplayOptions gameOpts;
	CompatibilityOptions compatOpts;
	VideoOptions videoOpts;
	AudioOptions audioOpts;
	GlobalOptions globalOpts;

	LauncherSettings settings;

};


#endif // MAIN_WINDOW_INCLUDED
