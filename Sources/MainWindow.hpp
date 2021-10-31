//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  13.5.2019
// Description:
//======================================================================================================================

#ifndef MAIN_WINDOW_INCLUDED
#define MAIN_WINDOW_INCLUDED


#include "Common.hpp"

#include "UserData.hpp"
#include "ListModel.hpp"
#include "FileSystemUtils.hpp"  // PathContext
#include "EventFilters.hpp"  // ConfirmationFilter
#include "UpdateChecker.hpp"

#include <QMainWindow>
#include <QString>
#include <QFileInfo>
#include <QFileSystemModel>

class QItemSelection;
class QComboBox;

namespace Ui {
	class MainWindow;
}


//======================================================================================================================

class MainWindow : public QMainWindow {

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
	void runGameOptsDialog();
	void runCompatOptsDialog();

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

	void modAdd();
	void modAddDir();
	void modDelete();
	void modMoveUp();
	void modMoveDown();
	void modsDropped( int row, int count );

	void modeStandard();
	void modeLaunchMap();
	void modeSavedGame();
	void modeRecordDemo();
	void modeReplayDemo();
	void changeMap( const QString & mapName );
	void selectSavedGame( int index );
	void changeMap_demo( const QString & mapName );
	void changeDemoFile_record( const QString & fileName );
	void selectDemoFile_replay( int index );

	void selectSkill( int skill );
	void changeSkillNum( int skillNum );
	void toggleNoMonsters( bool checked );
	void toggleFastMonsters( bool checked );
	void toggleMonstersRespawn( bool checked );
	void toggleAllowCheats( bool checked );

	void selectMonitor( int index );
	void changeResolutionX( const QString & xStr );
	void changeResolutionY( const QString & yStr );

	void toggleNoSound( bool checked );
	void toggleNoSFX( bool checked );
	void toggleNoMusic( bool checked );

	void changeSaveDir( const QString & dir );
	void changeScreenshotDir( const QString & dir );
	void browseSaveDir();
	void browseScreenshotDir();

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

	void exportPreset();
	void importPreset();

	void updatePresetCmdArgs( const QString & text );
	void updateGlobalCmdArgs( const QString & text );

	void launch();

 private: // methods

	void setupPresetView();
	void setupIWADView();
	void setupMapPackView();
	void setupModView();

	void loadMonitorInfo( QComboBox * box );

	void toggleAbsolutePaths( bool absolute );

	void restorePreset( int index );
	void togglePresetSubWidgets( bool enabled );
	void clearPresetSubWidgets();

	void restoreLaunchOptions( LaunchOptions & opts );

	void updateIWADsFromDir();
	void refreshMapPacks();
	void updateSaveFilesFromDir();
	void updateDemoFilesFromDir();
	void updateConfigFilesFromDir();
	void updateMapsFromIWAD();

	void toggleOptionsSubwidgets( bool enabled );

	void saveOptions( const QString & fileName );
	void loadOptions( const QString & fileName );

	void updateLaunchCommand( bool verifyPaths = false );
	QString generateLaunchCommand( const QString & baseDir, bool verifyPaths );

 private: // internal members

	Ui::MainWindow * ui;

	uint tickCount;

	bool optionsCorrupted;  ///< true when was a critical error during parsing of options file, such content should not be saved

	bool disableSelectionCallbacks;  ///< flag that temporarily disables callbacks like selectEngine(), selectConfig(), selectIWAD() \
	                                      used to prevent unnecessary or unwanted operations when updating the lists

	QString compatOptsCmdArgs;  ///< string with command line args created from compatibility options, cached so that it doesn't need to be regenerated on every command line update

	KeyPressFilter modKeyFilter;  ///< event filter that captures the presses of spacebar to correct the unsuitable default behaviour

	PathContext pathContext;  ///< stores path settings and automatically converts paths to relative or absolute

	UpdateChecker updateChecker;

 private: // user data

	bool checkForUpdates;

	OptionsStorage optsStorage;

	bool closeOnLaunch;

	// We use model-view design pattern for several widgets, because it allows us to organize the data in a way we need,
	// and have the widget (frontend) automatically mirror the underlying data (backend) without syncing them manually.
	//
	// You can read more about it here: https://doc.qt.io/qt-5/model-view-programming.html#model-subclassing-reference

	ReadOnlyListModel< Engine > engineModel;    ///< user-ordered list of engines (managed by SetupDialog)

	struct ConfigFile
	{
		QString fileName;
		ConfigFile( const QString & fileName ) : fileName( fileName ) {}
		ConfigFile( const QFileInfo & file ) : fileName( file.fileName() ) {}
	};
	ReadOnlyListModel< ConfigFile > configModel;    ///< list of config files found in pre-defined directory

	struct SaveFile
	{
		QString fileName;
		SaveFile( const QString & fileName ) : fileName( fileName ) {}
		SaveFile( const QFileInfo & file ) : fileName( file.fileName() ) {}
	};
	ReadOnlyListModel< SaveFile > saveModel;    ///< list of save files found in pre-defined directory

	struct DemoFile
	{
		QString fileName;
		DemoFile( const QString & fileName ) : fileName( fileName ) {}
		DemoFile( const QFileInfo & file ) : fileName( file.fileName() ) {}
	};
	ReadOnlyListModel< DemoFile > demoModel;    ///< list of demo files found in pre-defined directory

	ReadOnlyListModel< IWAD > iwadModel;    ///< user-ordered list of iwads (managed by SetupDialog)
	IwadSettings iwadSettings;    ///< IWAD-related preferences (value returned by SetupDialog)

	QFileSystemModel mapModel;  ///< model representing a directory with map files
	MapSettings mapSettings;    ///< map-related preferences (value returned by SetupDialog)

	EditableListModel< Mod > modModel;
	ModSettings modSettings;    ///< mod-related preferences (value returned by SetupDialog)

	EditableListModel< Preset > presetModel;    ///< user-made presets, when one is selected from the list view, it applies its stored options to the other widgets

	LaunchOptions opts;

};


#endif // MAIN_WINDOW_INCLUDED
