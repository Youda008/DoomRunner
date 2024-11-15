//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: the data user enters into the launcher
//======================================================================================================================

#ifndef USER_DATA_INCLUDED
#define USER_DATA_INCLUDED


#include "Essential.hpp"

#include "Widgets/ListModel.hpp"     // ReadOnlyListModelItem, EditableListModelItem
#include "Utils/JsonUtils.hpp"       // enumName, enumSize
#include "Utils/FileSystemUtils.hpp" // PathStyle
#include "Utils/OSUtils.hpp"         // EnvVar
#include "EngineTraits.hpp"          // EngineFamily
#include "Themes.hpp"                // Theme

#include <QString>
#include <QFileInfo>
#include <QRect>


//======================================================================================================================
//  OS-specific defaults

#if IS_WINDOWS
	constexpr PathStyle defaultPathStyle = PathStyle::Relative;
	constexpr bool showEngineOutputByDefault = false;
#else
	constexpr PathStyle defaultPathStyle = PathStyle::Absolute;
	constexpr bool showEngineOutputByDefault = true;
#endif


//======================================================================================================================
//  user data definition

// Constructors from QFileInfo are used in automatic list updates for initializing an element from a file-system entry.
// getID() methods are used in automatic list updates for ensuring the same items remain selected.

//----------------------------------------------------------------------------------------------------------------------
//  files

/// a ported Doom engine (source port) located somewhere on the disc
struct Engine : public EditableListModelItem
{
	QString name;            ///< user defined engine name
	QString executablePath;  ///< path to the engine's executable
	QString configDir;       ///< directory with engine's .ini files
	QString dataDir;         ///< directory for engine's data files (save files, demo files, ...)
	EngineFamily family = EngineFamily::ZDoom;  ///< automatically detected, but user-selectable engine family

	Engine() {}
	// When file is dropped from a file explorer: The other members have to be auto-detected later by EngineTraits.
	Engine( const QFileInfo & file ) : executablePath( file.filePath() ) {}

	// requirements of EditableListModel
	const QString & getFilePath() const   { return executablePath; }
	QString getID() const                 { return executablePath; }
};

struct IWAD : public EditableListModelItem
{
	QString name;   ///< initially set to file name, but user can edit it by double-clicking on it in SetupDialog
	QString path;   ///< path to the IWAD file

	IWAD() {}
	IWAD( const QFileInfo & file ) : name( file.fileName() ), path( file.filePath() ) {}

	// requirements of EditableListModel
	bool isEditable() const                 { return true; }
	const QString & getEditString() const   { return name; }
	void setEditString( QString str )       { name = std::move(str); }
	const QString & getFilePath() const     { return path; }
	QString getID() const                   { return path; }
};

struct Mod : public EditableListModelItem
{
	QString path;           ///< path to the mod file
	QString fileName;       ///< cached last part of path, beware of inconsistencies
	bool checked = true;    ///< whether this mod is selected to be loaded
	bool isCmdArg = false;  ///< indicates that this is a special item used to insert a custom command line argument between the mod files

	Mod() {}
	Mod( const QFileInfo & file, bool checked = true )
		: path( file.filePath() ), fileName( file.fileName() ), checked( checked ) {}

	// requirements of EditableListModel
	bool isEditable() const                 { return isCmdArg; }
	const QString & getEditString() const   { return fileName; }
	void setEditString( QString str )       { fileName = std::move(str); }
	bool isCheckable() const                { return true; }
	bool isChecked() const                  { return checked; }
	void setChecked( bool checked )         { this->checked = checked; }
	const QString & getFilePath() const     { return path; }
	const QIcon & getIcon() const;
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
	int32_t dmflags3 = 0;  // only in GZDoom 4.11.0+
};

struct GameplayOptions : public GameplayDetails  // inherited instead of included to avoid long identifiers
{
	int skillIdx = TooYoungToDie;  ///< value in the skill combo-box (only from TooYoungToDie=1 to Custom=6)
	int skillNum = skillIdx;  ///< value in the skill spin-box (when skillIdx == Custom, it can be any number)
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

// While storing the environment variables in a map is more logical, in our application we need to have it as
// a linear list, otherwise it's impossible to update the data structure when the user changes the variable name (key).
// The list should also retain the order the user gave it.
using EnvVars = QVector< os::EnvVar >;

struct GlobalOptions
{
	bool usePresetNameAsDir = false;
	QString cmdArgs;
	EnvVars envVars;
};

//----------------------------------------------------------------------------------------------------------------------
//  preset

struct Preset : public EditableListModelItem
{
	QString name;

