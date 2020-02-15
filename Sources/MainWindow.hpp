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

#include "SharedData.hpp"
#include "ItemModels.hpp"
#include "Utils.hpp"

#include <QMainWindow>
#include <QString>
#include <QList>

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

 private: //methods

	virtual void showEvent( QShowEvent * event ) override;
	virtual void timerEvent( QTimerEvent * event ) override;
	virtual void closeEvent( QCloseEvent * event ) override;

	void firstRun();

 private slots:

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

	void togglePresetSubWidgets( bool enabled );

	void presetAdd();
	void presetDelete();
	void presetClone();
	void presetMoveUp();
	void presetMoveDown();
	void presetDropped();

	void modAdd();
	void modDelete();
	void modMoveUp();
	void modMoveDown();
	void modsDropped();

	void updateIWADsFromDir();
	void updateMapPacksFromDir();
	void updateSaveFilesFromDir();
	void updateConfigFilesFromDir();
	void updateMapsFromIWAD();
	void updateListsFromDirs();

	void toggleAbsolutePaths( bool absolute );

	void modeStandard();
	void modeSelectedMap();
	void modeSavedGame();

	void selectMap( QString map );
	void selectSavedGame( QString saveName );
	void selectSkill( int skill );
	void changeSkillNum( int skillNum );
	void toggleNoMonsters( bool checked );
	void toggleFastMonsters( bool checked );
	void toggleMonstersRespawn( bool checked );

	void toggleMultiplayer( bool checked );
	void selectMultRole( int role );
	void changeHost( QString host );
	void changePort( int port );
	void selectNetMode( int mode );
	void selectGameMode( int mode );
	void changePlayerCount( int count );
	void changeTeamDamage( double damage );
	void changeTimeLimit( int limit );

	void saveOptions( QString fileName );
	void loadOptions( QString fileName );

	void exportPreset();
	void importPreset();

	void updateAdditionalArgs( QString text );

	QString generateLaunchCommand( QString baseDir = "" );
	void updateLaunchCommand();

	void launch();

 private: //members

	Ui::MainWindow * ui;

	// workaround for the Qt not showing the final position in window constructor
	bool shown;
	int width;  // these dimensions are the ones loaded from the options file
	int height;

	uint tickCount;

	PathHelper pathHelper;  ///< stores path settings and automatically converts paths to relative or absolute

	// We use model-view design pattern for several list widgets, because it allows us to have all the related data
	// packed together in one struct, and have the UI automatically mirror the underlying list without manually syncing
	// the underlying list (backend) with the widget list (frontend).
	//
	// You can read more about it here: https://doc.qt.io/qt-5/model-view-programming.html#model-subclassing-reference

	ReadOnlyListModel< Engine > engineModel;    ///< user-ordered list of engines (managed by SetupDialog)

	ReadOnlyListModel< QString > configModel;    ///< list of config files found inside directory of selected engine

	ReadOnlyListModel< IWAD > iwadModel;    ///< user-ordered list of iwads (managed by SetupDialog)
	bool iwadListFromDir;    ///< whether the IWAD list should be periodically updated from a directory (value returned by SetupDialog)
	QString iwadDir;    ///< directory to update IWAD list from (value returned by SetupDialog)
	bool iwadSubdirs;    ///< whether to search for IWADs recursivelly in subdirectories
	QString selectedIWAD;    ///< which IWAD was selected last (workaround to allow user to deselect IWAD by clicking it again)

	TreeModel mapModel;    ///< model owning a tree structure representing a directory with map files
	QString mapDir;    ///< directory with map packs to automatically load the list from (value returned by SetupDialog)
	TreePath selectedMapPack;    ///< which map pack was selected last (workaround to allow user to deselect map pack by clicking it again)

	EditableListModel< Mod > modModel;
	QString modDir;    ///< directory with mods, starting dir for "Add mod" dialog (value returned by SetupDialog)

	EditableListModel< Preset > presetModel;    ///< user-made presets, when one is selected from the list view, it applies its stored options to the other widgets

	// options managed by GameOptsDialog and CompatOptsDialog
	GameplayOptions gameOpts;
	CompatibilityOptions compatOpts;
	QString compatOptsCmdArgs;    ///< string with command line args created from compatibility options, cached so that it doesn't need to be regenerated on every command line update

};


#endif // MAIN_WINDOW_INCLUDED
