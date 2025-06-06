//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: the data user enters into the launcher
//======================================================================================================================

#ifndef USER_DATA_INCLUDED
#define USER_DATA_INCLUDED


#include "Essential.hpp"

#include "DataModels/AModelItem.hpp"        // AModelItem - all list items inherit from this
#include "Utils/PtrList.hpp"                // PtrList
#include "Utils/EnumTraits.hpp"             // enumName, enumSize
#include "Utils/FileSystemUtilsTypes.hpp"   // PathStyle
#include "Utils/OSUtilsTypes.hpp"           // EnvVar
#include "EngineTraits.hpp"                 // EngineFamily
#include "Themes.hpp"                       // Theme

#include <QString>
#include <QFileInfo>
#include <QDir>
#include <QRect>  // WindowGeometry
#include <QColor>  // player color in multiplayer

template<> inline const char * enumName< Qt::SortOrder >() { return "Qt::SortOrder"; }
template<> inline int enumSize< Qt::SortOrder >() { return 2; }


//======================================================================================================================
// OS-specific defaults

#if IS_WINDOWS
	constexpr PathStyle defaultPathStyle = PathStyle::Relative;
	constexpr bool showEngineOutputByDefault = false;
#else
	constexpr PathStyle defaultPathStyle = PathStyle::Absolute;
	constexpr bool showEngineOutputByDefault = true;
#endif


//======================================================================================================================
// user data definition

// Constructors from QFileInfo are used in automatic list updates for initializing an element from a file-system entry.
// getID() methods are used in automatic list updates for ensuring the same items remain selected.

//----------------------------------------------------------------------------------------------------------------------
// files

/// a ported Doom engine (source port) located somewhere on the disc
struct Engine : public AModelItem
{
	QString name;            ///< user defined engine name
	QString executablePath;  ///< path to the engine's executable
	QString configDir;       ///< directory with engine's config files, style of this path is kept as the user entered it
	QString dataDir;         ///< directory for engine's data files (save files, demo files, ...), style of this path is kept as the user entered it
	EngineFamily family = EngineFamily::ZDoom;  ///< automatically detected, but user-selectable engine family

	Engine() {}
	// When file is dropped from a file explorer: The other members have to be auto-detected later by EngineTraits.
	explicit Engine( const QFileInfo & file ) : executablePath( file.filePath() ) {}

	// requirements of GenericListModel
	const QString & getFilePath() const   { return executablePath; }
	const QString & getID() const         { return executablePath; }
	QJsonObject serialize() const;
	bool deserialize( const JsonObjectCtx & modJs );
};

struct IWAD : public AModelItem
{
	QString name;   ///< initially set to file name, but user can edit it by double-clicking on it in SetupDialog
	QString path;   ///< path to the IWAD file

	IWAD() {}
	explicit IWAD( const QString & filePath ) : IWAD( QFileInfo( filePath ) ) {}
	explicit IWAD( const QFileInfo & file ) : name( file.fileName() ), path( file.filePath() ) {}

	// requirements of GenericListModel
	bool isEditable() const                 { return true; }
	const QString & getEditString() const   { return name; }
	void setEditString( QString str )       { name = std::move(str); }
	const QString & getFilePath() const     { return path; }
	const QString & getID() const           { return path; }
	QJsonObject serialize() const;
	bool deserialize( const JsonObjectCtx & modJs );
};

struct Mod : public AModelItem
{
	QString path;           ///< path to the mod file
	QString name;           ///< cached last part of path, beware of inconsistencies
	bool checked = false;   ///< whether this mod is selected to be loaded
	bool isCmdArg = false;  ///< indicates that this is a special item used to insert a custom command line argument between the mod files

	explicit Mod( bool checked = false ) : checked( checked ) {}
	explicit Mod( const QString & filePath, bool checked = true ) : Mod( QFileInfo( filePath ), checked ) {}
	explicit Mod( const QFileInfo & file, bool checked = true )
		: path( file.filePath() ), name( file.fileName() ), checked( checked ) {}

	// requirements of GenericListModel
	bool isEditable() const                 { return isCmdArg; }
	const QString & getEditString() const   { return name; }
	void setEditString( QString str )       { name = std::move(str); }
	bool isCheckable() const                { return true; }
	bool isChecked() const                  { return checked; }
	void setChecked( bool checked )         { this->checked = checked; }
	const QString & getFilePath() const     { return path; }
	const QIcon & getIcon() const;
	QJsonObject serialize() const;
	bool deserialize( const JsonObjectCtx & modJs );
};

//----------------------------------------------------------------------------------------------------------------------
// gameplay/compatibility options

enum LaunchMode
{
	Default,
	LaunchMap,
	LoadSave,
	RecordDemo,
	ReplayDemo,
	ResumeDemo,
};
template<> inline const char * enumName< LaunchMode >() { return "LaunchMode"; }
template<> inline int enumSize< LaunchMode >() { return size_t( LaunchMode::ResumeDemo ) + 1; }

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
template<> inline int enumSize< Skill >() { return size_t( Skill::Custom ) + 1; }

enum MultRole
{
	Server,
	Client
};
template<> inline const char * enumName< MultRole >() { return "MultRole"; }
template<> inline int enumSize< MultRole >() { return size_t( MultRole::Client ) + 1; }

enum NetMode
{
	PeerToPeer,
	PacketServer
};
template<> inline const char * enumName< NetMode >() { return "NetMode"; }
template<> inline int enumSize< NetMode >() { return size_t( NetMode::PacketServer ) + 1; }

