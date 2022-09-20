//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
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

	if (engine.isSeparator)
	{
		jsEngine["separator"] = true;
		jsEngine["name"] = engine.name;
	}
	else
	{
		jsEngine["name"] = engine.name;
		jsEngine["path"] = engine.path;
		jsEngine["config_dir"] = engine.configDir;
		jsEngine["family"] = familyToStr( engine.family );
	}

	return jsEngine;
}

void deserialize( const JsonObjectCtx & jsEngine, Engine & engine )
{
	engine.isSeparator = jsEngine.getBool( "separator", false, false );
	if (engine.isSeparator)
	{
		engine.name = jsEngine.getString( "name", "<missing name>" );
	}
	else
	{
		engine.name = jsEngine.getString( "name", "<missing name>" );
		engine.path = jsEngine.getString( "path" );
		engine.configDir = jsEngine.getString( "config_dir", getDirOfFile( engine.path ) );
		engine.family = familyFromStr( jsEngine.getString( "family", "", DontShowError ) );
		if (engine.family == EngineFamily::_EnumEnd)
			engine.family = guessEngineFamily( getFileBasenameFromPath( engine.path ) );
	}
}

QJsonObject serialize( const IWAD & iwad )
{
	QJsonObject jsIWAD;

	if (iwad.isSeparator)
	{
		jsIWAD["separator"] = true;
		jsIWAD["name"] = iwad.name;
	}
	else
	{
		jsIWAD["name"] = iwad.name;
		jsIWAD["path"] = iwad.path;
	}

	return jsIWAD;
}

void deserialize( const JsonObjectCtx & jsEngine, IWAD & iwad )
{
	iwad.isSeparator = jsEngine.getBool( "separator", false, false );
	if (iwad.isSeparator)
	{
		iwad.name = jsEngine.getString( "name", "<missing name>" );
	}
	else
	{
		iwad.path = jsEngine.getString( "path" );
		iwad.name = jsEngine.getString( "name", QFileInfo( iwad.path ).fileName() );
	}
}

QJsonObject serialize( const Mod & mod )
{
	QJsonObject jsMod;

	if (mod.isSeparator)
	{
		jsMod["separator"] = true;
		jsMod["name"] = mod.fileName;
	}
	else
	{
		jsMod["path"] = mod.path;
		jsMod["checked"] = mod.checked;
	}

	return jsMod;
}

void deserialize( const JsonObjectCtx & jsMod, Mod & mod )
{
	mod.isSeparator = jsMod.getBool( "separator", false, false );
	if (mod.isSeparator)
	{
		mod.fileName = jsMod.getString( "name", "<missing name>" );
	}
	else
	{
		mod.path = jsMod.getString( "path" );
		mod.fileName = QFileInfo( mod.path ).fileName();
		mod.checked = jsMod.getBool( "checked", false );
	}
}

QJsonObject serialize( const IwadSettings & iwadSettings )
{
	QJsonObject jsIWADs;

	jsIWADs["auto_update"] = iwadSettings.updateFromDir;
	jsIWADs["directory"] = iwadSettings.dir;
	jsIWADs["subdirs"] = iwadSettings.searchSubdirs;

	return jsIWADs;
}

void deserialize( const JsonObjectCtx & jsIWADs, IwadSettings & iwadSettings )
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

void deserialize( const JsonObjectCtx & jsMaps, MapSettings & mapSettings )
{
	mapSettings.dir = jsMaps.getString( "directory" );
}

QJsonObject serialize( const ModSettings & modSettings )
{
	QJsonObject jsMods;

	jsMods["directory"] = modSettings.dir;

	return jsMods;
}

