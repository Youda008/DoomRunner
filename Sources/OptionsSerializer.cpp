//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: serialization/deserialization of the user data into/from a file
//======================================================================================================================

#include "OptionsSerializer.hpp"

#include "AppVersion.hpp"
#include "Utils/JsonUtils.hpp"
#include "Utils/FileSystemUtils.hpp"
#include "Utils/PathCheckUtils.hpp"  // checkPath, highlightInvalidListItem
#include "Utils/ErrorHandling.hpp"

#include <QFileInfo>


//----------------------------------------------------------------------------------------------------------------------

const QString InvalidItemName = "<invalid name>";
const QString InvalidItemPath = "<invalid path>";


//======================================================================================================================
// custom data types

static QJsonObject serialize( const EnvVars & envVars )
{
	QJsonObject envVarsJs;

	// convert list to a map
	for (const auto & envVar : envVars)
	{
		envVarsJs[ envVar.name ] = envVar.value;
	}

	return envVarsJs;
}

static void deserialize( const JsonObjectCtx & envVarsJs, EnvVars & envVars )
{
	// convert map to a list
	auto keys = envVarsJs.keys();
	for (auto & varName : keys)
	{
		QString value = envVarsJs.getString( varName, {} );
		envVars.append({ std::move(varName), std::move(value) });
	}

	// keep the list sorted
	std::sort( envVars.begin(), envVars.end(), []( const auto & a, const auto & b ) { return a.name < b.name; });
}

static QJsonValue toJson( const QColor & color )
{
	if (color.isValid())
		return color.name( QColor::HexRgb );
	else
		return QJsonValue::Null;
}

static QColor getColor( const JsonObjectCtx & parentObj, const QString & key )
{
	QJsonValue colorJs = parentObj.getMember( key );
	QColor color;  // not valid until successfully parsed
	if (colorJs.isString())
	{
		QString colorStr = colorJs.toString();
		color = QColor( colorStr );
	}
	else if (!colorJs.isNull())  // null is allowed - it means no color has been selected
	{
		parentObj.invalidTypeAtKey( key, "QColor" );
	}
	return color;
}


//======================================================================================================================
// user data sub-sections

QJsonObject serialize( const Engine & engine )
{
	QJsonObject engineJs;

	if (engine.isSeparator)
	{
		engineJs["separator"] = true;
		engineJs["name"] = engine.name;
	}
	else
	{
		engineJs["name"] = engine.name;
		engineJs["path"] = engine.executablePath;
		engineJs["config_dir"] = engine.configDir;
		engineJs["data_dir"] = engine.dataDir;
		engineJs["family"] = familyToStr( engine.family );
	}

	return engineJs;
}

void deserialize( const JsonObjectCtx & engineJs, Engine & engine )
{
	engine.isSeparator = engineJs.getBool( "separator", false, DontShowError );
	if (engine.isSeparator)
	{
		engine.name = engineJs.getString( "name", InvalidItemName );
	}
	else
	{
		engine.name = engineJs.getString( "name", InvalidItemName );
		engine.executablePath = engineJs.getString( "path", InvalidItemPath );
		if (engine.executablePath != InvalidItemPath)
		{
			engine.configDir = engineJs.getString( "config_dir", fs::getParentDir( engine.executablePath ) );
			engine.dataDir = engineJs.getString( "data_dir", engine.configDir );
		}
		engine.family = familyFromStr( engineJs.getString( "family", {} ) );
	}
}

QJsonObject serialize( const IWAD & iwad )
{
	QJsonObject iwadJs;

	if (iwad.isSeparator)
	{
		iwadJs["separator"] = true;
		iwadJs["name"] = iwad.name;
	}
	else
	{
		iwadJs["name"] = iwad.name;
		iwadJs["path"] = iwad.path;
	}

	return iwadJs;
}

void deserialize( const JsonObjectCtx & engineJs, IWAD & iwad )
{
	iwad.isSeparator = engineJs.getBool( "separator", false, DontShowError );
	if (iwad.isSeparator)
	{
		iwad.name = engineJs.getString( "name", InvalidItemName );
	}
	else
	{
		iwad.path = engineJs.getString( "path", InvalidItemPath );
		iwad.name = engineJs.getString( "name", iwad.path != InvalidItemPath ? QFileInfo( iwad.path ).fileName() : InvalidItemName );
	}
}

QJsonObject serialize( const Mod & mod )
{
	QJsonObject modJs;

	if (mod.isSeparator)
	{
		modJs["separator"] = true;
		modJs["name"] = mod.name;
	}
	else if (mod.isCmdArg)
	{
		modJs["cmd_argument"] = true;
		modJs["value"] = mod.name;
		modJs["checked"] = mod.checked;
	}
	else
	{
		modJs["path"] = mod.path;
		modJs["checked"] = mod.checked;
	}

	return modJs;
}

