//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: the data user enters into the launcher
//======================================================================================================================

#ifndef USER_DATA_INCLUDED
#define USER_DATA_INCLUDED


#include "Common.hpp"

#include "ListModel.hpp"  // ReadOnlyListModelItem, EditableListModelItem
#include "JsonUtils.hpp"
#include "EngineProperties.hpp"  // EngineFamily

#include <QString>
#include <QVector>
#include <QFileInfo>
#include <QDir>


//======================================================================================================================
//  data definition

// Constructors from QFileInfo are used in automatic list updates for initializing an element from a file-system entry.
// getID methods are used in automatic list updates for ensuring the same items remain selected.

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
	DontStore,       ///< Everytime the launcher is closed and re-opened, the launch options are reset to defaults.
	StoreGlobally,   ///< When the launcher is closed, current state of launch options is saved. When it's re-opened, launch options are loaded from the last saved state.
	StoreToPreset,   ///< Launch options are stored to the currently selected preset. When a preset is selected, launch options are loaded from the preset.
};
template<> inline const char * enumName< OptionsStorage >() { return "OptionsStorage"; }
template<> inline uint enumSize< OptionsStorage >() { return uint( OptionsStorage::StoreToPreset ) + 1; }

enum LaunchMode
{
	Standard,
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

struct GameplayOptions
{
	int32_t flags1 = 0;
	int32_t flags2 = 0;
};

struct CompatibilityOptions
{
	int32_t flags1 = 0;
	int32_t flags2 = 0;
};

/// data of the second tab
struct LaunchOptions
{
	// launch mode
	LaunchMode mode = Standard;
	QString mapName;
	QString saveFile;
	QString mapName_demo;
	QString demoFile_record;
	QString demoFile_replay;

	// gameplay
	uint skillNum = uint( TooYoungToDie );
	bool noMonsters = false;
	bool fastMonsters = false;
	bool monstersRespawn = false;
	GameplayOptions gameOpts = { 0, 0 };
	CompatibilityOptions compatOpts = { 0, 0 };
	int compatLevel = -1;
	bool allowCheats = false;

	// multiplayer
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

	// alternative paths
	QString saveDir;
	QString screenshotDir;
};

struct OutputOptions
{
	// video
	int monitorIdx = 0;
	uint resolutionX = 0;
	uint resolutionY = 0;
	bool showFPS = false;

	// audio
	bool noSound = false;
	bool noSFX = false;
	bool noMusic = false;
};

/// misc data that never go to a preset
struct GlobalOptions
{
	QString cmdArgs;
};

struct Preset : public EditableListModelItem
{
	QString name;
	QString selectedEnginePath;   // we store the engine by path, so that it does't break when user renames them or reorders them
	QString selectedConfig;   // we store the config by name instead of index, so that it does't break when user reorders them
	QString selectedIWAD;   // we store the IWAD by path instead of index, so that it doesn't break when user reorders them
	QList< QString > selectedMapPacks;
	QList< Mod > mods;   // this list needs to be kept in sync with mod list widget
	QString cmdArgs;
	LaunchOptions launchOpts;

	// requirements of EditableListModel
	const QString & getEditString() const { return name; }
	void setEditString( const QString & str ) { name = str; }

	Preset() {}
	Preset( const QString & name ) : name( name ) {}
	Preset( const QFileInfo & ) {}  // dummy, it's required by the EditableListModel template, but isn't actually used
};

#ifdef _WIN32
	static const bool useAbsolutePathsByDefault = false;
#else
	static const bool useAbsolutePathsByDefault = true;
#endif

/// Additional launcher settings
struct LauncherOptions
{
	bool checkForUpdates = true;
	bool useAbsolutePaths = useAbsolutePathsByDefault;
	OptionsStorage launchOptsStorage = StoreGlobally;
	bool closeOnLaunch = false;
	bool showEngineOutput = false;
};


//======================================================================================================================
//  data serialization

QJsonObject serialize( const Engine & engine );
QJsonObject serialize( const IWAD & iwad );
QJsonObject serialize( const Mod & mod );
QJsonObject serialize( const IwadSettings & iwadSettings );
QJsonObject serialize( const MapSettings & mapSettings );
QJsonObject serialize( const ModSettings & modSettings );
QJsonObject serialize( const LaunchOptions & options );
QJsonObject serialize( const OutputOptions & options );
QJsonObject serialize( const Preset & preset, bool storeOpts );
void serialize( QJsonObject & jsOptions, const GlobalOptions & options );
void serialize( QJsonObject & jsOptions, const LauncherOptions & options );

template< typename Elem >
QJsonArray serializeList( const QList< Elem > & list )
{
	QJsonArray jsArray;
	for (const Elem & elem : list)
	{
		jsArray.append( serialize( elem ) );
	}
	return jsArray;
}

void deserialize( const JsonObjectCtx & jsEngine, Engine & engine );
void deserialize( const JsonObjectCtx & jsIWAD, IWAD & iwad );
void deserialize( const JsonObjectCtx & jsMod, Mod & mod );
void deserialize( const JsonObjectCtx & jsIWADs, IwadSettings & iwadSettings );
void deserialize( const JsonObjectCtx & jsMaps, MapSettings & mapSettings );
void deserialize( const JsonObjectCtx & jsMods, ModSettings & modSettings );
void deserialize( const JsonObjectCtx & jsOptions, LaunchOptions & options );
void deserialize( const JsonObjectCtx & jsOptions, OutputOptions & options );
void deserialize( const JsonObjectCtx & jsPreset, Preset & preset, bool loadOpts );
void deserialize( const JsonObjectCtx & jsOptions, GlobalOptions & options );
void deserialize( const JsonObjectCtx & jsOptions, LauncherOptions & options );


#endif // USER_DATA_INCLUDED
