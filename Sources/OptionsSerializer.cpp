//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: serialization/deserialization of the user data into/from a file
//======================================================================================================================

#include "OptionsSerializer.hpp"

#include "CommonTypes.hpp"
#include "Version.hpp"
#include "Utils/JsonUtils.hpp"
#include "Utils/MiscUtils.hpp"  // checkPath, highlightInvalidListItem
#include "Utils/ErrorHandling.hpp"

#include <QFileInfo>


//======================================================================================================================
//  custom data types

static QJsonObject serialize( const EnvVars & envVars )
{
	QJsonObject jsEnvVars;

	// convert list to a map
	for (const auto & envVar : envVars)
	{
		jsEnvVars[ envVar.name ] = envVar.value;
	}

	return jsEnvVars;
}

static void deserialize( const JsonObjectCtx & jsEnvVars, EnvVars & envVars )
{
	// convert map to a list
	auto keys = jsEnvVars.keys();
	for (auto & varName : keys)
	{
		QString value = jsEnvVars.getString( varName, {} );
		envVars.append({ std::move(varName), std::move(value) });
	}

	// keep the list sorted
	std::sort( envVars.begin(), envVars.end(), []( const auto & a, const auto & b ) { return a.name < b.name; });
}


//======================================================================================================================
//  user data sub-sections

static QJsonObject serialize( const Engine & engine )
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
		jsEngine["path"] = engine.executablePath;
		jsEngine["config_dir"] = engine.configDir;
		jsEngine["data_dir"] = engine.dataDir;
		jsEngine["family"] = familyToStr( engine.family );
	}

	return jsEngine;
}

static void deserialize( const JsonObjectCtx & jsEngine, Engine & engine )
{
	engine.isSeparator = jsEngine.getBool( "separator", false, DontShowError );
	if (engine.isSeparator)
	{
		engine.name = jsEngine.getString( "name", "<missing name>" );
	}
	else
	{
		engine.name = jsEngine.getString( "name", "<missing name>" );
		engine.executablePath = jsEngine.getString( "path", {} );  // empty path is used to indicate invalid entry to be skipped
		engine.configDir = jsEngine.getString( "config_dir", fs::getDirOfFile( engine.executablePath ) );
		engine.dataDir = jsEngine.getString( "data_dir", engine.configDir );
		engine.family = familyFromStr( jsEngine.getString( "family", {} ) );
		if (engine.family == EngineFamily::_EnumEnd)
			engine.family = guessEngineFamily( fs::getFileBasenameFromPath( engine.executablePath ) );
	}
}

static QJsonObject serialize( const IWAD & iwad )
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

static void deserialize( const JsonObjectCtx & jsEngine, IWAD & iwad )
{
	iwad.isSeparator = jsEngine.getBool( "separator", false, DontShowError );
	if (iwad.isSeparator)
	{
		iwad.name = jsEngine.getString( "name", "<missing name>" );
	}
	else
	{
		iwad.path = jsEngine.getString( "path", {} );  // empty path is used to indicate invalid entry to be skipped
		iwad.name = jsEngine.getString( "name", QFileInfo( iwad.path ).fileName() );
	}
}

static QJsonObject serialize( const Mod & mod )
{
	QJsonObject jsMod;

	if (mod.isSeparator)
	{
		jsMod["separator"] = true;
		jsMod["name"] = mod.fileName;
	}
	else if (mod.isCmdArg)
	{
		jsMod["cmd_argument"] = true;
		jsMod["value"] = mod.fileName;
		jsMod["checked"] = mod.checked;
	}
	else
	{
		jsMod["path"] = mod.path;
		jsMod["checked"] = mod.checked;
	}

	return jsMod;
}

static void deserialize( const JsonObjectCtx & jsMod, Mod & mod )
{
	if ((mod.isSeparator = jsMod.getBool( "separator", false, DontShowError )))
	{
		mod.fileName = jsMod.getString( "name", "<missing name>" );
	}
	else if ((mod.isCmdArg = jsMod.getBool( "cmd_argument", false, DontShowError )))
	{
		mod.fileName = jsMod.getString( "value", "<missing value>" );
		mod.checked = jsMod.getBool( "checked", mod.checked );
	}
	else
	{
		mod.path = jsMod.getString( "path", {} );  // empty path is used to indicate invalid entry to be skipped
		mod.fileName = QFileInfo( mod.path ).fileName();
		mod.checked = jsMod.getBool( "checked", mod.checked );
	}
}