void deserialize( const JsonObjectCtx & modJs, Mod & mod )
{
	if ((mod.isSeparator = modJs.getBool( "separator", false, DontShowError )))
	{
		mod.name = modJs.getString( "name", InvalidItemName );
	}
	else if ((mod.isCmdArg = modJs.getBool( "cmd_argument", false, DontShowError )))
	{
		mod.name = modJs.getString( "value", InvalidItemName );
		mod.checked = modJs.getBool( "checked", mod.checked );
	}
	else
	{
		mod.path = modJs.getString( "path", InvalidItemPath );
		mod.name = mod.path != InvalidItemPath ? QFileInfo( mod.path ).fileName() : InvalidItemName;
		mod.checked = modJs.getBool( "checked", mod.checked );
	}
}

static QJsonObject serialize( const EngineSettings & engineSettings )
{
	QJsonObject engineSettingsJs;

	engineSettingsJs["default_engine"] = engineSettings.defaultEngine;

	return engineSettingsJs;
}

static void deserialize( const JsonObjectCtx & enginesJs, EngineSettings & engineSettings )
{
	engineSettings.defaultEngine = enginesJs.getString( "default_engine", {}, DontShowError );
}

static QJsonObject serialize( const IwadSettings & iwadSettings )
{
	QJsonObject iwadSettingsJs;

	iwadSettingsJs["auto_update"] = iwadSettings.updateFromDir;
	iwadSettingsJs["directory"] = iwadSettings.dir;
	iwadSettingsJs["search_subdirs"] = iwadSettings.searchSubdirs;
	iwadSettingsJs["default_iwad"] = iwadSettings.defaultIWAD;

	return iwadSettingsJs;
}

static void deserialize( const JsonObjectCtx & iwadSettingsJs, IwadSettings & iwadSettings )
{
	iwadSettings.updateFromDir = iwadSettingsJs.getBool( "auto_update", iwadSettings.updateFromDir );
	iwadSettings.dir = iwadSettingsJs.getString( "directory" );
	iwadSettings.searchSubdirs = iwadSettingsJs.getBool( "search_subdirs", iwadSettings.searchSubdirs );
	iwadSettings.defaultIWAD = iwadSettingsJs.getString( "default_iwad", {}, DontShowError );
}

static QJsonObject serialize( const MapSettings & mapSettings )
{
	QJsonObject mapsJs;

	mapsJs["directory"] = mapSettings.dir;
	mapsJs["show_icons"] = mapSettings.showIcons;

	return mapsJs;
}

static void deserialize( const JsonObjectCtx & mapSettingsJs, MapSettings & mapSettings )
{
	mapSettings.dir = mapSettingsJs.getString( "directory" );
	mapSettings.showIcons = mapSettingsJs.getBool( "show_icons", mapSettings.showIcons );
}

static QJsonObject serialize( const ModSettings & modSettings )
{
	QJsonObject modsJs;

	modsJs["last_used_dir"] = modSettings.lastUsedDir;
	modsJs["show_icons"] = modSettings.showIcons;

	return modsJs;
}

static void deserialize( const JsonObjectCtx & modSettingsJs, ModSettings & modSettings )
{
	modSettings.lastUsedDir = modSettingsJs.getString( "last_used_dir" );
	modSettings.showIcons = modSettingsJs.getBool( "show_icons", modSettings.showIcons );
}

static QJsonObject serialize( const LaunchOptions & opts )
{
	QJsonObject optsJs;

	optsJs["launch_mode"] = int( opts.mode );
	optsJs["map_name"] = opts.mapName;
	optsJs["save_file"] = opts.saveFile;
	optsJs["map_name_demo"] = opts.mapName_demo;
	optsJs["demo_file_record"] = opts.demoFile_record;
	optsJs["demo_file_replay"] = opts.demoFile_replay;
	optsJs["demo_file_resume_from"] = opts.demoFile_resumeFrom;
	optsJs["demo_file_resume_to"] = opts.demoFile_resumeTo;

	return optsJs;
}

static void deserialize( const JsonObjectCtx & optsJs, LaunchOptions & opts )
{
	opts.mode = optsJs.getEnum< LaunchMode >( "launch_mode", opts.mode );
	opts.mapName = optsJs.getString( "map_name" );
	opts.saveFile = optsJs.getString( "save_file" );
	opts.mapName_demo = optsJs.getString( "map_name_demo" );
	opts.demoFile_record = optsJs.getString( "demo_file_record" );
	opts.demoFile_replay = optsJs.getString( "demo_file_replay" );
	opts.demoFile_resumeFrom = optsJs.getString( "demo_file_resume_from" );
	opts.demoFile_resumeTo = optsJs.getString( "demo_file_resume_to" );
}