void deserialize( const JsonObjectCtx & jsMods, ModSettings & modSettings )
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
	jsOptions["map_name_demo"] = opts.mapName_demo;
	jsOptions["demo_file_record"] = opts.demoFile_record;
	jsOptions["demo_file_replay"] = opts.demoFile_replay;

	// gameplay
	jsOptions["skill_num"] = int( opts.skillNum );
	jsOptions["no_monsters"] = opts.noMonsters;
	jsOptions["fast_monsters"] = opts.fastMonsters;
	jsOptions["monsters_respawn"] = opts.monstersRespawn;
	jsOptions["dmflags1"] = qint64( opts.gameOpts.flags1 );
	jsOptions["dmflags2"] = qint64( opts.gameOpts.flags2 );
	jsOptions["compatflags1"] = qint64( opts.compatOpts.flags1 );
	jsOptions["compatflags2"] = qint64( opts.compatOpts.flags2 );
	jsOptions["compat_level"] = opts.compatLevel;
	jsOptions["allow_cheats"] = opts.allowCheats;

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

	// alternative paths
	jsOptions["save_dir"] = opts.saveDir;
	jsOptions["screenshot_dir"] = opts.screenshotDir;

	return jsOptions;
}

void deserialize( const JsonObjectCtx & jsOptions, LaunchOptions & opts )
{
	// launch mode
	opts.mode = jsOptions.getEnum< LaunchMode >( "launch_mode", opts.mode );
	opts.mapName = jsOptions.getString( "map_name", opts.mapName );
	opts.saveFile = jsOptions.getString( "save_file", opts.saveFile );
	opts.mapName_demo = jsOptions.getString( "map_name_demo", opts.mapName_demo );
	opts.demoFile_record = jsOptions.getString( "demo_file_record", opts.demoFile_record );
	opts.demoFile_replay = jsOptions.getString( "demo_file_replay", opts.demoFile_replay );

	// gameplay
	opts.skillNum = jsOptions.getUInt( "skill_num", opts.skillNum );
	opts.noMonsters = jsOptions.getBool( "no_monsters", opts.noMonsters );
	opts.fastMonsters = jsOptions.getBool( "fast_monsters", opts.fastMonsters );
	opts.monstersRespawn = jsOptions.getBool( "monsters_respawn", opts.monstersRespawn );
	opts.gameOpts.flags1 = jsOptions.getInt( "dmflags1", opts.gameOpts.flags1 );
	opts.gameOpts.flags2 = jsOptions.getInt( "dmflags2", opts.gameOpts.flags2 );
	opts.compatOpts.flags1 = jsOptions.getInt( "compatflags1", opts.compatOpts.flags1 );
	opts.compatOpts.flags2 = jsOptions.getInt( "compatflags2", opts.compatOpts.flags2 );
	opts.compatLevel = jsOptions.getInt( "compat_level", opts.compatLevel, DontShowError );
	opts.allowCheats = jsOptions.getBool( "allow_cheats", opts.allowCheats );

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

	// alternative paths
	opts.saveDir = jsOptions.getString( "save_dir", opts.saveDir );
	opts.screenshotDir = jsOptions.getString( "screenshot_dir", opts.screenshotDir );
}

QJsonObject serialize( const OutputOptions & opts )
{
	QJsonObject jsOptions;

	// video
	jsOptions["monitor_idx"] = opts.monitorIdx;
	jsOptions["resolution_x"] = qint64( opts.resolutionX );
	jsOptions["resolution_y"] = qint64( opts.resolutionY );
	jsOptions["show_fps"] = opts.showFPS;

	// audio
	jsOptions["no_sound"] = opts.noSound;
	jsOptions["no_sfx"] = opts.noSFX;
	jsOptions["no_music"] = opts.noMusic;

	return jsOptions;
}

void deserialize( const JsonObjectCtx & jsOptions, OutputOptions & opts )
{
	// video
	opts.monitorIdx = jsOptions.getInt( "monitor_idx", opts.monitorIdx );
	opts.resolutionX = jsOptions.getUInt( "resolution_x", opts.resolutionX );
	opts.resolutionY = jsOptions.getUInt( "resolution_y", opts.resolutionY );
	opts.showFPS = jsOptions.getBool( "show_fps", opts.showFPS, DontShowError );

	// audio
	opts.noSound = jsOptions.getBool( "no_sound", opts.noSound );
	opts.noSFX = jsOptions.getBool( "no_sfx", opts.noSFX );
	opts.noMusic = jsOptions.getBool( "no_music", opts.noMusic );
}

