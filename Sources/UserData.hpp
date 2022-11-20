//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: the data user enters into the launcher
//======================================================================================================================

#ifndef USER_DATA_INCLUDED
#define USER_DATA_INCLUDED


#include "Common.hpp"

#include "Widgets/ListModel.hpp"  // ReadOnlyListModelItem, EditableListModelItem
#include "Utils/JsonUtils.hpp"    // enumName, enumSize
#include "ColorThemes.hpp"        // Theme
#include "EngineTraits.hpp"       // EngineFamily

#include <QString>
#include <QFileInfo>
#include <QRect>


//======================================================================================================================
//  OS-specific defaults

#ifdef _WIN32
	constexpr bool useAbsolutePathsByDefault = false;
	constexpr bool showEngineOutputByDefault = false;
#else
	constexpr bool useAbsolutePathsByDefault = true;
	constexpr bool showEngineOutputByDefault = true;
#endif


//======================================================================================================================
//  data definition

// Constructors from QFileInfo are used in automatic list updates for initializing an element from a file-system entry.
// getID methods are used in automatic list updates for ensuring the same items remain selected.

//----------------------------------------------------------------------------------------------------------------------
//  files

/// a ported Doom engine (source port) located somewhere on the disc
struct Engine : public EditableListModelItem
{
	QString name;        ///< user defined engine name
	QString path;        ///< path to the engine's executable
	QString configDir;   ///< directory with engine's .ini files
	EngineFamily family = EngineFamily::ZDoom;  ///< automatically detected, but user-selectable engine family

	Engine() {}
	Engine( const QFileInfo & file )
		: name( file.fileName() ), path( file.filePath() ), configDir( file.dir().path() ) {}

	// requirements of EditableListModel
	const QString & getFilePath() const { return path; }

	QString getID() const { return path; }
};

struct IWAD : public EditableListModelItem
{
	QString name;   ///< initially set to file name, but user can edit it by double-clicking on it in SetupDialog
	QString path;   ///< path to the IWAD file

	IWAD() {}
	IWAD( const QFileInfo & file ) : name( file.fileName() ), path( file.filePath() ) {}

	// requirements of EditableListModel
	const QString & getEditString() const { return name; }
	void setEditString( const QString & str ) { name = str; }
	const QString & getFilePath() const { return path; }

	QString getID() const { return path; }
};

struct Mod : public EditableListModelItem
{
	QString path;           ///< path to the mod file
	QString fileName;       ///< cached last part of path, beware of inconsistencies
	bool checked = false;   ///< whether this mod is selected to be loaded

	// requirements of EditableListModel
	const QString & getEditString() const { return fileName; }
	void setEditString( const QString & str ) { fileName = str; }
	bool isChecked() const { return checked; }
	void setChecked( bool checked ) { this->checked = checked; }
	const QString & getFilePath() const { return path; }

	Mod() {}
	Mod( const QFileInfo & file, bool checked = true )
		: path( file.filePath() ), fileName( file.fileName() ), checked( checked ) {}
};

//----------------------------------------------------------------------------------------------------------------------
//  gameplay/compatibility options

enum LaunchMode
{
	Default,
	LaunchMap,
	LoadSave,
	RecordDemo,
	ReplayDemo,
};
template<> inline const char * enumName< LaunchMode >() { return "LaunchMode"; }
template<> inline uint enumSize< LaunchMode >() { return uint( LaunchMode::ReplayDemo ) + 1; }

enum Skill
{
	TooYoungToDie = 1,
	NotTooRough = 2,
	HurtMePlenty = 3,
	UltraViolence = 4,
	Nightmare = 5,
	Custom
};
template<> inline const char * enumName< Skill >() { return "Skill"; }
template<> inline uint enumSize< Skill >() { return uint( Skill::Custom ) + 1; }

enum MultRole
{
	Server,
	Client
};
template<> inline const char * enumName< MultRole >() { return "MultRole"; }
template<> inline uint enumSize< MultRole >() { return uint( MultRole::Client ) + 1; }

enum NetMode
{
	PeerToPeer,
	PacketServer
};
template<> inline const char * enumName< NetMode >() { return "NetMode"; }
template<> inline uint enumSize< NetMode >() { return uint( NetMode::PacketServer ) + 1; }

enum GameMode
{
	Deathmatch,
	TeamDeathmatch,
	AltDeathmatch,
	AltTeamDeathmatch,
	Cooperative
};
template<> inline const char * enumName< GameMode >() { return "GameMode"; }
template<> inline uint enumSize< GameMode >() { return uint( GameMode::Cooperative ) + 1; }

struct LaunchOptions
{
	LaunchMode mode = Default;
	QString mapName;
	QString saveFile;
	QString mapName_demo;
	QString demoFile_record;
	QString demoFile_replay;
};

struct MultiplayerOptions
{
	bool isMultiplayer = false;
	MultRole multRole = Server;
	QString hostName;
	uint16_t port = 5029;
	NetMode netMode = PeerToPeer;
	GameMode gameMode = Deathmatch;
	uint playerCount = 2;
	double teamDamage = 0.0;
	uint timeLimit = 0;
	uint fragLimit = 0;
};