static QJsonObject serialize( const MultiplayerOptions & opts )
{
	QJsonObject optsJs;

	optsJs["is_multiplayer"] = opts.isMultiplayer;
	optsJs["mult_role"] = int( opts.multRole );
	optsJs["host_name"] = opts.hostName;
	optsJs["port"] = int( opts.port );
	optsJs["net_mode"] = int( opts.netMode );
	optsJs["game_mode"] = int( opts.gameMode );
	optsJs["player_count"] = int( opts.playerCount );
	optsJs["team_damage"] = opts.teamDamage;
	optsJs["time_limit"] = int( opts.timeLimit );
	optsJs["frag_limit"] = int( opts.fragLimit );
	optsJs["player_name"] = opts.playerName;
	optsJs["player_color"] = toJson( opts.playerColor );

	return optsJs;
}

static void deserialize( const JsonObjectCtx & optsJs, MultiplayerOptions & opts )
{
	opts.isMultiplayer = optsJs.getBool( "is_multiplayer", opts.isMultiplayer );
	opts.multRole = optsJs.getEnum< MultRole >( "mult_role", opts.multRole );
	opts.hostName = optsJs.getString( "host_name" );
	opts.port = optsJs.getUInt16( "port", opts.port );
	opts.netMode = optsJs.getEnum< NetMode >( "net_mode", opts.netMode );
	opts.gameMode = optsJs.getEnum< GameMode >( "game_mode", opts.gameMode );
	opts.playerCount = optsJs.getUInt( "player_count", opts.playerCount );
	opts.teamDamage = optsJs.getDouble( "team_damage", opts.teamDamage );
	opts.timeLimit = optsJs.getUInt( "time_limit", opts.timeLimit );
	opts.fragLimit = optsJs.getUInt( "frag_limit", opts.fragLimit );
	opts.playerName = optsJs.getString( "player_name" );
	opts.playerColor = getColor( optsJs, "player_color" );
}

static QJsonObject serialize( const GameplayOptions & opts )
{
	QJsonObject optsJs;

	optsJs["skill_idx"] = int( opts.skillIdx );
	optsJs["skill_num"] = int( opts.skillNum );
	optsJs["no_monsters"] = opts.noMonsters;
	optsJs["fast_monsters"] = opts.fastMonsters;
	optsJs["monsters_respawn"] = opts.monstersRespawn;
	optsJs["pistol_start"] = opts.pistolStart;
	optsJs["allow_cheats"] = opts.allowCheats;
	optsJs["dmflags1"] = qint64( opts.dmflags1 );
	optsJs["dmflags2"] = qint64( opts.dmflags2 );
	optsJs["dmflags3"] = qint64( opts.dmflags3 );

	return optsJs;
}

static void deserialize( const JsonObjectCtx & optsJs, GameplayOptions & opts )
{
	opts.skillIdx = optsJs.getInt( "skill_idx", opts.skillIdx );
	opts.skillNum = optsJs.getInt( "skill_num", opts.skillNum );
	opts.noMonsters = optsJs.getBool( "no_monsters", opts.noMonsters );
	opts.fastMonsters = optsJs.getBool( "fast_monsters", opts.fastMonsters );
	opts.monstersRespawn = optsJs.getBool( "monsters_respawn", opts.monstersRespawn );
	opts.pistolStart = optsJs.getBool( "pistol_start", opts.pistolStart );
	opts.allowCheats = optsJs.getBool( "allow_cheats", opts.allowCheats );
	opts.dmflags1 = optsJs.getInt( "dmflags1", opts.dmflags1 );
	opts.dmflags2 = optsJs.getInt( "dmflags2", opts.dmflags2 );
	opts.dmflags3 = optsJs.getInt( "dmflags3", opts.dmflags3 );
}

static QJsonObject serialize( const CompatibilityOptions & opts )
{
	QJsonObject optsJs;

	optsJs["compat_mode"] = opts.compatMode;
	optsJs["compatflags1"] = qint64( opts.compatflags1 );
	optsJs["compatflags2"] = qint64( opts.compatflags2 );

	return optsJs;
}

static void deserialize( const JsonObjectCtx & optsJs, CompatibilityOptions & opts )
{
	opts.compatflags1 = optsJs.getInt( "compatflags1", opts.compatflags1 );
	opts.compatflags2 = optsJs.getInt( "compatflags2", opts.compatflags2 );
	if (optsJs.hasMember("compat_level"))
		opts.compatMode = optsJs.getInt( "compat_level", opts.compatMode );
	else
		opts.compatMode = optsJs.getInt( "compat_mode", opts.compatMode );
}

static QJsonObject serialize( const AlternativePaths & opts )
{
	QJsonObject optsJs;

	optsJs["config_dir"] = opts.configDir;
	optsJs["save_dir"] = opts.saveDir;
	optsJs["demo_dir"] = opts.demoDir;
	optsJs["screenshot_dir"] = opts.screenshotDir;

	return optsJs;
}

