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
#include <QHash>

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

	virtual void closeEvent( QCloseEvent * event ) override;
	virtual void timerEvent( QTimerEvent * event ) override;
	virtual void showEvent( QShowEvent * event ) override;

 private slots:

	void runSetupDialog();
	void runDMFlagsDialog();
	void runCompatFlagsDialog();

	void selectEngine( int index );

	void selectPreset( const QModelIndex & index );
	void toggleIWAD( const QModelIndex & index );
	void toggleMapPack( const QModelIndex & index );
	void toggleMod( QListWidgetItem * item );

	void presetAdd();
	void presetDelete();
	void presetMoveUp();
	void presetMoveDown();

	void modAdd();
	void modDelete();
	void modMoveUp();
	void modMoveDown();
	void addModByPath( QString path );

	void updateIWADsFromDir();
	void updateMapPacksFromDir();
	void updateSaveFilesFromDir();
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

	PathHelper pathHelper;

	// We use model-view design pattern for several list widgets, because it allows us to have all the related data
	// packed together in one struct, and have the UI automatically mirror the underlying list without manually syncing
	// the underlying list (backend) with the widget list (frontend), and also because the data can be shared in
	// multiple widgets, even across multiple windows/dialogs.

	// engine info
	QList< Engine > engines;    ///< user-ordered list of engines (managed by SetupDialog)
	ObjectListROModel< Engine, makeEngineUITextFromName > engineModel;    ///< wrapper around list of engines mediating their names to the engine combo box

	// IWAD info
	QList< IWAD > iwads;    ///< user-ordered list of iwads (managed by SetupDialog)
	ObjectListROModel< IWAD, makeIwadUITextFromName > iwadModel;    ///< wrapper around list of iwads mediating their names to the iwad list view
	bool iwadListFromDir;    ///< whether the IWAD list should be periodically updated from a directory (value returned by SetupDialog)
	QString iwadDir;    ///< directory to update IWAD list from (value returned by SetupDialog)
	int selectedIWADIdx;    ///< which IWAD was selected last (workaround to allow user to deselect IWAD by clicking it again)

	// map pack info
	struct MapPack {
		QString name;
	};
	static QString makeMapPackUIText( const MapPack & pack ) { return pack.name; }
	QList< MapPack > maps;    ///< list of map packs automatically loaded from a directory
	ObjectListROModel< MapPack, makeMapPackUIText > mapModel;    ///< wrapper around list of map packs mediating their names to the map pack list view
	QString mapDir;    ///< directory with map packs to automatically load the list from (value returned by SetupDialog)
	int selectedPackIdx;    ///< which map pack was selected last (workaround to allow user to deselect map pack by clicking it again)

	// mod info
	struct ModInfo {
		QString path;
	};
	// using model-view architecture with checkable items is too hard, TODO
	// so we rather use list widget and map the string values from it into mod info struct
	QHash< QString, ModInfo > modInfo;
	QString modDir;    ///< directory with mods, starting dir for "Add mod" dialog (value returned by SetupDialog)

	// presets
	struct Preset {
		QString name;
		QString selectedEnginePath;  // we store the engine by path, so that it does't break when user renames them or reorders them
		QString selectedIWAD;  // we store the IWAD by name, so that it doesn't break when user reorders them
		//QString selectedMapPack;
		struct ModEntry {
			QString name;
			ModInfo info;
			bool checked;
		};
		QList< ModEntry > mods;  // this list needs to be kept in sync with mod list widget
	};
	QList< Preset > presets;    ///< user-made presets, when one is selected from the list view, it applies its stored options to the other widgets
	ObjectListRWModel< Preset > presetModel;    ///< wrapper around list of presets mediating their names to the editable preset list view

	// gameplay options (values returned by DMFlagsDialog and CompatFlagsDialog)
	uint32_t dmflags1;
	uint32_t dmflags2;
	uint32_t compatflags1;
	uint32_t compatflags2;

};


#endif // MAIN_WINDOW_INCLUDED
