//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  14.5.2019
// Description: the data user enters into the launcher
//======================================================================================================================

#include "UserData.hpp"

#include "JsonUtils.hpp"

#include <QJsonObject>
#include <QJsonArray>


//======================================================================================================================
//  data serialization

QJsonObject serialize( const Engine & engine )
{
	QJsonObject jsEngine;

	jsEngine["name"] = engine.name;
	jsEngine["path"] = engine.path;
	jsEngine["config_dir"] = engine.configDir;

	return jsEngine;
}

void deserialize( JsonObjectCtx & jsEngine, Engine & engine )
{
	engine.name = jsEngine.getString( "name", "<missing name>" );
	engine.path = jsEngine.getString( "path" );
	engine.configDir = jsEngine.getString( "config_dir", QFileInfo( engine.path ).dir().path() );
}

QJsonObject serialize( const IWAD & iwad )
{
	QJsonObject jsIWAD;

	jsIWAD["name"] = iwad.name;
	jsIWAD["path"] = iwad.path;

	return jsIWAD;
}

void deserialize( JsonObjectCtx & jsEngine, IWAD & iwad )
{
	iwad.path = jsEngine.getString( "path" );
	iwad.name = jsEngine.getString( "name", QFileInfo( iwad.path ).fileName() );
}

QJsonObject serialize( const Mod & mod )
{
	QJsonObject jsMod;

	jsMod["path"] = mod.path;
	jsMod["checked"] = mod.checked;

	return jsMod;
}

void deserialize( JsonObjectCtx & jsMod, Mod & mod )
{
	mod.path = jsMod.getString( "path" );
	mod.checked = jsMod.getBool( "checked", false );
}

QJsonObject serialize( const IwadSettings & iwadSettings )
{
	QJsonObject jsIWADs;

	jsIWADs["auto_update"] = iwadSettings.updateFromDir;
	jsIWADs["directory"] = iwadSettings.dir;
	jsIWADs["subdirs"] = iwadSettings.searchSubdirs;

	return jsIWADs;
}

void deserialize( JsonObjectCtx & jsIWADs, IwadSettings & iwadSettings )
{
	iwadSettings.updateFromDir = jsIWADs.getBool( "auto_update", false );
	iwadSettings.dir = jsIWADs.getString( "directory" );
	iwadSettings.searchSubdirs = jsIWADs.getBool( "subdirs", false );
}

QJsonObject serialize( const MapSettings & mapSettings )
{
	QJsonObject jsMaps;

	jsMaps["directory"] = mapSettings.dir;

	return jsMaps;
}

void deserialize( JsonObjectCtx & jsMaps, MapSettings & mapSettings )
{
	mapSettings.dir = jsMaps.getString( "directory" );
}

QJsonObject serialize( const ModSettings & modSettings )
{
	QJsonObject jsMods;

	jsMods["directory"] = modSettings.dir;

	return jsMods;
}

void deserialize( JsonObjectCtx & jsMods, ModSettings & modSettings )
{
	modSettings.dir = jsMods.getString( "directory" );
}

QJsonObject serialize( const LaunchOptions & opts )
{
	QJsonObject jsOptions;

	// launch mode
	jsOptions["launch_mode"] = int( opts.mode );
	jsOptions["map_name"] = opts.mapName;
	jsOptions["save_file"] = opts.saveFile;

	// gameplay
	jsOptions["skill_num"] = int( opts.skillNum );
	jsOptions["no_monsters"] = opts.noMonsters;
	jsOptions["fast_monsters"] = opts.fastMonsters;
	jsOptions["monsters_respawn"] = opts.monstersRespawn;
	jsOptions["dmflags1"] = qint64( opts.gameOpts.flags1 );
	jsOptions["dmflags2"] = qint64( opts.gameOpts.flags2 );
	jsOptions["compatflags1"] = qint64( opts.compatOpts.flags1 );
	jsOptions["compatflags2"] = qint64( opts.compatOpts.flags2 );
	jsOptions["allow_cheats"] = opts.allowCheats;

	// video
	jsOptions["monitor_idx"] = opts.monitorIdx;
	jsOptions["resolution_x"] = qint64( opts.resolutionX );
	jsOptions["resolution_y"] = qint64( opts.resolutionY );

	// audio
	jsOptions["no_sound"] = opts.noSound;
	jsOptions["no_sfx"] = opts.noSFX;
	jsOptions["no_music"] = opts.noMusic;

	// alternative paths
	jsOptions["save_dir"] = opts.saveDir;
	jsOptions["screenshot_dir"] = opts.screenshotDir;

	// multiplayer
	jsOptions["is_multiplayer"] = opts.isMultiplayer;
	jsOptions["mult_role"] = int( opts.multRole );
	jsOptions["host_name"] = opts.hostName;
	jsOptions["port"] = int( opts.port );
	jsOptions["net_mode"] = int( opts.netMode );
	jsOptions["game_mode"] = int( opts.gameMode );
	jsOptions["player_count"] = int( opts.playerCount );
	jsOptions["team_damage"] = opts.teamDamage;
	jsOptions["time_limit"] = int( opts.timeLimit );
	jsOptions["frag_limit"] = int( opts.fragLimit );

	return jsOptions;
}