static void deserialize( const JsonObjectCtx & optsJs, AlternativePaths & opts )
{
	opts.configDir = optsJs.getString( "config_dir" );
	opts.saveDir = optsJs.getString( "save_dir" );
	opts.demoDir = optsJs.getString( "demo_dir" );
	opts.screenshotDir = optsJs.getString( "screenshot_dir" );
}

static QJsonObject serialize( const VideoOptions & opts )
{
	QJsonObject optsJs;

	optsJs["monitor_idx"] = opts.monitorIdx;
	optsJs["resolution_x"] = qint64( opts.resolutionX );
	optsJs["resolution_y"] = qint64( opts.resolutionY );
	optsJs["show_fps"] = opts.showFPS;

	return optsJs;
}

static void deserialize( const JsonObjectCtx & optsJs, VideoOptions & opts )
{
	opts.monitorIdx = optsJs.getInt( "monitor_idx", opts.monitorIdx );
	opts.resolutionX = optsJs.getUInt( "resolution_x", opts.resolutionX );
	opts.resolutionY = optsJs.getUInt( "resolution_y", opts.resolutionY );
	opts.showFPS = optsJs.getBool( "show_fps", opts.showFPS );
}

static QJsonObject serialize( const AudioOptions & opts )
{
	QJsonObject optsJs;

	optsJs["no_sound"] = opts.noSound;
	optsJs["no_sfx"] = opts.noSFX;
	optsJs["no_music"] = opts.noMusic;

	return optsJs;
}

static void deserialize( const JsonObjectCtx & optsJs, AudioOptions & opts )
{
	opts.noSound = optsJs.getBool( "no_sound", opts.noSound );
	opts.noSFX = optsJs.getBool( "no_sfx", opts.noSFX );
	opts.noMusic = optsJs.getBool( "no_music", opts.noMusic );
}

static QJsonObject serialize( const GlobalOptions & opts )
{
	QJsonObject optsJs;

	optsJs["use_preset_name_as_config_dir"] = opts.usePresetNameAsConfigDir;
	optsJs["use_preset_name_as_save_dir"] = opts.usePresetNameAsSaveDir;
	optsJs["use_preset_name_as_demo_dir"] = opts.usePresetNameAsDemoDir;
	optsJs["use_preset_name_as_screenshot_dir"] = opts.usePresetNameAsScreenshotDir;
	optsJs["additional_args"] = opts.cmdArgs;
	optsJs["cmd_prefix"] = opts.cmdPrefix;
	optsJs["env_vars"] = serialize( opts.envVars );

	return optsJs;
}

static void deserialize( const JsonObjectCtx & optsJs, GlobalOptions & opts )
{
	if (optsJs.hasMember("use_preset_name_as_dir"))  // old options (older than 1.9.0)
	{
		bool usePresetNameAsDir = optsJs.getBool( "use_preset_name_as_dir", false );
		// Before 1.9.0 this option controlled saves and screenshots together -> apply it for those.
		opts.usePresetNameAsSaveDir = usePresetNameAsDir;
		opts.usePresetNameAsDemoDir = usePresetNameAsDir;
		opts.usePresetNameAsScreenshotDir = usePresetNameAsDir;
		// However this option did not exist and could cause confusion -> leave at false.
		opts.usePresetNameAsConfigDir = false;
	}
	else  // new options (1.9.0 or newer)
	{
		opts.usePresetNameAsConfigDir = optsJs.getBool( "use_preset_name_as_config_dir", opts.usePresetNameAsConfigDir );
		opts.usePresetNameAsSaveDir = optsJs.getBool( "use_preset_name_as_save_dir", opts.usePresetNameAsSaveDir );
		opts.usePresetNameAsDemoDir = optsJs.getBool( "use_preset_name_as_demo_dir", opts.usePresetNameAsDemoDir );
		opts.usePresetNameAsScreenshotDir = optsJs.getBool( "use_preset_name_as_screenshot_dir", opts.usePresetNameAsScreenshotDir );
	}

	opts.cmdArgs = optsJs.getString( "additional_args" );
	opts.cmdPrefix = optsJs.getString( "cmd_prefix" );
	if (JsonObjectCtx jsEnvVars = optsJs.getObject( "env_vars" ))
		deserialize( jsEnvVars, opts.envVars );
}