static QJsonObject serialize( const EngineSettings & engineSettings )
{
	QJsonObject jsEngines;

	jsEngines["default_engine"] = engineSettings.defaultEngine;

	return jsEngines;
}

static void deserialize( const JsonObjectCtx & jsEngines, EngineSettings & engineSettings )
{
	engineSettings.defaultEngine = jsEngines.getString( "default_engine", {}, DontShowError );
}

static QJsonObject serialize( const IwadSettings & iwadSettings )
{
	QJsonObject jsIWADs;

	jsIWADs["auto_update"] = iwadSettings.updateFromDir;
	jsIWADs["directory"] = iwadSettings.dir;
	jsIWADs["search_subdirs"] = iwadSettings.searchSubdirs;
	jsIWADs["default_iwad"] = iwadSettings.defaultIWAD;

	return jsIWADs;
}

static void deserialize( const JsonObjectCtx & jsIWADs, IwadSettings & iwadSettings )
{
	iwadSettings.updateFromDir = jsIWADs.getBool( "auto_update", iwadSettings.updateFromDir );
	iwadSettings.dir = jsIWADs.getString( "directory" );
	iwadSettings.searchSubdirs = jsIWADs.getBool( "search_subdirs", iwadSettings.searchSubdirs );
	iwadSettings.defaultIWAD = jsIWADs.getString( "default_iwad", {}, DontShowError );
}

static QJsonObject serialize( const MapSettings & mapSettings )
{
	QJsonObject jsMaps;

	jsMaps["directory"] = mapSettings.dir;

	return jsMaps;
}

static void deserialize( const JsonObjectCtx & jsMaps, MapSettings & mapSettings )
{
	mapSettings.dir = jsMaps.getString( "directory" );
}

static QJsonObject serialize( const ModSettings & modSettings )
{
	QJsonObject jsMods;

	jsMods["directory"] = modSettings.dir;
	jsMods["show_icons"] = modSettings.showIcons;

	return jsMods;
}

static void deserialize( const JsonObjectCtx & jsMods, ModSettings & modSettings )
{
	modSettings.dir = jsMods.getString( "directory" );
	modSettings.showIcons = jsMods.getBool( "show_icons", modSettings.showIcons );
}

static QJsonObject serialize( const LaunchOptions & opts )
{
	QJsonObject jsOptions;

	jsOptions["launch_mode"] = int( opts.mode );
	jsOptions["map_name"] = opts.mapName;
	jsOptions["save_file"] = opts.saveFile;
	jsOptions["map_name_demo"] = opts.mapName_demo;
	jsOptions["demo_file_record"] = opts.demoFile_record;
	jsOptions["demo_file_replay"] = opts.demoFile_replay;

	return jsOptions;
}

static void deserialize( const JsonObjectCtx & jsOptions, LaunchOptions & opts )
{
	opts.mode = jsOptions.getEnum< LaunchMode >( "launch_mode", opts.mode );
	opts.mapName = jsOptions.getString( "map_name" );
	opts.saveFile = jsOptions.getString( "save_file" );
	opts.mapName_demo = jsOptions.getString( "map_name_demo" );
	opts.demoFile_record = jsOptions.getString( "demo_file_record" );
	opts.demoFile_replay = jsOptions.getString( "demo_file_replay" );
}

