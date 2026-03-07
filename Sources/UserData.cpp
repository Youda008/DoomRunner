//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: the data user enters into the launcher
//======================================================================================================================

#include "UserData.hpp"

#include "Utils/JsonUtils.hpp"  // serialization/deserialization
#include "Utils/FileSystemUtils.hpp"  // getParentDir

#include <QIcon>
#include <QFileIconProvider>
#include <QString>
#include <QHash>

#include <optional>


//======================================================================================================================
// icons


static const QIcon emptyIcon;

// Not sure how QIcon is implemented and how heavy is its construction and copying,
// so we better cache it and return references.
// Especially on Windows the icon loading via QFileIconProvider seems notably slow.
static QHash< QString, QIcon > g_filesystemIconCache;

// better construct it only once rather than in every call
static std::optional< QFileIconProvider > g_iconProvider;

const QIcon & Mod::getIcon() const
{
	if (isCmdArg)
	{
		return emptyIcon;
	}

	if (!g_iconProvider)
	{
		g_iconProvider.emplace();
		g_iconProvider->setOptions( QFileIconProvider::DontUseCustomDirectoryIcons );  // custom dir icons might cause freezes
	}

	// File icons are mostly determined by file suffix, so we can cache the icons only for the suffixes to load less
	// icons in total. The only exception is when a file has no suffix on Linux, then the icon can be determined
	// by file header, but those files will not be used as mods, so we can ignore that.
	// Special handling is needed for directories, because they don't have suffixes (usually),
	// but we want to display them differently than files without suffixes.

	QFileInfo entryInfo( this->path );
	QString entryID = entryInfo.isDir() ? "<dir>" : entryInfo.suffix().toLower();

	auto iter = g_filesystemIconCache.find( entryID );
	if (iter == g_filesystemIconCache.end())
	{
		QIcon origIcon = g_iconProvider->icon( entryInfo );

		// strip the icon from unnecessary high-res variants that slow down the painting process
		QSize smallestSize = origIcon.availableSizes().at(0);
		QPixmap pixmap = origIcon.pixmap( smallestSize );

		iter = g_filesystemIconCache.insert( std::move( entryID ), QIcon( pixmap ) );
	}
	return iter.value();
}




//======================================================================================================================
// serialization/deserialization


const QString InvalidItemValue = "<invalid value>";
const QString InvalidItemName = "<invalid name>";
const QString InvalidItemPath = "<invalid path>";


//----------------------------------------------------------------------------------------------------------------------
// custom data types

static QJsonValue colorToJson( const QColor & color )
{
	if (color.isValid())
		return color.name( QColor::HexRgb );
	else
		return QJsonValue::Null;
}

static QColor getColorFromJson( const JsonObjectCtx & parentObj, const QString & key )
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


//----------------------------------------------------------------------------------------------------------------------
// options - files

