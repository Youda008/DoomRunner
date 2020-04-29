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

void deserialize( JsonContext & json, Engine & engine )
{
	engine.name = json.getString( "name", "<missing name>" );
	engine.path = json.getString( "path" );
	engine.configDir = json.getString( "config_dir", QFileInfo( engine.path ).dir().path() );
}

QJsonObject serialize( const IWAD & iwad )
{
	QJsonObject jsIWAD;

	jsIWAD["name"] = iwad.name;
	jsIWAD["path"] = iwad.path;

	return jsIWAD;
}

void deserialize( JsonContext & json, IWAD & iwad )
{
	iwad.path = json.getString( "path" );
	iwad.name = json.getString( "name", QFileInfo( iwad.path ).fileName() );
}

QJsonObject serialize( const Mod & mod )
{
	QJsonObject jsMod;

	jsMod["path"] = mod.path;
	jsMod["checked"] = mod.checked;

	return jsMod;
}

void deserialize( JsonContext & json, Mod & mod )
{
	mod.path = json.getString( "path" );
	mod.checked = json.getBool( "checked", false );
}

QJsonObject serialize( const IwadSettings & iwadSettings )
{
	QJsonObject jsIWADs;

	jsIWADs["auto_update"] = iwadSettings.updateFromDir;
	jsIWADs["directory"] = iwadSettings.dir;
	jsIWADs["subdirs"] = iwadSettings.searchSubdirs;

	return jsIWADs;
}

void deserialize( JsonContext & json, IwadSettings & iwadSettings )
{
	iwadSettings.updateFromDir = json.getBool( "auto_update", false );
	iwadSettings.dir = json.getString( "directory" );
	iwadSettings.searchSubdirs = json.getBool( "subdirs", false );
}

QJsonObject serialize( const MapSettings & mapSettings )
{
	QJsonObject jsMaps;

	jsMaps["directory"] = mapSettings.dir;

	return jsMaps;
}

void deserialize( JsonContext & json, MapSettings & mapSettings )
{
	mapSettings.dir = json.getString( "directory" );
}

QJsonObject serialize( const ModSettings & modSettings )
{
	QJsonObject jsMods;

	jsMods["directory"] = modSettings.dir;

	return jsMods;
}

void deserialize( JsonContext & json, ModSettings & modSettings )
{
	modSettings.dir = json.getString( "directory" );
}

QJsonObject serialize( const LaunchOptions & opts )
{
	QJsonObject jsOptions;

	jsOptions["launch_mode"] = int( opts.mode );
	jsOptions["map_name"] = opts.mapName;
	jsOptions["save_file"] = opts.saveFile;
	jsOptions["skill_num"] = int( opts.skillNum );
	jsOptions["no_monsters"] = opts.fastMonsters;
	jsOptions["fast_monsters"] = opts.fastMonsters;
	jsOptions["monsters_respawn"] = opts.monstersRespawn;

	jsOptions["dmflags1"] = qint64( opts.gameOpts.flags1 );
	jsOptions["dmflags2"] = qint64( opts.gameOpts.flags2 );
	jsOptions["compatflags1"] = qint64( opts.compatOpts.flags1 );
	jsOptions["compatflags2"] = qint64( opts.compatOpts.flags2 );

	jsOptions["is_multiplayer"] = opts.isMultiplayer;
	jsOptions["mult_role"] = int( opts.multRole );
	jsOptions["host_name"] = opts.hostName;
	jsOptions["port"] = int( opts.port );
	jsOptions["net_mode"] = int( opts.netMode );
	jsOptions["game_mode"] = int( opts.gameMode );
	jsOptions["player_count"] = int( opts.playerCount );
	jsOptions["team_damage"] = opts.teamDamage;
	jsOptions["time_limit"] = int( opts.timeLimit );

	return jsOptions;
}

void deserialize( JsonContext & json, LaunchOptions & opts )
{
	opts.mode = json.getEnum< LaunchMode >( "launch_mode", opts.mode );
	opts.mapName = json.getString( "map_name", opts.mapName );
	opts.saveFile = json.getString( "save_file", opts.saveFile );
	opts.skillNum = json.getUInt( "skill_num", opts.skillNum );
	opts.noMonsters = json.getBool( "no_monsters", opts.noMonsters );
	opts.fastMonsters = json.getBool( "fast_monsters", opts.fastMonsters );
	opts.monstersRespawn = json.getBool( "monsters_respawn", opts.monstersRespawn );

	opts.gameOpts.flags1 = json.getInt( "dmflags1", opts.gameOpts.flags1 );
	opts.gameOpts.flags2 = json.getInt( "dmflags2", opts.gameOpts.flags2 );
	opts.compatOpts.flags1 = json.getInt( "compatflags1", opts.compatOpts.flags1 );
	opts.compatOpts.flags2 = json.getInt( "compatflags2", opts.compatOpts.flags2 );

	opts.isMultiplayer = json.getBool( "is_multiplayer", opts.isMultiplayer );
	opts.multRole = json.getEnum< MultRole >( "mult_role", opts.multRole );
	opts.hostName = json.getString( "host_name", opts.hostName );
	opts.port = json.getUInt16( "port", opts.port );
	opts.netMode = json.getEnum< NetMode >( "net_mode", opts.netMode );
	opts.gameMode = json.getEnum< GameMode >( "game_mode", opts.gameMode );
	opts.playerCount = json.getUInt( "player_count", opts.playerCount );
	opts.teamDamage = json.getDouble( "team_damage", opts.teamDamage );
	opts.timeLimit = json.getUInt( "time_limit", opts.timeLimit );
}

QJsonObject serialize( const Preset & preset )
{
	QJsonObject jsPreset;

	jsPreset["name"] = preset.name;
	jsPreset["selected_engine"] = preset.selectedEnginePath;
	jsPreset["selected_config"] = preset.selectedConfig;
	jsPreset["selected_IWAD"] = preset.selectedIWAD;

	QJsonArray jsMapArray;
	for (const TreePosition & pos : preset.selectedMapPacks) {
		jsMapArray.append( pos.toString() );
	}
	jsPreset["selected_mappacks"] = jsMapArray;

	jsPreset["mods"] = serializeList( preset.mods );

	jsPreset["additional_args"] = preset.cmdArgs;

	return jsPreset;
}

void deserialize( JsonContext & json, Preset & preset )
{
	preset.name = json.getString( "name", "<missing name>" );
	preset.selectedEnginePath = json.getString( "selected_engine" );
	preset.selectedConfig = json.getString( "selected_config" );
	preset.selectedIWAD = json.getString( "selected_IWAD" );

	if (json.enterArray( "selected_mappacks" ))
	{
		for (int i = 0; i < json.arraySize(); i++) {
			QString selectedMapPack = json.getString( i );
			if (!selectedMapPack.isEmpty()) {
				preset.selectedMapPacks.append( TreePosition( selectedMapPack ) );
			}
		}
		json.exitArray();
	}

	if (json.enterArray( "mods" ))
	{
		for (int i = 0; i < json.arraySize(); i++)
		{
			if (!json.enterObject( i ))  // wrong type on position i - skip this entry
				continue;

			Mod mod;
			deserialize( json, mod );

			if (mod.path.isEmpty())  // element isn't present in JSON -> skip this entry
				continue;

			mod.fileName = QFileInfo( mod.path ).fileName();

			preset.mods.append( std::move( mod ) );

			json.exitObject();
		}
		json.exitArray();
	}


	preset.cmdArgs = json.getString( "additional_args" );
}