static QJsonObject serialize( const Preset & preset, const StorageSettings & settings )
{
	QJsonObject presetJs;

	presetJs["name"] = preset.name;

	if (preset.isSeparator)
	{
		presetJs["separator"] = true;
		return presetJs;
	}

	// files

	presetJs["selected_engine"] = preset.selectedEnginePath;
	presetJs["selected_config"] = preset.selectedConfig;
	presetJs["selected_IWAD"] = preset.selectedIWAD;

	presetJs["selected_mappacks"] = serializeStringList( preset.selectedMapPacks );

	presetJs["mods"] = serializeList( preset.mods );

	presetJs["load_maps_after_mods"] = preset.loadMapsAfterMods;

	// options

	if (settings.launchOptsStorage == StoreToPreset)
		presetJs["launch_options"] = serialize( preset.launchOpts );

	if (settings.launchOptsStorage == StoreToPreset)
		presetJs["multiplayer_options"] = serialize( preset.multOpts );

	if (settings.gameOptsStorage == StoreToPreset)
		presetJs["gameplay_options"] = serialize( preset.gameOpts );

	if (settings.compatOptsStorage == StoreToPreset)
		presetJs["compatibility_options"] = serialize( preset.compatOpts );

	if (settings.videoOptsStorage == StoreToPreset)
		presetJs["video_options"] = serialize( preset.videoOpts );

	if (settings.audioOptsStorage == StoreToPreset)
		presetJs["audio_options"] = serialize( preset.audioOpts );

	presetJs["alternative_paths"] = serialize( preset.altPaths );

	// preset-specific args

	presetJs["additional_args"] = preset.cmdArgs;
	presetJs["env_vars"] = serialize( preset.envVars );

	return presetJs;
}

static void deserialize( const JsonObjectCtx & presetJs, Preset & preset, const StorageSettings & settings )
{
	preset.name = presetJs.getString( "name", InvalidItemName );

	preset.isSeparator = presetJs.getBool( "separator", false, DontShowError );
	if (preset.isSeparator)
	{
		return;
	}

	// files

	preset.selectedEnginePath = presetJs.getString( "selected_engine" );
	preset.selectedConfig = presetJs.getString( "selected_config" );
	preset.selectedIWAD = presetJs.getString( "selected_IWAD" );

	if (JsonArrayCtx selectedMapPacksJs = presetJs.getArray( "selected_mappacks" ))
	{
		preset.selectedMapPacks = deserializeStringList( selectedMapPacksJs );
	}

	if (JsonArrayCtx modArrayJs = presetJs.getArray( "mods" ))
	{
		// iterate manually, so that we can filter-out invalid items
		preset.mods.reserve( modArrayJs.size() );
		for (qsize_t i = 0; i < modArrayJs.size(); i++)
		{
			JsonObjectCtx modJs = modArrayJs.getObject( i );
			if (!modJs)  // wrong type on position i - skip this entry
				continue;

			Mod mod( /*checked*/false );
			deserialize( modJs, mod );

			bool isValid = mod.name != InvalidItemName && mod.path != InvalidItemPath;
				//&& (mod.path.isEmpty() || PathChecker::checkFilePath( mod.path, true, "a Mod from the saved options", "Please update it in the corresponding preset." ));
			if (!isValid)
				highlightListItemAsInvalid( mod );

			preset.mods.append( std::move( mod ) );
		}
	}

	preset.loadMapsAfterMods = presetJs.getBool( "load_maps_after_mods", preset.loadMapsAfterMods );

	// options

	if (settings.launchOptsStorage == StoreToPreset)
		if (JsonObjectCtx optsJs = presetJs.getObject( "launch_options" ))
			deserialize( optsJs, preset.launchOpts );

	if (settings.launchOptsStorage == StoreToPreset)
		if (JsonObjectCtx optsJs = presetJs.getObject( "multiplayer_options" ))
			deserialize( optsJs, preset.multOpts );

	if (settings.gameOptsStorage == StoreToPreset)
		if (JsonObjectCtx optsJs = presetJs.getObject( "gameplay_options" ))
			deserialize( optsJs, preset.gameOpts );

	if (settings.compatOptsStorage == StoreToPreset)
		if (JsonObjectCtx optsJs = presetJs.getObject( "compatibility_options" ))
			deserialize( optsJs, preset.compatOpts );

	if (settings.videoOptsStorage == StoreToPreset)
		if (JsonObjectCtx optsJs = presetJs.getObject( "video_options" ))
			deserialize( optsJs, preset.videoOpts );

	if (settings.audioOptsStorage == StoreToPreset)
		if (JsonObjectCtx optsJs = presetJs.getObject( "audio_options" ))
			deserialize( optsJs, preset.audioOpts );

	if (JsonObjectCtx optsJs = presetJs.getObject( "alternative_paths" ))
		deserialize( optsJs, preset.altPaths );

	// preset-specific args

	preset.cmdArgs = presetJs.getString( "additional_args" );
	if (JsonObjectCtx envVarsJs = presetJs.getObject( "env_vars" ))
		deserialize( envVarsJs, preset.envVars );
}

static QJsonObject serialize( const StorageSettings & settings )
{
	QJsonObject settingsJs;

	settingsJs["launch_opts"] = int( settings.launchOptsStorage );
	settingsJs["gameplay_opts"] = int( settings.gameOptsStorage );
	settingsJs["compat_opts"] = int( settings.compatOptsStorage );
	settingsJs["video_opts"] = int( settings.videoOptsStorage );
	settingsJs["audio_opts"] = int( settings.audioOptsStorage );

	return settingsJs;
}

