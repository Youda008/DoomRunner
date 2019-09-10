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

 private:

	virtual void showEvent( QShowEvent * event ) override;
	virtual void timerEvent( QTimerEvent * event ) override;
	virtual void closeEvent( QCloseEvent * event ) override;

 private slots:

	void runSetupDialog();
	void runGameOptsDialog();
	void runCompatOptsDialog();

	void selectPreset( const QModelIndex & index );
	void selectEngine( int index );
	void selectConfig( int index );
	void toggleIWAD( const QModelIndex & index );
	void toggleMapPack( const QModelIndex & index );
	void toggleMod( const QModelIndex & index );

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

	void updateIWADsFromDir( QListView * view );
	void updateMapPacksFromDir();
	void updateSaveFilesFromDir();
	void updateConfigFilesFromDir();
	void updateMapsFromIWAD();
	void updateListsFromDirs();

	void toggleAbsolutePaths( bool absolute );

	void modeGameMenu();
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

	void generateLaunchCommand();

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
	// the underlying list (backend) with the widget list (frontend), and also because the data can be shared in
	// multiple widgets, even across multiple windows/dialogs.
	//
	// Model and its underlying list are separated, the model doesn't hold the list inside itself.
	// It's because we want to display the same data differently in different widgets or different dialogs.
	// Therefore the models are merely mediators between the data and views,
	// which presents the data to the views and propagate user input from the views back to data.

	// engine info
	QList< Engine > engines;    ///< user-ordered list of engines (managed by SetupDialog)
	ReadOnlyListModel< Engine > engineModel;    ///< wrapper around list of engines mediating their names to the engine combo box

	// config files
	QList< QString > configs;    ///< list of config files found inside directory of selected engine
	ReadOnlyListModel< QString > configModel;    ///< wrapper around list of configs mediating their names to the config combo box

	// IWAD info
	QList< IWAD > iwads;    ///< user-ordered list of iwads (managed by SetupDialog)
	ReadOnlyListModel< IWAD > iwadModel;    ///< wrapper around list of iwads mediating their names to the iwad list view
	bool iwadListFromDir;    ///< whether the IWAD list should be periodically updated from a directory (value returned by SetupDialog)
	QString iwadDir;    ///< directory to update IWAD list from (value returned by SetupDialog)
	bool iwadSubdirs;    ///< whether to search for IWADs recursivelly in subdirectories
	int selectedIWADIdx;    ///< which IWAD was selected last (workaround to allow user to deselect IWAD by clicking it again)

	// map pack info
	QList< MapPack > maps;    ///< list of map packs automatically loaded from a directory
	ReadOnlyListModel< MapPack > mapModel;    ///< wrapper around list of map packs mediating their names to the map pack list view
	QString mapDir;    ///< directory with map packs to automatically load the list from (value returned by SetupDialog)
	bool mapSubdirs;    ///< whether to search for maps recursivelly in subdirectories
	int selectedPackIdx;    ///< which map pack was selected last (workaround to allow user to deselect map pack by clicking it again)

	// mod info
	struct Mod {
		QString name;
		QString path;
		bool checked;
	};
	QList< Mod > mods;
	EditableListModel< Mod > modModel;
	QString modDir;    ///< directory with mods, starting dir for "Add mod" dialog (value returned by SetupDialog)

	// presets
	struct Preset {
		QString name;
		QString selectedEnginePath;  // we store the engine by path, so that it does't break when user renames them or reorders them
		QString selectedConfig;  // we store the engine by name, so that it does't break when user reorders them
		QString selectedIWAD;  // we store the IWAD by name, so that it doesn't break when user reorders them
		//QString selectedMapPack;
		QList< Mod > mods;  // this list needs to be kept in sync with mod list widget
	};
	QList< Preset > presets;    ///< user-made presets, when one is selected from the list view, it applies its stored options to the other widgets
	EditableListModel< Preset > presetModel;    ///< wrapper around list of presets mediating their names to the editable preset list view

	// options managed by GameOptsDialog and CompatOptsDialog
	GameplayOptions gameOpts;
	CompatibilityOptions compatOpts;
	QString compatOptsCmdArgs;    ///< string with command line args created from compatibility options, cached so that it doesn't need to be regenerated on every command line update

};


#endif // MAIN_WINDOW_INCLUDED
