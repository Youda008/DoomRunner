//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  14.5.2019
// Description: the data user enters into the launcher
//======================================================================================================================

#ifndef USER_DATA_INCLUDED
#define USER_DATA_INCLUDED


#include "Common.hpp"

#include "ListModel.hpp"
#include "JsonUtils.hpp"

#include <QString>
#include <QVector>
#include <QFileInfo>
#include <QDir>


//======================================================================================================================
//  data definition

// Constructors from QFileInfo are used in automatic list updates for initializing an element from a file-system entry.
// getID methods are used in automatic list updates for ensuring the same items remain selected.

struct Engine : public EditableListModelItem
{
	QString name;        ///< user defined engine name
	QString path;        ///< path to the engine's executable
	QString configDir;   ///< directory with engine's .ini files

	Engine() {}
	Engine( const QFileInfo & file )
		: name( file.fileName() ), path( file.filePath() ), configDir( file.dir().path() ) {}

	const QString & getFilePath() const { return path; }

	QString getID() const { return path; }
};

struct IWAD : public EditableListModelItem
{
	QString name;   ///< initially set to file name, but user can edit it by double-clicking on it in SetupDialog
	QString path;   ///< path to the IWAD file

	IWAD() {}
	IWAD( const QFileInfo & file ) : name( file.fileName() ), path( file.filePath() ) {}

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
	QString dir;          ///< directory to update IWAD list from (value returned by SetupDialog)
	bool updateFromDir;   ///< whether the IWAD list should be periodically updated from a directory
	bool searchSubdirs;   ///< whether to search for IWADs recursivelly in subdirectories
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
	DONT_STORE,       ///< Everytime the launcher is closed and re-opened, the launch options are reset to defaults.
	STORE_GLOBALLY,   ///< When the launcher is closed, current state of launch options is saved. When it's re-opened, launch options are loaded from the last saved state.
	STORE_TO_PRESET   ///< Launch options are stored to the currently selected preset. When a preset is selected, launch options are loaded from the preset.
};
template<> inline const char * enumName< OptionsStorage >() { return "OptionsStorage"; }
template<> inline uint enumSize< OptionsStorage >() { return uint( OptionsStorage::STORE_TO_PRESET ) + 1; }

enum LaunchMode
{
	STANDARD,
	LAUNCH_MAP,
	LOAD_SAVE,
	RECORD_DEMO,
	REPLAY_DEMO
};
template<> inline const char * enumName< LaunchMode >() { return "LaunchMode"; }
template<> inline uint enumSize< LaunchMode >() { return uint( LaunchMode::REPLAY_DEMO ) + 1; }

enum Skill
{
	TOO_YOUNG_TO_DIE,
	NOT_TOO_ROUGH,
	HURT_ME_PLENTY,
	ULTRA_VIOLENCE,
	NIGHTMARE,
	CUSTOM
};
template<> inline const char * enumName< Skill >() { return "Skill"; }
template<> inline uint enumSize< Skill >() { return uint( Skill::CUSTOM ) + 1; }

enum MultRole
{
	SERVER,
	CLIENT
};
template<> inline const char * enumName< MultRole >() { return "MultRole"; }
template<> inline uint enumSize< MultRole >() { return uint( MultRole::CLIENT ) + 1; }

enum NetMode
{
	PEER_TO_PEER,
	PACKET_SERVER
};
template<> inline const char * enumName< NetMode >() { return "NetMode"; }
template<> inline uint enumSize< NetMode >() { return uint( NetMode::PACKET_SERVER ) + 1; }

enum GameMode
{
	DEATHMATCH,
	TEAM_DEATHMATCH,
	ALT_DEATHMATCH,
	ALT_TEAM_DEATHMATCH,
	COOPERATIVE
};
template<> inline const char * enumName< GameMode >() { return "GameMode"; }
template<> inline uint enumSize< GameMode >() { return uint( GameMode::COOPERATIVE ) + 1; }

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

struct LaunchOptions
{
	// launch mode
	LaunchMode mode = STANDARD;
	QString mapName;
	QString saveFile;
	QString mapName_demo;
	QString demoFile_record;
	QString demoFile_replay;

	// gameplay
	uint skillNum = uint( TOO_YOUNG_TO_DIE );
	bool noMonsters = false;
	bool fastMonsters = false;
	bool monstersRespawn = false;
	GameplayOptions gameOpts = { 0, 0 };
	CompatibilityOptions compatOpts = { 0, 0 };
	bool allowCheats = false;

	// alternative paths
	QString saveDir;
	QString screenshotDir;

	// multiplayer
	bool isMultiplayer = false;
	MultRole multRole = SERVER;
	QString hostName;
	uint16_t port = 5029;
	NetMode netMode = PEER_TO_PEER;
	GameMode gameMode = DEATHMATCH;
	uint playerCount = 2;
	double teamDamage = 0.0;
	uint timeLimit = 0;
	uint fragLimit = 0;
};

struct OutputOptions
{
	// video
	int monitorIdx;
	uint resolutionX;
	uint resolutionY;

	// audio
	bool noSound = false;
	bool noSFX = false;
	bool noMusic = false;
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
	LaunchOptions opts;

	const QString & getEditString() const { return name; }
	void setEditString( const QString & str ) { name = str; }

	Preset() {}
	Preset( const QString & name ) : name( name ) {}
	Preset( const QFileInfo & ) {}  // dummy, it's required by the EditableListModel template, but isn't actually used
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

void deserialize( JsonObjectCtx & jsEngine, Engine & engine );
void deserialize( JsonObjectCtx & jsIWAD, IWAD & iwad );
void deserialize( JsonObjectCtx & jsMod, Mod & mod );
void deserialize( JsonObjectCtx & jsIWADs, IwadSettings & iwadSettings );
void deserialize( JsonObjectCtx & jsMaps, MapSettings & mapSettings );
void deserialize( JsonObjectCtx & jsMods, ModSettings & modSettings );
void deserialize( JsonObjectCtx & jsOptions, LaunchOptions & options );
void deserialize( JsonObjectCtx & jsOptions, OutputOptions & options );
void deserialize( JsonObjectCtx & jsPreset, Preset & preset, bool loadOpts );


#endif // USER_DATA_INCLUDED