struct GameplayDetails
{
	int32_t dmflags1 = 0;
	int32_t dmflags2 = 0;
};

struct GameplayOptions : public GameplayDetails  // inherited instead of included to avoid long identifiers
{
	int skillNum = TooYoungToDie;
	bool noMonsters = false;
	bool fastMonsters = false;
	bool monstersRespawn = false;
	bool allowCheats = false;

	void assign( const GameplayDetails & other ) { static_cast< GameplayDetails & >( *this ) = other; }
};

struct CompatibilityDetails
{
	int32_t compatflags1 = 0;
	int32_t compatflags2 = 0;
};

struct CompatibilityOptions : public CompatibilityDetails  // inherited instead of included to avoid long identifiers
{
	int compatLevel = -1;

	void assign( const CompatibilityDetails & other ) { static_cast< CompatibilityDetails & >( *this ) = other; }
};

//----------------------------------------------------------------------------------------------------------------------
//  other options

struct AlternativePaths
{
	QString saveDir;
	QString screenshotDir;
};

struct VideoOptions
{
	int monitorIdx = 0;
	uint resolutionX = 0;
	uint resolutionY = 0;
	bool showFPS = false;
};

struct AudioOptions
{
	bool noSound = false;
	bool noSFX = false;
	bool noMusic = false;
};

struct GlobalOptions
{
	bool usePresetNameAsDir = false;
	QString cmdArgs;
};

//----------------------------------------------------------------------------------------------------------------------
//  preset

struct Preset : public EditableListModelItem
{
	QString name;

	QString selectedEnginePath;   // we store the engine by path, so that it does't break when user renames them or reorders them
	QString selectedConfig;   // we store the config by name instead of index, so that it does't break when user reorders them
	QString selectedIWAD;   // we store the IWAD by path instead of index, so that it doesn't break when user reorders them
	QList< QString > selectedMapPacks;
	QList< Mod > mods;   // this list needs to be kept in sync with mod list widget

	LaunchOptions launchOpts;
	MultiplayerOptions multOpts;
	GameplayOptions gameOpts;
	CompatibilityOptions compatOpts;
	VideoOptions videoOpts;
	AudioOptions audioOpts;
	AlternativePaths altPaths;

	QString cmdArgs;

	// requirements of EditableListModel
	const QString & getEditString() const { return name; }
	void setEditString( const QString & str ) { name = str; }

	Preset() {}
	Preset( const QString & name ) : name( name ) {}
	Preset( const QFileInfo & ) {}  // dummy, it's required by the EditableListModel template, but isn't actually used
};

//----------------------------------------------------------------------------------------------------------------------
//  global settings

struct IwadSettings
{
	QString dir;                  ///< directory to update IWAD list from (value returned by SetupDialog)
	bool updateFromDir = false;   ///< whether the IWAD list should be periodically updated from a directory
	bool searchSubdirs = false;   ///< whether to search for IWADs recursivelly in subdirectories
};

struct MapSettings
{
	QString dir;   ///< directory with map packs to automatically load the list from
};

struct ModSettings
{
	QString dir;   ///< directory with mods, starting dir for "Add mod" dialog
};

enum OptionsStorage
{
	DontStore,       ///< Everytime the launcher is closed and re-opened, the options are reset to defaults.
	StoreGlobally,   ///< When the launcher is closed, current state of the options is saved. When it's re-opened, the options are loaded from the last saved state.
	StoreToPreset,   ///< Options are stored to the currently selected preset. When a preset is selected, the options are loaded from the preset.
};
template<> inline const char * enumName< OptionsStorage >() { return "OptionsStorage"; }
template<> inline uint enumSize< OptionsStorage >() { return uint( OptionsStorage::StoreToPreset ) + 1; }

struct StorageSettings
{
	OptionsStorage launchOptsStorage = StoreGlobally;  ///< controls both LaunchOptions and MultiplayerOptions, since they are heavily tied together
	OptionsStorage gameOptsStorage = StoreGlobally;
	OptionsStorage compatOptsStorage = StoreToPreset;
	OptionsStorage videoOptsStorage = StoreGlobally;
	OptionsStorage audioOptsStorage = StoreGlobally;
};

/// Additional launcher settings
struct LauncherSettings : public StorageSettings  // inherited instead of included to avoid long identifiers
{
	bool checkForUpdates = true;
	bool useAbsolutePaths = useAbsolutePathsByDefault;
	bool closeOnLaunch = false;
	bool showEngineOutput = showEngineOutputByDefault;
	Theme theme = Theme::SystemDefault;

	void assign( const StorageSettings & other ) { static_cast< StorageSettings & >( *this ) = other; }
};

struct WindowGeometry
{
	int width;
	int height;

	WindowGeometry() {}
	WindowGeometry( const QRect & rect ) : width( rect.width() ), height( rect.height() ) {}
};


#endif // USER_DATA_INCLUDED
