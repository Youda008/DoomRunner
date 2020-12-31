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
#include "DirTreeModel.hpp"
#include "FileSystemUtils.hpp"  // PathHelper
#include "EventFilters.hpp"  // ConfirmationFilter
#include "UpdateChecker.hpp"

#include <QMainWindow>
#include <QString>
#include <QFileInfo>

class QListView;

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

	void runSetupDialog();
	void runGameOptsDialog();
	void runCompatOptsDialog();
	void runAboutDialog();

	void loadPreset( const QModelIndex & index );

	void selectEngine( int index );
	void selectConfig( int index );
	void toggleIWAD( const QModelIndex & index );
	void toggleMapPack( const QModelIndex & index );
	void toggleMod( const QModelIndex & index );

	void showMapPackDesc( const QModelIndex & index );

	void presetAdd();
	void presetDelete();
	void presetClone();
	void presetMoveUp();
	void presetMoveDown();
	void presetConfirm();

	void modAdd();
	void modDelete();
	void modMoveUp();
	void modMoveDown();
	void modsDropped( int row, int count );
	void modConfirm();

	void iwadConfirm();
	void mapPackConfirm();

	void modeStandard();
	void modeLaunchMap();
	void modeSavedGame();

	void selectMap( const QString & map );
	void selectSavedGame( int index );
	void selectSkill( int skill );
	void changeSkillNum( int skillNum );
	void toggleNoMonsters( bool checked );
	void toggleFastMonsters( bool checked );
	void toggleMonstersRespawn( bool checked );

	void toggleMultiplayer( bool checked );
	void selectMultRole( int role );
	void changeHost( const QString & host );
	void changePort( int port );
	void selectNetMode( int mode );
	void selectGameMode( int mode );
	void changePlayerCount( int count );
	void changeTeamDamage( double damage );
	void changeTimeLimit( int limit );

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

	void toggleAbsolutePaths( bool absolute );

	void togglePresetSubWidgets( bool enabled );
	void clearPresetSubWidgets();

	void restoreLaunchOptions( const LaunchOptions & opts );

	void updateListsFromDirs();
	void updateIWADsFromDir();
	void updateMapPacksFromDir();
	void updateSaveFilesFromDir();
	void updateConfigFilesFromDir();
	void updateMapsFromIWAD();

	void saveOptions( const QString & fileName );
	void loadOptions( const QString & fileName );

	void updateLaunchCommand();
	QString generateLaunchCommand( QString baseDir = "" );

 private: // internal members

	Ui::MainWindow * ui;

	uint tickCount;

	bool optionsCorrupted;  ///< true when was a critical error during parsing of options file, such content should not be saved

	QString compatOptsCmdArgs;    ///< string with command line args created from compatibility options, cached so that it doesn't need to be regenerated on every command line update

	PathHelper pathHelper;  ///< stores path settings and automatically converts paths to relative or absolute

	ConfirmationFilter presetConfirmationFilter;
	ConfirmationFilter modConfirmationFilter;
	ConfirmationFilter iwadConfirmationFilter;
	ConfirmationFilter mapConfirmationFilter;

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
		ConfigFile( const QFileInfo & file ) : fileName( file.fileName() ) {}
	};
	ReadOnlyListModel< ConfigFile > configModel;    ///< list of config files found in pre-defined directory

	struct SaveFile
	{
		QString fileName;
		SaveFile( const QFileInfo & file ) : fileName( file.fileName() ) {}
	};
	ReadOnlyListModel< SaveFile > saveModel;    ///< list of save files found in pre-defined directory

	ReadOnlyListModel< IWAD > iwadModel;    ///< user-ordered list of iwads (managed by SetupDialog)
	IwadSettings iwadSettings;    ///< IWAD-related preferences (value returned by SetupDialog)

	DirTreeModel mapModel;    ///< model owning a tree structure representing a directory with map files
	MapSettings mapSettings;    ///< map-related preferences (value returned by SetupDialog)

	EditableListModel< Mod > modModel;
	ModSettings modSettings;    ///< mod-related preferences (value returned by SetupDialog)

	EditableListModel< Preset > presetModel;    ///< user-made presets, when one is selected from the list view, it applies its stored options to the other widgets

	LaunchOptions opts;

};


#endif // MAIN_WINDOW_INCLUDED