static void deserialize( const JsonObjectCtx & settingsJs, StorageSettings & settings )
{
	settings.launchOptsStorage = settingsJs.getEnum< OptionsStorage >( "launch_opts", settings.launchOptsStorage );
	settings.gameOptsStorage = settingsJs.getEnum< OptionsStorage >( "gameplay_opts", settings.gameOptsStorage );
	settings.compatOptsStorage = settingsJs.getEnum< OptionsStorage >( "compat_opts", settings.compatOptsStorage );
	settings.videoOptsStorage = settingsJs.getEnum< OptionsStorage >( "video_opts", settings.videoOptsStorage );
	settings.audioOptsStorage = settingsJs.getEnum< OptionsStorage >( "audio_opts", settings.audioOptsStorage );
}

static void serialize( QJsonObject & settingsJs, const LauncherSettings & settings )
{
	settingsJs["use_absolute_paths"] = settings.pathStyle.isAbsolute();
	settingsJs["show_engine_output"] = settings.showEngineOutput;
	settingsJs["close_on_launch"] = settings.closeOnLaunch;
	settingsJs["close_output_on_success"] = settings.closeOutputOnSuccess;
	settingsJs["check_for_updates"] = settings.checkForUpdates;
	settingsJs["ask_for_sandbox_permissions"] = settings.askForSandboxPermissions;
	settingsJs["hide_map_label"] = settings.hideMapHelpLabel;

	settingsJs["options_storage"] = serialize( static_cast< const StorageSettings & >( settings ) );
}

static void deserialize( const JsonObjectCtx & settingsJs, LauncherSettings & settings )
{
	settings.pathStyle.toggleAbsolute( settingsJs.getBool( "use_absolute_paths", settings.pathStyle.isAbsolute() ) );

	settings.showEngineOutput = settingsJs.getBool( "show_engine_output", settings.showEngineOutput, DontShowError );
	settings.closeOnLaunch = settingsJs.getBool( "close_on_launch", settings.closeOnLaunch, DontShowError );
	settings.closeOutputOnSuccess = settingsJs.getBool( "close_output_on_success", settings.closeOutputOnSuccess, DontShowError );
	settings.checkForUpdates = settingsJs.getBool( "check_for_updates", settings.checkForUpdates, DontShowError );
	settings.askForSandboxPermissions = settingsJs.getBool( "ask_for_sandbox_permissions", settings.askForSandboxPermissions, DontShowError );
	settings.hideMapHelpLabel = settingsJs.getBool( "hide_map_label", settings.hideMapHelpLabel, DontShowError );

	if (JsonObjectCtx optsStorageJs = settingsJs.getObject( "options_storage" ))
	{
		deserialize( optsStorageJs, static_cast< StorageSettings & >( settings ) );
	}
}

static QJsonObject serialize( const WindowGeometry & geometry )
{
	QJsonObject geometryJs;

	geometryJs["x"] = geometry.x;
	geometryJs["y"] = geometry.y;
	geometryJs["width"] = geometry.width;
	geometryJs["height"] = geometry.height;

	return geometryJs;
}

static void deserialize( const JsonObjectCtx & geometryJs, WindowGeometry & geometry )
{
	geometry.x = geometryJs.getInt( "x", geometry.x );
	geometry.y = geometryJs.getInt( "y", geometry.y );
	geometry.width = geometryJs.getInt( "width", geometry.width );
	geometry.height = geometryJs.getInt( "height", geometry.height );
}

static void serialize( QJsonObject & appearanceJs, const AppearanceSettings & appearance )
{
	appearanceJs["geometry"] = serialize( appearance.geometry );
	appearanceJs["app_style"] = appearance.appStyle.isNull() ? QJsonValue( QJsonValue::Null ) : appearance.appStyle;
	appearanceJs["color_scheme"] = schemeToString( appearance.colorScheme );
}

static void deserialize( const JsonObjectCtx & appearanceJs, AppearanceSettings & appearance, bool loadGeometry )
{
	if (loadGeometry)
	{
		if (JsonObjectCtx geometryJs = appearanceJs.getObject( "geometry" ))
		{
			deserialize( geometryJs, appearance.geometry );
		}
	}

	appearance.appStyle = appearanceJs.getString( "app_style", {}, DontShowError );  // null value means system-default

	ColorScheme colorScheme = schemeFromString( appearanceJs.getString( "color_scheme" ) );
	if (colorScheme != ColorScheme::_EnumEnd)
		appearance.colorScheme = colorScheme;  // otherwise leave default
}


//======================================================================================================================
// top-level JSON stucture