QJsonObject serialize( const Preset & preset, bool storeOpts )
{
	QJsonObject jsPreset;

	if (preset.isSeparator)
	{
		jsPreset["separator"] = true;
		jsPreset["name"] = preset.name;
	}
	else
	{
		jsPreset["name"] = preset.name;
		jsPreset["selected_engine"] = preset.selectedEnginePath;
		jsPreset["selected_config"] = preset.selectedConfig;
		jsPreset["selected_IWAD"] = preset.selectedIWAD;

		QJsonArray jsMapArray;
		for (const QString & pos : preset.selectedMapPacks)
		{
			jsMapArray.append( pos );
		}
		jsPreset["selected_mappacks"] = jsMapArray;

		jsPreset["mods"] = serializeList( preset.mods );

		if (storeOpts)
		{
			jsPreset["launch_options"] = serialize( preset.launchOpts );
		}

		jsPreset["additional_args"] = preset.cmdArgs;
	}

	return jsPreset;
}

void deserialize( const JsonObjectCtx & jsPreset, Preset & preset, bool loadOpts )
{
	preset.isSeparator = jsPreset.getBool( "separator", false, false );
	if (preset.isSeparator)
	{
		preset.name = jsPreset.getString( "name", "<missing name>" );
	}
	else
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
					preset.selectedMapPacks.append( selectedMapPack );
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

				if (!mod.isSeparator && mod.path.isEmpty())  // element isn't present in JSON -> skip this entry
					continue;
				else if (mod.isSeparator && mod.fileName.isEmpty())  // element isn't present in JSON -> skip this entry
					continue;

				preset.mods.append( std::move( mod ) );
			}
		}

		if (loadOpts)
		{
			if (JsonObjectCtx jsOptions = jsPreset.getObject( "launch_options" ))
			{
				deserialize( jsOptions, preset.launchOpts );
			}
		}

		preset.cmdArgs = jsPreset.getString( "additional_args" );
	}
}

// We want to serialize this into an existing JSON object instead of its own one, for compatibility reasons.

void serialize( QJsonObject & jsOptions, const GlobalOptions & opts )
{
	jsOptions["use_preset_name_as_dir"] = opts.usePresetNameAsDir;
	jsOptions["additional_args"] = opts.cmdArgs;
}

void deserialize( const JsonObjectCtx & jsOptions, GlobalOptions & opts )
{
	opts.usePresetNameAsDir = jsOptions.getBool( "use_preset_name_as_dir", opts.usePresetNameAsDir, DontShowError );
	opts.cmdArgs = jsOptions.getString( "additional_args" );
}

void serialize( QJsonObject & jsOptions, const LauncherOptions & opts )
{
	jsOptions["check_for_updates"] = opts.checkForUpdates;
	jsOptions["use_absolute_paths"] = opts.useAbsolutePaths;
	jsOptions["options_storage"] = int( opts.launchOptsStorage );
	jsOptions["close_on_launch"] = opts.closeOnLaunch;
	jsOptions["show_engine_output"] = opts.showEngineOutput;
}

void deserialize( const JsonObjectCtx & jsOptions, LauncherOptions & opts )
{
	opts.checkForUpdates = jsOptions.getBool( "check_for_updates", true, DontShowError );
	opts.useAbsolutePaths = jsOptions.getBool( "use_absolute_paths", useAbsolutePathsByDefault );
	opts.launchOptsStorage = jsOptions.getEnum< OptionsStorage >( "options_storage", StoreGlobally );
	opts.closeOnLaunch = jsOptions.getBool( "close_on_launch", false, DontShowError );
	opts.showEngineOutput = jsOptions.getBool( "show_engine_output", false, DontShowError );
}