	QString selectedEnginePath;   // we store the engine by path, so that it does't break when user renames them or reorders them
	QString selectedConfig;   // we store the config by name instead of index, so that it does't break when user reorders them
	QString selectedIWAD;   // we store the IWAD by path instead of index, so that it doesn't break when user reorders them
	QStringVec selectedMapPacks;
	QList< Mod > mods;   // this list needs to be kept in sync with mod list widget

	LaunchOptions launchOpts;
	MultiplayerOptions multOpts;
	GameplayOptions gameOpts;
	CompatibilityOptions compatOpts;
	VideoOptions videoOpts;
	AudioOptions audioOpts;
	AlternativePaths altPaths;

	QString cmdArgs;

	EnvVars envVars;

	Preset() {}
	Preset( const QString & name ) : name( name ) {}
	Preset( const QFileInfo & ) {}  // dummy, it's required by the EditableListModel template, but isn't actually used

	// requirements of EditableListModel
	bool isEditable() const                 { return true; }
	const QString & getEditString() const   { return name; }
	void setEditString( QString str )       { name = std::move(str); }
	QString getID() const                   { return name; }
};

//----------------------------------------------------------------------------------------------------------------------
//  global settings

struct EngineSettings
{
	QString defaultEngine;
};

struct IwadSettings
{
	QString dir;                  ///< directory to update IWAD list from (value returned by SetupDialog)
	bool updateFromDir = false;   ///< whether the IWAD list should be periodically updated from a directory
	bool searchSubdirs = false;   ///< whether to search for IWADs recursivelly in subdirectories
	QString defaultIWAD;
};

struct MapSettings
{
	QString dir;   ///< directory with map packs to automatically load the list from
};

struct ModSettings
{
	QString dir;   ///< directory with mods, starting dir for "Add mod" dialog
	bool showIcons = true;   ///< whether the mod list should show file-system icons provided by the OS
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

/// settings of the launcher's behaviour
struct LauncherSettings : public StorageSettings  // inherited instead of included to avoid long identifiers
{
	PathStyle pathStyle = defaultPathStyle;
	bool showEngineOutput = showEngineOutputByDefault;
	bool closeOnLaunch = false;
	bool checkForUpdates = true;
	bool askForSandboxPermissions = true;

	void assign( const StorageSettings & other ) { static_cast< StorageSettings & >( *this ) = other; }
};

struct WindowGeometry
{
	int x = INT_MIN;
	int y = INT_MIN;  // 0 is a valid coordinate so we need to use something else to indicate default
	int width = 0;
	int height = 0;

	WindowGeometry() {}
	WindowGeometry( const QRect & r ) : x( r.x() ), y( r.y() ), width( r.width() ), height( r.height() ) {}
};

// separated from the LauncherSettings for reasons explained at MainWindow initialization
struct AppearanceSettings
{
	WindowGeometry geometry;
	QString appStyle;
	ColorScheme colorScheme = ColorScheme::SystemDefault;
};


//======================================================================================================================
//  derived data
//
//  Strictly-speaking, this does not belong here because this data is not user-specified but automatically determined.
//  But it is related to the data above and it is used across multiple dialogs, so it is acceptable to be here.

// This combines user-defined and automatically determined engine information under a single struct for simpler processing.
// Inheritance is used instead of composition, so that we can write engine.exeAppName() instead of engine.traits.exeAppName().
struct EngineInfo : public Engine, public EngineTraits
{
	os::SandboxEnvInfo sandboxEnv;

	using Engine::Engine;
	EngineInfo( const Engine & engine ) { static_cast< Engine & >( *this ) = engine; }
	EngineInfo( Engine && engine )      { static_cast< Engine & >( *this ) = std::move( engine ); }

	void initSandboxEnvInfo( const QString & executablePath_ )
	{
		sandboxEnv = os::getSandboxEnvInfo( executablePath_ );
	}
	bool hasSandboxEnvInfo() const
	{
		return !sandboxEnv.appName.isNull();
	}
};


#endif // USER_DATA_INCLUDED