static QJsonObject serialize( const MultiplayerOptions & opts )
{
	QJsonObject jsOptions;

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

static void deserialize( const JsonObjectCtx & jsOptions, MultiplayerOptions & opts )
{
	opts.isMultiplayer = jsOptions.getBool( "is_multiplayer", opts.isMultiplayer );
	opts.multRole = jsOptions.getEnum< MultRole >( "mult_role", opts.multRole );
	opts.hostName = jsOptions.getString( "host_name" );
	opts.port = jsOptions.getUInt16( "port", opts.port );
	opts.netMode = jsOptions.getEnum< NetMode >( "net_mode", opts.netMode );
	opts.gameMode = jsOptions.getEnum< GameMode >( "game_mode", opts.gameMode );
	opts.playerCount = jsOptions.getUInt( "player_count", opts.playerCount );
	opts.teamDamage = jsOptions.getDouble( "team_damage", opts.teamDamage );
	opts.timeLimit = jsOptions.getUInt( "time_limit", opts.timeLimit );
	opts.fragLimit = jsOptions.getUInt( "frag_limit", opts.fragLimit );
}

static QJsonObject serialize( const GameplayOptions & opts )
{
	QJsonObject jsOptions;

	jsOptions["skill_idx"] = int( opts.skillIdx );
	jsOptions["skill_num"] = int( opts.skillNum );
	jsOptions["no_monsters"] = opts.noMonsters;
	jsOptions["fast_monsters"] = opts.fastMonsters;
	jsOptions["monsters_respawn"] = opts.monstersRespawn;
	jsOptions["pistol_start"] = opts.pistolStart;
	jsOptions["allow_cheats"] = opts.allowCheats;
	jsOptions["dmflags1"] = qint64( opts.dmflags1 );
	jsOptions["dmflags2"] = qint64( opts.dmflags2 );
	jsOptions["dmflags3"] = qint64( opts.dmflags3 );

	return jsOptions;
}

static void deserialize( const JsonObjectCtx & jsOptions, GameplayOptions & opts )
{
	opts.skillIdx = jsOptions.getInt( "skill_idx", opts.skillIdx );
	opts.skillNum = jsOptions.getInt( "skill_num", opts.skillNum );
	opts.noMonsters = jsOptions.getBool( "no_monsters", opts.noMonsters );
	opts.fastMonsters = jsOptions.getBool( "fast_monsters", opts.fastMonsters );
	opts.monstersRespawn = jsOptions.getBool( "monsters_respawn", opts.monstersRespawn );
	opts.pistolStart = jsOptions.getBool( "pistol_start", opts.pistolStart );
	opts.allowCheats = jsOptions.getBool( "allow_cheats", opts.allowCheats );
	opts.dmflags1 = jsOptions.getInt( "dmflags1", opts.dmflags1 );
	opts.dmflags2 = jsOptions.getInt( "dmflags2", opts.dmflags2 );
	opts.dmflags3 = jsOptions.getInt( "dmflags3", opts.dmflags3 );
}

static QJsonObject serialize( const CompatibilityOptions & opts )
{
	QJsonObject jsOptions;

	jsOptions["compat_level"] = opts.compatLevel;
	jsOptions["compatflags1"] = qint64( opts.compatflags1 );
	jsOptions["compatflags2"] = qint64( opts.compatflags2 );

	return jsOptions;
}

static void deserialize( const JsonObjectCtx & jsOptions, CompatibilityOptions & opts )
{
	opts.compatflags1 = jsOptions.getInt( "compatflags1", opts.compatflags1 );
	opts.compatflags2 = jsOptions.getInt( "compatflags2", opts.compatflags2 );
	opts.compatLevel = jsOptions.getInt( "compat_level", opts.compatLevel );
}

static QJsonObject serialize( const AlternativePaths & opts )
{
	QJsonObject jsOptions;

	jsOptions["save_dir"] = opts.saveDir;
	jsOptions["screenshot_dir"] = opts.screenshotDir;

	return jsOptions;
}

static void deserialize( const JsonObjectCtx & jsOptions, AlternativePaths & opts )
{
	opts.saveDir = jsOptions.getString( "save_dir" );
	opts.screenshotDir = jsOptions.getString( "screenshot_dir" );
}

static QJsonObject serialize( const VideoOptions & opts )
{
	QJsonObject jsOptions;

	jsOptions["monitor_idx"] = opts.monitorIdx;
	jsOptions["resolution_x"] = qint64( opts.resolutionX );
	jsOptions["resolution_y"] = qint64( opts.resolutionY );
	jsOptions["show_fps"] = opts.showFPS;

	return jsOptions;
}

static void deserialize( const JsonObjectCtx & jsOptions, VideoOptions & opts )
{
	opts.monitorIdx = jsOptions.getInt( "monitor_idx", opts.monitorIdx );
	opts.resolutionX = jsOptions.getUInt( "resolution_x", opts.resolutionX );
	opts.resolutionY = jsOptions.getUInt( "resolution_y", opts.resolutionY );
	opts.showFPS = jsOptions.getBool( "show_fps", opts.showFPS );
}

static QJsonObject serialize( const AudioOptions & opts )
{
	QJsonObject jsOptions;

	jsOptions["no_sound"] = opts.noSound;
	jsOptions["no_sfx"] = opts.noSFX;
	jsOptions["no_music"] = opts.noMusic;

	return jsOptions;
}

static void deserialize( const JsonObjectCtx & jsOptions, AudioOptions & opts )
{
	opts.noSound = jsOptions.getBool( "no_sound", opts.noSound );
	opts.noSFX = jsOptions.getBool( "no_sfx", opts.noSFX );
	opts.noMusic = jsOptions.getBool( "no_music", opts.noMusic );
}

static QJsonObject serialize( const GlobalOptions & opts )
{
	QJsonObject jsOptions;

	jsOptions["use_preset_name_as_dir"] = opts.usePresetNameAsDir;
	jsOptions["additional_args"] = opts.cmdArgs;
	jsOptions["env_vars"] = serialize( opts.envVars );

	return jsOptions;
}

static void deserialize( const JsonObjectCtx & jsOptions, GlobalOptions & opts )
{
	opts.usePresetNameAsDir = jsOptions.getBool( "use_preset_name_as_dir", opts.usePresetNameAsDir );
	opts.cmdArgs = jsOptions.getString( "additional_args" );
	if (JsonObjectCtx jsEnvVars = jsOptions.getObject( "env_vars" ))
		deserialize( jsEnvVars, opts.envVars );
}

static QJsonObject serialize( const Preset & preset, const StorageSettings & settings )
{
	QJsonObject jsPreset;

	jsPreset["name"] = preset.name;

	if (preset.isSeparator)
	{
		jsPreset["separator"] = true;
		return jsPreset;
	}

	// files

	jsPreset["selected_engine"] = preset.selectedEnginePath;
	jsPreset["selected_config"] = preset.selectedConfig;
	jsPreset["selected_IWAD"] = preset.selectedIWAD;

	jsPreset["selected_mappacks"] = serializeStringVec( preset.selectedMapPacks );

	jsPreset["mods"] = serializeList( preset.mods );

	// options

	if (settings.launchOptsStorage == StoreToPreset)
		jsPreset["launch_options"] = serialize( preset.launchOpts );

	if (settings.launchOptsStorage == StoreToPreset)
		jsPreset["multiplayer_options"] = serialize( preset.multOpts );

	if (settings.gameOptsStorage == StoreToPreset)
		jsPreset["gameplay_options"] = serialize( preset.gameOpts );

	if (settings.compatOptsStorage == StoreToPreset)
		jsPreset["compatibility_options"] = serialize( preset.compatOpts );

	if (settings.videoOptsStorage == StoreToPreset)
		jsPreset["video_options"] = serialize( preset.videoOpts );

	if (settings.audioOptsStorage == StoreToPreset)
		jsPreset["audio_options"] = serialize( preset.audioOpts );

	jsPreset["alternative_paths"] = serialize( preset.altPaths );

	// preset-specific args

	jsPreset["additional_args"] = preset.cmdArgs;
	jsPreset["env_vars"] = serialize( preset.envVars );

	return jsPreset;
}

static void deserialize( const JsonObjectCtx & jsPreset, Preset & preset, const StorageSettings & settings )
{
	preset.name = jsPreset.getString( "name", "<missing name>" );

	preset.isSeparator = jsPreset.getBool( "separator", false, DontShowError );
	if (preset.isSeparator)
	{
		return;
	}

	// files

	preset.selectedEnginePath = jsPreset.getString( "selected_engine" );
	preset.selectedConfig = jsPreset.getString( "selected_config" );
	preset.selectedIWAD = jsPreset.getString( "selected_IWAD" );

	if (JsonArrayCtx jsSelectedMapPacks = jsPreset.getArray( "selected_mappacks" ))
	{
		preset.selectedMapPacks = deserializeStringVec( jsSelectedMapPacks );
	}

	if (JsonArrayCtx jsMods = jsPreset.getArray( "mods" ))
	{
		// iterate manually, so that we can filter-out invalid items
		for (int i = 0; i < jsMods.size(); i++)
		{
			JsonObjectCtx jsMod = jsMods.getObject( i );
			if (!jsMod)  // wrong type on position i - skip this entry
				continue;

			Mod mod;
			deserialize( jsMod, mod );

			if (mod.isSeparator || mod.isCmdArg) {
				if (mod.fileName.isEmpty())  // vital element is missing in the JSON -> skip this entry
					continue;
			} else {
				if (mod.path.isEmpty())  // vital element is missing in the JSON -> skip this entry
					continue;
			}

			// check path or not?

			preset.mods.append( std::move( mod ) );
		}
	}

	// options

	if (settings.launchOptsStorage == StoreToPreset)
		if (JsonObjectCtx jsOptions = jsPreset.getObject( "launch_options" ))
			deserialize( jsOptions, preset.launchOpts );

	if (settings.launchOptsStorage == StoreToPreset)
		if (JsonObjectCtx jsOptions = jsPreset.getObject( "multiplayer_options" ))
			deserialize( jsOptions, preset.multOpts );

	if (settings.gameOptsStorage == StoreToPreset)
		if (JsonObjectCtx jsOptions = jsPreset.getObject( "gameplay_options" ))
			deserialize( jsOptions, preset.gameOpts );

	if (settings.compatOptsStorage == StoreToPreset)
		if (JsonObjectCtx jsOptions = jsPreset.getObject( "compatibility_options" ))
			deserialize( jsOptions, preset.compatOpts );

	if (settings.videoOptsStorage == StoreToPreset)
		if (JsonObjectCtx jsOptions = jsPreset.getObject( "video_options" ))
			deserialize( jsOptions, preset.videoOpts );

	if (settings.audioOptsStorage == StoreToPreset)
		if (JsonObjectCtx jsOptions = jsPreset.getObject( "audio_options" ))
			deserialize( jsOptions, preset.audioOpts );

	if (JsonObjectCtx jsOptions = jsPreset.getObject( "alternative_paths" ))
		deserialize( jsOptions, preset.altPaths );

	// preset-specific args

	preset.cmdArgs = jsPreset.getString( "additional_args" );
	if (JsonObjectCtx jsEnvVars = jsPreset.getObject( "env_vars" ))
		deserialize( jsEnvVars, preset.envVars );
}

static QJsonObject serialize( const StorageSettings & settings )
{
	QJsonObject jsSettings;

	jsSettings["launch_opts"] = int( settings.launchOptsStorage );
	jsSettings["gameplay_opts"] = int( settings.gameOptsStorage );
	jsSettings["compat_opts"] = int( settings.compatOptsStorage );
	jsSettings["video_opts"] = int( settings.videoOptsStorage );
	jsSettings["audio_opts"] = int( settings.audioOptsStorage );

	return jsSettings;
}

static void deserialize( const JsonObjectCtx & jsSettings, StorageSettings & settings )
{
	settings.launchOptsStorage = jsSettings.getEnum< OptionsStorage >( "launch_opts", settings.launchOptsStorage );
	settings.gameOptsStorage = jsSettings.getEnum< OptionsStorage >( "gameplay_opts", settings.gameOptsStorage );
	settings.compatOptsStorage = jsSettings.getEnum< OptionsStorage >( "compat_opts", settings.compatOptsStorage );
	settings.videoOptsStorage = jsSettings.getEnum< OptionsStorage >( "video_opts", settings.videoOptsStorage );
	settings.audioOptsStorage = jsSettings.getEnum< OptionsStorage >( "audio_opts", settings.audioOptsStorage );
}

static void serialize( QJsonObject & jsSettings, const LauncherSettings & settings )
{
	jsSettings["use_absolute_paths"] = settings.pathStyle == PathStyle::Absolute;
	jsSettings["show_engine_output"] = settings.showEngineOutput;
	jsSettings["close_on_launch"] = settings.closeOnLaunch;
	jsSettings["check_for_updates"] = settings.checkForUpdates;
	jsSettings["ask_for_sandbox_permissions"] = settings.askForSandboxPermissions;

	jsSettings["options_storage"] = serialize( static_cast< const StorageSettings & >( settings ) );
}

static void deserialize( const JsonObjectCtx & jsSettings, LauncherSettings & settings )
{
	bool useAbsolutePaths = jsSettings.getBool( "use_absolute_paths", settings.pathStyle == PathStyle::Absolute );
	settings.pathStyle = useAbsolutePaths ? PathStyle::Absolute : PathStyle::Relative;

	settings.showEngineOutput = jsSettings.getBool( "show_engine_output", settings.showEngineOutput, DontShowError );
	settings.closeOnLaunch = jsSettings.getBool( "close_on_launch", settings.closeOnLaunch, DontShowError );
	settings.checkForUpdates = jsSettings.getBool( "check_for_updates", settings.checkForUpdates, DontShowError );
	settings.askForSandboxPermissions = jsSettings.getBool( "ask_for_sandbox_permissions", settings.askForSandboxPermissions, DontShowError );

	if (JsonObjectCtx jsOptsStorage = jsSettings.getObject( "options_storage" ))
	{
		deserialize( jsOptsStorage, static_cast< StorageSettings & >( settings ) );
	}
}

static QJsonObject serialize( const WindowGeometry & geometry )
{
	QJsonObject jsGeometry;

	jsGeometry["x"] = geometry.x;
	jsGeometry["y"] = geometry.y;
	jsGeometry["width"] = geometry.width;
	jsGeometry["height"] = geometry.height;

	return jsGeometry;
}

static void deserialize( const JsonObjectCtx & jsGeometry, WindowGeometry & geometry )
{
	geometry.x = jsGeometry.getInt( "x", geometry.x );
	geometry.y = jsGeometry.getInt( "y", geometry.y );
	geometry.width = jsGeometry.getInt( "width", geometry.width );
	geometry.height = jsGeometry.getInt( "height", geometry.height );
}

static void serialize( QJsonObject & jsAppearance, const AppearanceSettings & appearance )
{
	jsAppearance["geometry"] = serialize( appearance.geometry );
	jsAppearance["app_style"] = appearance.appStyle.isNull() ? QJsonValue( QJsonValue::Null ) : appearance.appStyle;
	jsAppearance["color_scheme"] = schemeToString( appearance.colorScheme );
}

static void deserialize( const JsonObjectCtx & jsAppearance, AppearanceSettings & appearance, bool loadGeometry )
{
	if (loadGeometry)
	{
		if (JsonObjectCtx jsGeometry = jsAppearance.getObject( "geometry" ))
		{
			deserialize( jsGeometry, appearance.geometry );
		}
	}

	appearance.appStyle = jsAppearance.getString( "app_style", {}, DontShowError );  // null value means system-default

	ColorScheme colorScheme = schemeFromString( jsAppearance.getString( "color_scheme" ) );
	if (colorScheme != ColorScheme::_EnumEnd)
		appearance.colorScheme = colorScheme;  // otherwise leave default
}


//======================================================================================================================
//  top-level JSON stucture

static void serialize( QJsonObject & jsRoot, const OptionsToSave & opts )
{
	// files and related settings

	{
		// better keep room for adding some engine settings later, so that we don't have to break compatibility again
		QJsonObject jsEngines = serialize( opts.engineSettings );

		jsEngines["engine_list"] = serializeList( opts.engines );  // serializes only Engine fields, leaves other EngineInfo fields alone

		jsRoot["engines"] = jsEngines;
	}

	{
		QJsonObject jsIWADs = serialize( opts.iwadSettings );

		if (!opts.iwadSettings.updateFromDir)
			jsIWADs["IWAD_list"] = serializeList( opts.iwads );

		jsRoot["IWADs"] = jsIWADs;
	}

	jsRoot["maps"] = serialize( opts.mapSettings );

	jsRoot["mods"] = serialize( opts.modSettings );

	// options

	if (opts.settings.launchOptsStorage == StoreGlobally)
		jsRoot["launch_options"] = serialize( opts.launchOpts );

	if (opts.settings.launchOptsStorage == StoreGlobally)
		jsRoot["multiplayer_options"] = serialize( opts.multOpts );

	if (opts.settings.gameOptsStorage == StoreGlobally)
		jsRoot["gameplay_options"] = serialize( opts.gameOpts );

	if (opts.settings.compatOptsStorage == StoreGlobally)
		jsRoot["compatibility_options"] = serialize( opts.compatOpts );

	if (opts.settings.videoOptsStorage == StoreGlobally)
		jsRoot["video_options"] = serialize( opts.videoOpts );

	if (opts.settings.audioOptsStorage == StoreGlobally)
		jsRoot["audio_options"] = serialize( opts.audioOpts );

	jsRoot["global_options"] = serialize( opts.globalOpts );

	// presets

	{
		QJsonArray jsPresetArray;

		for (const Preset & preset : opts.presets)
		{
			QJsonObject jsPreset = serialize( preset, opts.settings );
			jsPresetArray.append( jsPreset );
		}

		jsRoot["presets"] = jsPresetArray;
	}

	jsRoot["selected_preset"] = opts.selectedPresetIdx >= 0 ? opts.presets[ opts.selectedPresetIdx ].name : QString();

	// global settings - serialize directly to root, so that we don't have to break compatibility with older options

	serialize( jsRoot, opts.settings );

	serialize( jsRoot, opts.appearance );
}

static void deserialize( const JsonObjectCtx & jsRoot, OptionsToLoad & opts )
{
	// global settings - deserialize directly from root, so that we don't have to break compatibility with older options

	// This must be loaded early, because we need to know whether to attempt loading the opts from the presets or globally.
	deserialize( jsRoot, opts.settings );

	// files and related settings

	if (JsonObjectCtx jsEngines = jsRoot.getObject( "engines" ))
	{
		deserialize( jsEngines, opts.engineSettings );

		if (JsonArrayCtx jsEngineArray = jsEngines.getArray( "engine_list" ))
		{
			// iterate manually, so that we can filter-out invalid items
			for (int i = 0; i < jsEngineArray.size(); i++)
			{
				JsonObjectCtx jsEngine = jsEngineArray.getObject( i );
				if (!jsEngine)  // wrong type on position i -> skip this entry
					continue;

				Engine engine;
				deserialize( jsEngine, engine );

				if (engine.executablePath.isEmpty())  // element isn't present in JSON -> skip this entry
					continue;

				if (!PathChecker::checkFilePath( engine.executablePath, true, "an Engine from the saved options", "Please update it in Menu -> Initial Setup." ))
					highlightInvalidListItem( engine );

				opts.engines.append( std::move( engine ) );  // populates only Engine fields, leaves other EngineInfo fields empty
			}
		}
	}

	if (JsonObjectCtx jsIWADs = jsRoot.getObject( "IWADs" ))
	{
		deserialize( jsIWADs, opts.iwadSettings );

		if (opts.iwadSettings.updateFromDir)
		{
			PathChecker::checkNonEmptyDirPath( opts.iwadSettings.dir, true, "IWAD directory from the saved options", "Please update it in Menu -> Initial Setup." );
		}
		else
		{
			if (JsonArrayCtx jsIWADArray = jsIWADs.getArray( "IWAD_list" ))
			{
				// iterate manually, so that we can filter-out invalid items
				for (int i = 0; i < jsIWADArray.size(); i++)
				{
					JsonObjectCtx jsIWAD = jsIWADArray.getObject( i );
					if (!jsIWAD)  // wrong type on position i - skip this entry
						continue;

					IWAD iwad;
					deserialize( jsIWAD, iwad );

					if (iwad.name.isEmpty() || iwad.path.isEmpty())  // element isn't present in JSON -> skip this entry
						continue;

					if (!PathChecker::checkFilePath( iwad.path, true, "an IWAD from the saved options", "Please update it in Menu -> Initial Setup." ))
						highlightInvalidListItem( iwad );

					opts.iwads.append( std::move( iwad ) );
				}
			}
		}
	}

	if (JsonObjectCtx jsMaps = jsRoot.getObject( "maps" ))
	{
		deserialize( jsMaps, opts.mapSettings );

		PathChecker::checkNonEmptyDirPath( opts.mapSettings.dir, true, "map directory from the saved options", "Please update it in Menu -> Initial Setup." );
	}

	if (JsonObjectCtx jsMods = jsRoot.getObject( "mods" ))
	{
		deserialize( jsMods, opts.modSettings );

		PathChecker::checkNonEmptyDirPath( opts.modSettings.dir, true, "mod directory from the saved options", "Please update it in Menu -> Initial Setup." );
	}

	// options

	if (opts.settings.launchOptsStorage == StoreGlobally)
		if (JsonObjectCtx jsOptions = jsRoot.getObject( "launch_options" ))
			deserialize( jsOptions, opts.launchOpts );

	if (opts.settings.launchOptsStorage == StoreGlobally)
		if (JsonObjectCtx jsOptions = jsRoot.getObject( "multiplayer_options" ))
			deserialize( jsOptions, opts.multOpts );

	if (opts.settings.gameOptsStorage == StoreGlobally)
		if (JsonObjectCtx jsOptions = jsRoot.getObject( "gameplay_options" ))
			deserialize( jsOptions, opts.gameOpts );

	if (opts.settings.compatOptsStorage == StoreGlobally)
		if (JsonObjectCtx jsOptions = jsRoot.getObject( "compatibility_options" ))
			deserialize( jsOptions, opts.compatOpts );

	if (opts.settings.videoOptsStorage == StoreGlobally)
		if (JsonObjectCtx jsOptions = jsRoot.getObject( "video_options" ))
			deserialize( jsOptions, opts.videoOpts );

	if (opts.settings.audioOptsStorage == StoreGlobally)
		if (JsonObjectCtx jsOptions = jsRoot.getObject( "audio_options" ))
			deserialize( jsOptions, opts.audioOpts );

	if (JsonObjectCtx jsOptions = jsRoot.getObject( "global_options" ))
		deserialize( jsOptions, opts.globalOpts );

	// presets

	if (JsonArrayCtx jsPresetArray = jsRoot.getArray( "presets" ))
	{
		for (int i = 0; i < jsPresetArray.size(); i++)
		{
			JsonObjectCtx jsPreset = jsPresetArray.getObject( i );
			if (!jsPreset)  // wrong type on position i - skip this entry
				continue;

			Preset preset;
			deserialize( jsPreset, preset, opts.settings );

			opts.presets.append( std::move( preset ) );
		}
	}

	opts.selectedPreset = jsRoot.getString( "selected_preset" );
}


//======================================================================================================================
//  backward compatibility - loading user data from older options format

#include "OptionsSerializer_compat.cpp"  // hack, but it's ok in this case


//======================================================================================================================
//  top-level API

QJsonDocument serializeOptionsToJsonDoc( const OptionsToSave & opts )
{
	QJsonObject jsRoot;

	// this will be used to detect options created by older versions and supress "missing element" warnings
	jsRoot["version"] = appVersion;

	serialize( jsRoot, opts );

	return QJsonDocument( jsRoot );
}

void deserializeAppearanceFromJsonDoc( const JsonDocumentCtx & jsonDoc, AppearanceToLoad & opts, bool loadGeometry )
{
	const JsonObjectCtx & jsRoot = jsonDoc.rootObject();

	// deserialize directly from root, so that we don't have to break compatibility with older options
	deserialize( jsRoot, opts.appearance, loadGeometry );
}

void deserializeOptionsFromJsonDoc( const JsonDocumentCtx & jsonDoc, OptionsToLoad & opts )
{
	// We use this contextual mechanism instead of standard JSON getters, because when something fails to load
	// we want to print a useful error message with information exactly which JSON element is broken.
	const JsonObjectCtx & jsRoot = jsonDoc.rootObject();

	QString optsVersionStr = jsRoot.getString( "version", {}, DontShowError );
	Version optsVersion( optsVersionStr );

	if (!optsVersionStr.isEmpty() && optsVersion > appVersion)  // empty version means pre-1.4 version
	{
		reportRuntimeError( nullptr, "Loading options from newer version",
			"Detected saved options from newer version of DoomRunner. "
			"Some settings might not be compatible. Expect errors."
		);
	}

	// backward compatibility with older options format
	if (optsVersionStr.isEmpty() || optsVersion < Version(1,7))
	{
		jsonDoc.disableWarnings();  // supress "missing element" warnings when loading older version

		// try to load as the 1.6.3 format, older versions will have to accept resetting some values to defaults
		deserialize_pre17( jsRoot, opts );
	}
	else
	{
		if (optsVersion < appVersion)
			jsonDoc.disableWarnings();  // supress "missing element" warnings when loading older version

		deserialize( jsRoot, opts );
	}
}