void deserialize( JsonObjectCtx & jsOptions, LaunchOptions & opts )
{
	// launch mode
	opts.mode = jsOptions.getEnum< LaunchMode >( "launch_mode", opts.mode );
	opts.mapName = jsOptions.getString( "map_name", opts.mapName );
	opts.saveFile = jsOptions.getString( "save_file", opts.saveFile );

	// gameplay
	opts.skillNum = jsOptions.getUInt( "skill_num", opts.skillNum );
	opts.noMonsters = jsOptions.getBool( "no_monsters", opts.noMonsters );
	opts.fastMonsters = jsOptions.getBool( "fast_monsters", opts.fastMonsters );
	opts.monstersRespawn = jsOptions.getBool( "monsters_respawn", opts.monstersRespawn );
	opts.gameOpts.flags1 = jsOptions.getInt( "dmflags1", opts.gameOpts.flags1 );
	opts.gameOpts.flags2 = jsOptions.getInt( "dmflags2", opts.gameOpts.flags2 );
	opts.compatOpts.flags1 = jsOptions.getInt( "compatflags1", opts.compatOpts.flags1 );
	opts.compatOpts.flags2 = jsOptions.getInt( "compatflags2", opts.compatOpts.flags2 );
	opts.allowCheats = jsOptions.getBool( "allow_cheats", opts.allowCheats );

	// video
	opts.monitorIdx = jsOptions.getInt( "monitor_idx", opts.monitorIdx );
	opts.resolutionX = jsOptions.getUInt( "resolution_x", opts.resolutionX );
	opts.resolutionY = jsOptions.getUInt( "resolution_y", opts.resolutionY );

	// audio
	opts.noSound = jsOptions.getBool( "no_sound", opts.noSound );
	opts.noSFX = jsOptions.getBool( "no_sfx", opts.noSFX );
	opts.noMusic = jsOptions.getBool( "no_music", opts.noMusic );

	// alternative paths
	opts.saveDir = jsOptions.getString( "save_dir", opts.saveDir );
	opts.screenshotDir = jsOptions.getString( "screenshot_dir", opts.screenshotDir );

	// multiplayer
	opts.isMultiplayer = jsOptions.getBool( "is_multiplayer", opts.isMultiplayer );
	opts.multRole = jsOptions.getEnum< MultRole >( "mult_role", opts.multRole );
	opts.hostName = jsOptions.getString( "host_name", opts.hostName );
	opts.port = jsOptions.getUInt16( "port", opts.port );
	opts.netMode = jsOptions.getEnum< NetMode >( "net_mode", opts.netMode );
	opts.gameMode = jsOptions.getEnum< GameMode >( "game_mode", opts.gameMode );
	opts.playerCount = jsOptions.getUInt( "player_count", opts.playerCount );
	opts.teamDamage = jsOptions.getDouble( "team_damage", opts.teamDamage );
	opts.timeLimit = jsOptions.getUInt( "time_limit", opts.timeLimit );
	opts.fragLimit = jsOptions.getUInt( "frag_limit", opts.fragLimit );
}

QJsonObject serialize( const Preset & preset, bool storeOpts )
{
	QJsonObject jsPreset;

	jsPreset["name"] = preset.name;
	jsPreset["selected_engine"] = preset.selectedEnginePath;
	jsPreset["selected_config"] = preset.selectedConfig;
	jsPreset["selected_IWAD"] = preset.selectedIWAD;

	QJsonArray jsMapArray;
	for (const TreePosition & pos : preset.selectedMapPacks)
	{
		jsMapArray.append( pos.toString() );
	}
	jsPreset["selected_mappacks"] = jsMapArray;

	jsPreset["mods"] = serializeList( preset.mods );

	if (storeOpts)
	{
		jsPreset["options"] = serialize( preset.opts );
	}

	jsPreset["additional_args"] = preset.cmdArgs;

	return jsPreset;
}

void deserialize( JsonObjectCtx & jsPreset, Preset & preset, bool loadOpts )
{
	preset.name = jsPreset.getString( "name", "<missing name>" );
	preset.selectedEnginePath = jsPreset.getString( "selected_engine" );
	preset.selectedConfig = jsPreset.getString( "selected_config" );
	preset.selectedIWAD = jsPreset.getString( "selected_IWAD" );

	if (JsonArrayCtx jsSelectedMapPacks = jsPreset.getArray( "selected_mappacks" ))
	{
		for (int i = 0; i < jsSelectedMapPacks.size(); i++)
		{
			QString selectedMapPack = jsSelectedMapPacks.getString( i );
			if (!selectedMapPack.isEmpty())
			{
				preset.selectedMapPacks.append( TreePosition( selectedMapPack ) );
			}
		}
	}

	if (JsonArrayCtx jsMods = jsPreset.getArray( "mods" ))
	{
		for (int i = 0; i < jsMods.size(); i++)
		{
			JsonObjectCtx jsMod = jsMods.getObject( i );
			if (!jsMod)  // wrong type on position i - skip this entry
				continue;

			Mod mod;
			deserialize( jsMod, mod );

			if (mod.path.isEmpty())  // element isn't present in JSON -> skip this entry
				continue;

			mod.fileName = QFileInfo( mod.path ).fileName();

			preset.mods.append( std::move( mod ) );
		}
	}

	if (loadOpts)
	{
		if (JsonObjectCtx jsOptions = jsPreset.getObject( "options" ))
		{
			deserialize( jsOptions, preset.opts );
		}
	}

	preset.cmdArgs = jsPreset.getString( "additional_args" );
}