static void serialize( QJsonObject & rootJs, const OptionsToSave & opts )
{
	// files and related settings

	{
		// better keep room for adding some engine settings later, so that we don't have to break compatibility again
		QJsonObject enginesJs = serialize( opts.engineSettings );

		enginesJs["engine_list"] = serializeList( opts.engines );  // serializes only Engine fields, leaves other EngineInfo fields alone

		rootJs["engines"] = enginesJs;
	}

	{
		QJsonObject iwadsJs = serialize( opts.iwadSettings );

		if (!opts.iwadSettings.updateFromDir)
			iwadsJs["IWAD_list"] = serializeList( opts.iwads );

		rootJs["IWADs"] = iwadsJs;
	}

	rootJs["maps"] = serialize( opts.mapSettings );

	rootJs["mods"] = serialize( opts.modSettings );

	// options

	if (opts.settings.launchOptsStorage == StoreGlobally)
		rootJs["launch_options"] = serialize( opts.launchOpts );

	if (opts.settings.launchOptsStorage == StoreGlobally)
		rootJs["multiplayer_options"] = serialize( opts.multOpts );

	if (opts.settings.gameOptsStorage == StoreGlobally)
		rootJs["gameplay_options"] = serialize( opts.gameOpts );

	if (opts.settings.compatOptsStorage == StoreGlobally)
		rootJs["compatibility_options"] = serialize( opts.compatOpts );

	if (opts.settings.videoOptsStorage == StoreGlobally)
		rootJs["video_options"] = serialize( opts.videoOpts );

	if (opts.settings.audioOptsStorage == StoreGlobally)
		rootJs["audio_options"] = serialize( opts.audioOpts );

	rootJs["global_options"] = serialize( opts.globalOpts );

	// presets

	{
		QJsonArray presetArrayJs;

		for (const Preset & preset : opts.presets)
		{
			QJsonObject presetJs = serialize( preset, opts.settings );
			presetArrayJs.append( presetJs );
		}

		rootJs["presets"] = presetArrayJs;
	}

	rootJs["selected_preset"] = opts.selectedPresetIdx >= 0 ? opts.presets[ opts.selectedPresetIdx ].name : QString();

	// global settings - serialize directly to root, so that we don't have to break compatibility with older options

	serialize( rootJs, opts.settings );

	serialize( rootJs, opts.appearance );
}

static void deserialize( const JsonObjectCtx & rootJs, OptionsToLoad & opts )
{
	// global settings - deserialize directly from root, so that we don't have to break compatibility with older options

	// This must be loaded early, because we need to know whether to attempt loading the opts from the presets or globally.
	deserialize( rootJs, opts.settings );

	// files and related settings

	if (JsonObjectCtx enginesJs = rootJs.getObject( "engines" ))
	{
		deserialize( enginesJs, opts.engineSettings );

		if (JsonArrayCtx engineArrayJs = enginesJs.getArray( "engine_list" ))
		{
			// iterate manually, so that we can filter-out invalid items
			opts.engines.reserve( engineArrayJs.size() );
			for (qsize_t i = 0; i < engineArrayJs.size(); i++)
			{
				JsonObjectCtx engineJs = engineArrayJs.getObject( i );
				if (!engineJs)  // wrong type on position i -> skip this entry
					continue;

				Engine engine;
				deserialize( engineJs, engine );

				bool isValid = engine.name != InvalidItemName && engine.executablePath != InvalidItemPath
					&& (engine.executablePath.isEmpty() || PathChecker::checkFilePath( engine.executablePath, true, "an Engine from the saved options", "Please update it in Menu -> Initial Setup." ));
				if (!isValid)
					highlightListItemAsInvalid( engine );

				opts.engines.append( EngineInfo( std::move(engine) ) );  // populates only Engine fields, leaves other EngineInfo fields empty
			}
		}
	}

	if (JsonObjectCtx iwadsJs = rootJs.getObject( "IWADs" ))
	{
		deserialize( iwadsJs, opts.iwadSettings );

		if (opts.iwadSettings.updateFromDir)
		{
			PathChecker::checkOnlyNonEmptyDirPath( opts.iwadSettings.dir, true, "IWAD directory from the saved options", "Please update it in Menu -> Initial Setup." );
		}
		else
		{
			if (JsonArrayCtx iwadArrayJs = iwadsJs.getArray( "IWAD_list" ))
			{
				// iterate manually, so that we can filter-out invalid items
				opts.iwads.reserve( iwadArrayJs.size() );
				for (qsize_t i = 0; i < iwadArrayJs.size(); i++)
				{
					JsonObjectCtx iwadJs = iwadArrayJs.getObject( i );
					if (!iwadJs)  // wrong type on position i - skip this entry
						continue;

					IWAD iwad;
					deserialize( iwadJs, iwad );

					bool isValid = iwad.name != InvalidItemName && iwad.path != InvalidItemPath
						&& (iwad.path.isEmpty() || PathChecker::checkFilePath( iwad.path, true, "an IWAD from the saved options", "Please update it in Menu -> Initial Setup." ));
					if (!isValid)
						highlightListItemAsInvalid( iwad );

					opts.iwads.append( std::move( iwad ) );
				}
			}
		}
	}

	if (JsonObjectCtx mapsJs = rootJs.getObject( "maps" ))
	{
		deserialize( mapsJs, opts.mapSettings );

		PathChecker::checkOnlyNonEmptyDirPath( opts.mapSettings.dir, true, "map directory from the saved options", "Please update it in Menu -> Initial Setup." );
	}

	if (JsonObjectCtx modsJs = rootJs.getObject( "mods" ))
	{
		deserialize( modsJs, opts.modSettings );
	}

	// options

	if (opts.settings.launchOptsStorage == StoreGlobally)
		if (JsonObjectCtx optsJs = rootJs.getObject( "launch_options" ))
			deserialize( optsJs, opts.launchOpts );

	if (opts.settings.launchOptsStorage == StoreGlobally)
		if (JsonObjectCtx optsJs = rootJs.getObject( "multiplayer_options" ))
			deserialize( optsJs, opts.multOpts );

	if (opts.settings.gameOptsStorage == StoreGlobally)
		if (JsonObjectCtx optsJs = rootJs.getObject( "gameplay_options" ))
			deserialize( optsJs, opts.gameOpts );

	if (opts.settings.compatOptsStorage == StoreGlobally)
		if (JsonObjectCtx optsJs = rootJs.getObject( "compatibility_options" ))
			deserialize( optsJs, opts.compatOpts );

	if (opts.settings.videoOptsStorage == StoreGlobally)
		if (JsonObjectCtx optsJs = rootJs.getObject( "video_options" ))
			deserialize( optsJs, opts.videoOpts );

	if (opts.settings.audioOptsStorage == StoreGlobally)
		if (JsonObjectCtx optsJs = rootJs.getObject( "audio_options" ))
			deserialize( optsJs, opts.audioOpts );

	if (JsonObjectCtx optsJs = rootJs.getObject( "global_options" ))
		deserialize( optsJs, opts.globalOpts );

	// presets

	if (JsonArrayCtx presetArrayJs = rootJs.getArray( "presets" ))
	{
		opts.presets.reserve( presetArrayJs.size() );
		for (qsize_t i = 0; i < presetArrayJs.size(); i++)
		{
			JsonObjectCtx presetJs = presetArrayJs.getObject( i );
			if (!presetJs)  // wrong type on position i - skip this entry
				continue;

			Preset preset;
			deserialize( presetJs, preset, opts.settings );

			opts.presets.append( std::move( preset ) );
		}
	}

	opts.selectedPreset = rootJs.getString( "selected_preset" );
}