enum GameMode
{
	Deathmatch,
	TeamDeathmatch,
	AltDeathmatch,
	AltTeamDeathmatch,
	Deathmatch3,
	Cooperative
};
template<> inline const char * enumName< GameMode >() { return "GameMode"; }
template<> inline int enumSize< GameMode >() { return size_t( GameMode::Cooperative ) + 1; }

struct LaunchOptions
{
	LaunchMode mode = Default;
	QString mapName;
	QString saveFile;
	QString mapName_demo;
	QString demoFile_record;
	QString demoFile_replay;
	QString demoFile_resumeFrom;
	QString demoFile_resumeTo;
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
	QString playerName;
	QColor playerColor;
};

using GameFlags = int32_t;  // ZDoom uses signed int (the highest flag turns the number into negative), so let's be consistent with that

struct GameplayDetails
{
	GameFlags dmflags1 = 0;
	GameFlags dmflags2 = 0;
	GameFlags dmflags3 = 0;  // only in GZDoom 4.11.0+
};

struct GameplayOptions : public GameplayDetails  // inherited instead of included to avoid long identifiers
{
	int skillIdx = TooYoungToDie;  ///< value in the skill combo-box (only from TooYoungToDie=1 to Custom=6)
	int skillNum = skillIdx;  ///< value in the skill spin-box (when skillIdx == Custom, it can be any number)
	bool noMonsters = false;
	bool fastMonsters = false;
	bool monstersRespawn = false;
	bool pistolStart = false;
	bool allowCheats = false;

	void assign( const GameplayDetails & other ) { static_cast< GameplayDetails & >( *this ) = other; }
};

struct CompatibilityDetails
{
	GameFlags compatflags1 = 0;
	GameFlags compatflags2 = 0;
};

struct CompatibilityOptions : public CompatibilityDetails  // inherited instead of included to avoid long identifiers
{
	int compatMode = -1;

	void assign( const CompatibilityDetails & other ) { static_cast< CompatibilityDetails & >( *this ) = other; }
};

//----------------------------------------------------------------------------------------------------------------------
// other options

struct AlternativePaths
{
	// these are always relative either to Engine::configDir or Engine::dataDir
	QString configDir;
	QString saveDir;
	QString demoDir;
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
using EnvVars = QList< os::EnvVar >;

struct GlobalOptions
{
	bool usePresetNameAsConfigDir = false;
	bool usePresetNameAsSaveDir = false;
	bool usePresetNameAsDemoDir = false;
	bool usePresetNameAsScreenshotDir = false;
	QString cmdArgs;
	QString cmdPrefix;
	EnvVars envVars;
};

//----------------------------------------------------------------------------------------------------------------------
// preset

struct Preset : public AModelItem
{
	QString name;

	QString selectedEnginePath;   // we store the engine by path, so that it does't break when user renames them or reorders them
	QString selectedConfig;   // we store the config by name instead of index, so that it does't break when user reorders them
	QString selectedIWAD;   // we store the IWAD by path instead of index, so that it doesn't break when user reorders them
	QStringList selectedMapPacks;
	PtrList< Mod > mods;   // this list needs to be kept in sync with mod list widget
	bool loadMapsAfterMods = false;

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
	Preset( const QFileInfo & ) {}  // dummy, it's required by the GenericListModel template, but isn't actually used

	// requirements of GenericListModel
	bool isEditable() const                 { return true; }
	const QString & getEditString() const   { return name; }
	void setEditString( QString str )       { name = std::move(str); }
	const QString & getID() const           { return name; }
};

//----------------------------------------------------------------------------------------------------------------------
// global settings

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
	int sortColumn = 0;
	Qt::SortOrder sortOrder = Qt::AscendingOrder;
	bool showIcons = false;   ///< whether the map list should show file-system icons provided by the OS
};

struct ModSettings
{
	QString lastUsedDir;
	bool showIcons = true;   ///< whether the mod list should show file-system icons provided by the OS
};

enum OptionsStorage
{
	DontStore,       ///< Everytime the launcher is closed and re-opened, the options are reset to defaults.
	StoreGlobally,   ///< When the launcher is closed, current state of the options is saved. When it's re-opened, the options are loaded from the last saved state.
	StoreToPreset,   ///< Options are stored to the currently selected preset. When a preset is selected, the options are loaded from the preset.
};
template<> inline const char * enumName< OptionsStorage >() { return "OptionsStorage"; }
template<> inline int enumSize< OptionsStorage >() { return size_t( OptionsStorage::StoreToPreset ) + 1; }

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
	bool closeOutputOnSuccess = false;
	bool checkForUpdates = true;
	bool askForSandboxPermissions = true;
	bool hideMapHelpLabel = false;
	bool wrapLinesInTxtViewer = false;

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
// derived data
//
// Strictly-speaking, this does not belong here because this data is not user-specified but automatically determined.
// But it is related to the data above and it is used across multiple dialogs, so it is acceptable to be here.

// This combines user-defined and automatically determined engine information under a single struct for simpler processing.
// Inheritance is used instead of composition, so that we can write engine.exeAppName() instead of engine.traits.exeAppName().
struct EngineInfo : public Engine, public EngineTraits
{
	using Engine::Engine;
	explicit EngineInfo( const Engine & engine )       { static_cast< Engine & >( *this ) = engine; }
	explicit EngineInfo( Engine && engine ) noexcept   { static_cast< Engine & >( *this ) = std::move( engine ); }

	QString getDefaultSaveDir( const QString & IWADPath = {} ) const
	{
		QString saveSubdir = EngineTraits::getDefaultSaveSubdir( IWADPath );
		if (!Engine::dataDir.isEmpty())
			return QDir( Engine::dataDir ).filePath( saveSubdir );
		else
			return saveSubdir;
	}
};


#endif // USER_DATA_INCLUDED