QJsonObject Engine::serialize() const
{
	const Engine & engine = *this;
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

bool Engine::deserialize( const JsonObjectCtx & engineJs )
{
	Engine & engine = *this;
	bool isValid = true;

	engine.isSeparator = engineJs.getBool( "separator", false, AllowMissing );
	if (engine.isSeparator)
	{
		engine.name = engineJs.getString( "name", InvalidItemName, MustBePresent, MustNotBeEmpty );

		isValid = engine.name != InvalidItemName;
	}
	else
	{
		engine.name = engineJs.getString( "name", InvalidItemName, MustBePresent, MustNotBeEmpty );
		engine.executablePath = engineJs.getString( "path", InvalidItemPath, MustBePresent, MustNotBeEmpty );
		if (engine.executablePath != InvalidItemPath)
		{
			engine.configDir = engineJs.getString( "config_dir", fs::getParentDir( engine.executablePath ), MustBePresent, MustNotBeEmpty );
			engine.dataDir = engineJs.getString( "data_dir", engine.configDir, MustBePresent, MustNotBeEmpty );
		}
		engine.family = familyFromStr( engineJs.getString( "family", {}, MustBePresent, MustNotBeEmpty ) );

		isValid = engine.name != InvalidItemName
		       && engine.executablePath != InvalidItemPath
		       && engine.family != EngineFamily::_EnumEnd;
	}

	return isValid;
}

QJsonObject IWAD::serialize() const
{
	const IWAD & iwad = *this;
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

bool IWAD::deserialize( const JsonObjectCtx & iwadJs )
{
	IWAD & iwad = *this;
	bool isValid = true;

	iwad.isSeparator = iwadJs.getBool( "separator", false, AllowMissing );
	if (iwad.isSeparator)
	{
		iwad.name = iwadJs.getString( "name", InvalidItemName, MustBePresent, MustNotBeEmpty );

		isValid = iwad.name != InvalidItemName;
	}
	else
	{
		iwad.path = iwadJs.getString( "path", InvalidItemPath, MustBePresent, MustNotBeEmpty );
		iwad.name = iwadJs.getString( "name", iwad.path != InvalidItemPath ? QFileInfo( iwad.path ).fileName() : InvalidItemName, MustBePresent, MustNotBeEmpty );

		isValid = iwad.name != InvalidItemName
		       && iwad.path != InvalidItemPath;
	}

	return isValid;
}

QJsonObject Mod::serialize() const
{
	const Mod & mod = *this;
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

bool Mod::deserialize( const JsonObjectCtx & modJs )
{
	Mod & mod = *this;
	bool isValid = true;

	if ((mod.isSeparator = modJs.getBool( "separator", false, AllowMissing )))
	{
		mod.name = modJs.getString( "name", InvalidItemName, MustBePresent, MustNotBeEmpty );

		isValid = mod.name != InvalidItemName;
	}
	else if ((mod.isCmdArg = modJs.getBool( "cmd_argument", false, AllowMissing )))
	{
		mod.name = modJs.getString( "value", InvalidItemName, MustBePresent, MustNotBeEmpty );
		mod.checked = modJs.getBool( "checked", mod.checked );

		isValid = mod.name != InvalidItemName;
	}
	else
	{
		mod.path = modJs.getString( "path", InvalidItemPath, MustBePresent, MustNotBeEmpty );
		mod.name = mod.path != InvalidItemPath ? QFileInfo( mod.path ).fileName() : InvalidItemName;
		mod.checked = modJs.getBool( "checked", mod.checked );

		isValid = !mod.name.isEmpty() && mod.name != InvalidItemName
		       && !mod.path.isEmpty() && mod.path != InvalidItemPath;
	}

	return isValid;
}


//----------------------------------------------------------------------------------------------------------------------
// gameplay/compatibility options

QJsonObject LaunchOptions::serialize() const
{
	const LaunchOptions & opts = *this;
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

void LaunchOptions::deserialize( const JsonObjectCtx & optsJs )
{
	LaunchOptions & opts = *this;

	opts.mode = optsJs.getEnum< LaunchMode >( "launch_mode", opts.mode );
	opts.mapName = optsJs.getString( "map_name" );
	opts.saveFile = optsJs.getString( "save_file" );
	opts.mapName_demo = optsJs.getString( "map_name_demo" );
	opts.demoFile_record = optsJs.getString( "demo_file_record" );
	opts.demoFile_replay = optsJs.getString( "demo_file_replay" );
	opts.demoFile_resumeFrom = optsJs.getString( "demo_file_resume_from" );
	opts.demoFile_resumeTo = optsJs.getString( "demo_file_resume_to" );
}

QJsonObject MultiplayerOptions::serialize() const
{
	const MultiplayerOptions & opts = *this;
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
	optsJs["player_color"] = colorToJson( opts.playerColor );

	return optsJs;
}

void MultiplayerOptions::deserialize( const JsonObjectCtx & optsJs )
{
	MultiplayerOptions & opts = *this;

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
	opts.playerColor = getColorFromJson( optsJs, "player_color" );
}

QJsonObject GameplayOptions::serialize() const
{
	const GameplayOptions & opts = *this;
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

void GameplayOptions::deserialize( const JsonObjectCtx & optsJs )
{
	GameplayOptions & opts = *this;

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

QJsonObject CompatibilityOptions::serialize() const
{
	const CompatibilityOptions & opts = *this;
	QJsonObject optsJs;

	optsJs["compat_mode"] = opts.compatMode;
	optsJs["compatflags1"] = qint64( opts.compatflags1 );
	optsJs["compatflags2"] = qint64( opts.compatflags2 );

	return optsJs;
}

void CompatibilityOptions::deserialize( const JsonObjectCtx & optsJs )
{
	CompatibilityOptions & opts = *this;

	opts.compatflags1 = optsJs.getInt( "compatflags1", opts.compatflags1 );
	opts.compatflags2 = optsJs.getInt( "compatflags2", opts.compatflags2 );
	if (optsJs.hasMember("compat_level"))
		opts.compatMode = optsJs.getInt( "compat_level", opts.compatMode );
	else
		opts.compatMode = optsJs.getInt( "compat_mode", opts.compatMode );
}


//----------------------------------------------------------------------------------------------------------------------
// other options

QJsonObject AlternativePaths::serialize() const
{
	const AlternativePaths & opts = *this;
	QJsonObject optsJs;

	optsJs["config_dir"] = opts.configDir;
	optsJs["save_dir"] = opts.saveDir;
	optsJs["demo_dir"] = opts.demoDir;
	optsJs["screenshot_dir"] = opts.screenshotDir;

	return optsJs;
}

void AlternativePaths::deserialize( const JsonObjectCtx & optsJs )
{
	AlternativePaths & opts = *this;

	opts.configDir = optsJs.getString( "config_dir" );
	opts.saveDir = optsJs.getString( "save_dir" );
	opts.demoDir = optsJs.getString( "demo_dir" );
	opts.screenshotDir = optsJs.getString( "screenshot_dir" );
}

QJsonObject VideoOptions::serialize() const
{
	const VideoOptions & opts = *this;
	QJsonObject optsJs;

	optsJs["monitor_idx"] = opts.monitorIdx;
	optsJs["resolution_x"] = qint64( opts.resolutionX );
	optsJs["resolution_y"] = qint64( opts.resolutionY );
	optsJs["show_fps"] = opts.showFPS;

	return optsJs;
}

void VideoOptions::deserialize( const JsonObjectCtx & optsJs )
{
	VideoOptions & opts = *this;

	opts.monitorIdx = optsJs.getInt( "monitor_idx", opts.monitorIdx );
	opts.resolutionX = optsJs.getUInt( "resolution_x", opts.resolutionX );
	opts.resolutionY = optsJs.getUInt( "resolution_y", opts.resolutionY );
	opts.showFPS = optsJs.getBool( "show_fps", opts.showFPS );
}

QJsonObject AudioOptions::serialize() const
{
	const AudioOptions & opts = *this;
	QJsonObject optsJs;

	optsJs["no_sound"] = opts.noSound;
	optsJs["no_sfx"] = opts.noSFX;
	optsJs["no_music"] = opts.noMusic;

	return optsJs;
}

void AudioOptions::deserialize( const JsonObjectCtx & optsJs )
{
	AudioOptions & opts = *this;

	opts.noSound = optsJs.getBool( "no_sound", opts.noSound );
	opts.noSFX = optsJs.getBool( "no_sfx", opts.noSFX );
	opts.noMusic = optsJs.getBool( "no_music", opts.noMusic );
}

QJsonObject serialize( const EnvVars & envVars )
{
	QJsonObject envVarsJs;

	// convert list to a map
	for (const auto & envVar : envVars)
	{
		envVarsJs[ envVar.name ] = envVar.value;
	}

	return envVarsJs;
}

void deserialize( const JsonObjectCtx & envVarsJs, EnvVars & envVars )
{
	// convert map to a list
	auto keys = envVarsJs.keys();
	for (auto & varName : keys)
	{
		QString value = envVarsJs.getString( varName, InvalidItemValue, MustBePresent, AllowEmpty );
		envVars.append({ std::move(varName), std::move(value) });
	}

	// keep the list sorted
	std::sort( envVars.begin(), envVars.end(), []( const auto & a, const auto & b ) { return a.name < b.name; });
}

QJsonObject GlobalOptions::serialize() const
{
	const GlobalOptions & opts = *this;
	QJsonObject optsJs;

	optsJs["use_preset_name_as_config_dir"] = opts.usePresetNameAsConfigDir;
	optsJs["use_preset_name_as_save_dir"] = opts.usePresetNameAsSaveDir;
	optsJs["use_preset_name_as_demo_dir"] = opts.usePresetNameAsDemoDir;
	optsJs["use_preset_name_as_screenshot_dir"] = opts.usePresetNameAsScreenshotDir;
	optsJs["additional_args"] = opts.cmdArgs;
	optsJs["cmd_prefix"] = opts.cmdPrefix;
	optsJs["env_vars"] = ::serialize( opts.envVars );

	return optsJs;
}

void GlobalOptions::deserialize( const JsonObjectCtx & optsJs )
{
	GlobalOptions & opts = *this;

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
		::deserialize( jsEnvVars, opts.envVars );
}


//----------------------------------------------------------------------------------------------------------------------
// settings & state

QJsonObject EngineSettings::serialize() const
{
	const EngineSettings & settings = *this;
	QJsonObject settingsJs;

	settingsJs["default_engine"] = settings.defaultEngine;

	return settingsJs;
}

void EngineSettings::deserialize( const JsonObjectCtx & settingsJs )
{
	EngineSettings & settings = *this;

	settings.defaultEngine = settingsJs.getString( "default_engine", {}, AllowMissing );
}

QJsonObject IwadSettings::serialize() const
{
	const IwadSettings & settings = *this;
	QJsonObject settingsJs;

	settingsJs["auto_update"] = settings.updateFromDir;
	settingsJs["directory"] = settings.dir;
	settingsJs["search_subdirs"] = settings.searchSubdirs;
	settingsJs["default_iwad"] = settings.defaultIWAD;

	return settingsJs;
}

void IwadSettings::deserialize( const JsonObjectCtx & settingsJs )
{
	IwadSettings & settings = *this;

	settings.updateFromDir = settingsJs.getBool( "auto_update", settings.updateFromDir );
	settings.dir = settingsJs.getString( "directory" );
	settings.searchSubdirs = settingsJs.getBool( "search_subdirs", settings.searchSubdirs );
	settings.defaultIWAD = settingsJs.getString( "default_iwad", {}, AllowMissing );
}

QJsonObject MapSettings::serialize() const
{
	const MapSettings & settings = *this;
	QJsonObject settingsJs;

	settingsJs["directory"] = settings.dir;
	settingsJs["sort_column"] = settings.sortColumn;
	settingsJs["sort_order"] = int( settings.sortOrder );
	settingsJs["show_icons"] = settings.showIcons;

	return settingsJs;
}

void MapSettings::deserialize( const JsonObjectCtx & settingsJs )
{
	MapSettings & settings = *this;

	settings.dir = settingsJs.getString( "directory" );
	settings.sortColumn = settingsJs.getInt( "sort_column", settings.sortColumn );
	settings.sortOrder = settingsJs.getEnum< Qt::SortOrder >( "sort_order", settings.sortOrder );
	settings.showIcons = settingsJs.getBool( "show_icons", settings.showIcons );
}

QJsonObject ModSettings::serialize() const
{
	const ModSettings & settings = *this;
	QJsonObject settingsJs;

	settingsJs["last_used_dir"] = settings.lastUsedDir;
	settingsJs["show_icons"] = settings.showIcons;

	return settingsJs;
}

void ModSettings::deserialize( const JsonObjectCtx & settingsJs )
{
	ModSettings & settings = *this;

	settings.lastUsedDir = settingsJs.getString( "last_used_dir" );
	settings.showIcons = settingsJs.getBool( "show_icons", settings.showIcons );
}

QJsonObject StorageSettings::serialize() const
{
	const StorageSettings & settings = *this;
	QJsonObject settingsJs;

	settingsJs["launch_opts"] = int( settings.launchOptsStorage );
	settingsJs["gameplay_opts"] = int( settings.gameOptsStorage );
	settingsJs["compat_opts"] = int( settings.compatOptsStorage );
	settingsJs["video_opts"] = int( settings.videoOptsStorage );
	settingsJs["audio_opts"] = int( settings.audioOptsStorage );

	return settingsJs;
}

void StorageSettings::deserialize( const JsonObjectCtx & settingsJs )
{
	StorageSettings & settings = *this;

	settings.launchOptsStorage = settingsJs.getEnum< OptionsStorage >( "launch_opts", settings.launchOptsStorage );
	settings.gameOptsStorage = settingsJs.getEnum< OptionsStorage >( "gameplay_opts", settings.gameOptsStorage );
	settings.compatOptsStorage = settingsJs.getEnum< OptionsStorage >( "compat_opts", settings.compatOptsStorage );
	settings.videoOptsStorage = settingsJs.getEnum< OptionsStorage >( "video_opts", settings.videoOptsStorage );
	settings.audioOptsStorage = settingsJs.getEnum< OptionsStorage >( "audio_opts", settings.audioOptsStorage );
}

void LauncherSettings::serialize( QJsonObject & settingsJs ) const
{
	const LauncherSettings & settings = *this;

	settingsJs["use_absolute_paths"] = settings.pathStyle.isAbsolute();
	settingsJs["show_engine_output"] = settings.showEngineOutput;
	settingsJs["close_on_launch"] = settings.closeOnLaunch;
	settingsJs["close_output_on_success"] = settings.closeOutputOnSuccess;
	settingsJs["check_for_updates"] = settings.checkForUpdates;
	settingsJs["ask_for_sandbox_permissions"] = settings.askForSandboxPermissions;
	settingsJs["wrap_lines_in_txt_viewer"] = settings.wrapLinesInTxtViewer;

	settingsJs["options_storage"] = static_cast< const StorageSettings & >( settings ).serialize();
}

void LauncherSettings::deserialize( const JsonObjectCtx & settingsJs )
{
	LauncherSettings & settings = *this;

	settings.pathStyle.toggleAbsolute( settingsJs.getBool( "use_absolute_paths", settings.pathStyle.isAbsolute() ) );

	settings.showEngineOutput = settingsJs.getBool( "show_engine_output", settings.showEngineOutput, AllowMissing );
	settings.closeOnLaunch = settingsJs.getBool( "close_on_launch", settings.closeOnLaunch, AllowMissing );
	settings.closeOutputOnSuccess = settingsJs.getBool( "close_output_on_success", settings.closeOutputOnSuccess, AllowMissing );
	settings.checkForUpdates = settingsJs.getBool( "check_for_updates", settings.checkForUpdates, AllowMissing );
	settings.askForSandboxPermissions = settingsJs.getBool( "ask_for_sandbox_permissions", settings.askForSandboxPermissions, AllowMissing );
	settings.wrapLinesInTxtViewer = settingsJs.getBool( "wrap_lines_in_txt_viewer", settings.wrapLinesInTxtViewer, AllowMissing );

	if (JsonObjectCtx optsStorageJs = settingsJs.getObject( "options_storage" ))
	{
		StorageSettings::deserialize( optsStorageJs );
	}
}


//----------------------------------------------------------------------------------------------------------------------
// UI state & appearance

QJsonObject SearchState::serialize() const
{
	const SearchState & search = *this;
	QJsonObject searchJs;

	searchJs["panel_expanded"] = search.panelExpanded;
	//searchJs["search_phrase"] = search.phrase;
	searchJs["case_sensitive"] = search.caseSensitive;
	searchJs["use_regex"] = search.useRegex;

	return searchJs;
}

void SearchState::deserialize( const JsonObjectCtx & searchJs )
{
	SearchState & search = *this;

	search.panelExpanded = searchJs.getBool( "panel_expanded", search.panelExpanded );
	//search.phrase = searchJs.getString( "search_phrase", search.phrase );
	search.caseSensitive = searchJs.getBool( "case_sensitive", search.caseSensitive );
	search.useRegex = searchJs.getBool( "use_regex", search.useRegex );
}

void UIState::serialize( QJsonObject & stateJs ) const
{
	const UIState & state = *this;

	stateJs["preset_search"] = state.presetSearch.serialize();
	stateJs["hide_map_label"] = state.hideMapHelpLabel;
}

void UIState::deserialize( const JsonObjectCtx & stateJs )
{
	UIState & state = *this;

	if (JsonObjectCtx presetSearchJs = stateJs.getObject( "preset_search" ))
	{
		state.presetSearch.deserialize( presetSearchJs );
	}
	state.hideMapHelpLabel = stateJs.getBool( "hide_map_label", state.hideMapHelpLabel, AllowMissing );
}

QJsonObject WindowGeometry::serialize() const
{
	const WindowGeometry & geometry = *this;
	QJsonObject geometryJs;

	geometryJs["x"] = geometry.x;
	geometryJs["y"] = geometry.y;
	geometryJs["width"] = geometry.width;
	geometryJs["height"] = geometry.height;

	return geometryJs;
}

void WindowGeometry::deserialize( const JsonObjectCtx & geometryJs )
{
	WindowGeometry & geometry = *this;

	geometry.x = geometryJs.getInt( "x", geometry.x );
	geometry.y = geometryJs.getInt( "y", geometry.y );
	geometry.width = geometryJs.getInt( "width", geometry.width );
	geometry.height = geometryJs.getInt( "height", geometry.height );
}

void AppearanceSettings::serialize( QJsonObject & settingsJs ) const
{
	const AppearanceSettings & settings = *this;

	settingsJs["geometry"] = settings.geometry.serialize();
	settingsJs["app_style"] = settings.appStyle.isNull() ? QJsonValue( QJsonValue::Null ) : settings.appStyle;
	settingsJs["color_scheme"] = schemeToString( settings.colorScheme );
}

void AppearanceSettings::deserialize( const JsonObjectCtx & settingsJs, bool loadGeometry )
{
	AppearanceSettings & settings = *this;

	if (loadGeometry)
	{
		if (JsonObjectCtx geometryJs = settingsJs.getObject( "geometry" ))
		{
			settings.geometry.deserialize( geometryJs );
		}
	}

	settings.appStyle = settingsJs.getString( "app_style", {}, AllowMissing );  // null value means system-default

	ColorScheme colorScheme = schemeFromString( settingsJs.getString( "color_scheme" ) );
	if (colorScheme != ColorScheme::_EnumEnd)
		settings.colorScheme = colorScheme;  // otherwise leave default
}