//======================================================================================================================
// backward compatibility - loading user data from older options format

#include "OptionsSerializer_compat.cpp"  // hack, but it's ok in this case


//======================================================================================================================
// top-level API

QJsonDocument serializeOptionsToJsonDoc( const OptionsToSave & opts )
{
	QJsonObject rootJs;

	// this will be used to detect options created by older versions and supress "missing element" warnings
	rootJs["version"] = appVersion;

	serialize( rootJs, opts );

	return QJsonDocument( rootJs );
}

void deserializeAppearanceFromJsonDoc( const JsonDocumentCtx & jsonDoc, AppearanceToLoad & opts, bool loadGeometry )
{
	const JsonObjectCtx & rootJs = jsonDoc.rootObject();

	// deserialize directly from root, so that we don't have to break compatibility with older options
	deserialize( rootJs, opts.appearance, loadGeometry );
}

void deserializeOptionsFromJsonDoc( const JsonDocumentCtx & jsonDoc, OptionsToLoad & opts )
{
	// We use this contextual mechanism instead of standard JSON getters, because when something fails to load
	// we want to print a useful error message with information exactly which JSON element is broken.
	const JsonObjectCtx & rootJs = jsonDoc.rootObject();

	QString optsVersionStr = rootJs.getString( "version", {}, DontShowError );
	opts.version = Version( optsVersionStr );

	if (!optsVersionStr.isEmpty() && opts.version > appVersion)  // empty version means pre-1.4 version
	{
		reportRuntimeError( nullptr, "Loading options from newer version",
			"Detected saved options from newer version of DoomRunner. "
			"Some settings might not be compatible. Expect errors."
		);
	}

	// backward compatibility with older options format
	if (optsVersionStr.isEmpty() || opts.version < Version(1,7))
	{
		jsonDoc.disableWarnings();  // supress "missing element" warnings when loading older version

		// try to load as the 1.6.3 format, older versions will have to accept resetting some values to defaults
		deserialize_pre17( rootJs, opts );
	}
	else
	{
		if (opts.version < appVersion)
			jsonDoc.disableWarnings();  // supress "missing element" warnings when loading older version

		deserialize( rootJs, opts );
	}
}
