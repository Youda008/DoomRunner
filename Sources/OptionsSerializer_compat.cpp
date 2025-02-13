//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: deserialization of user data from older options.json structure to maintain backward compatibility
//======================================================================================================================

#include "OptionsSerializer.hpp"  // OptionsToLoad

#include "Utils/JsonUtils.hpp"
#include "Utils/MiscUtils.hpp"  // checkPath
#include "Version.hpp"

#include <QFileInfo>


//======================================================================================================================
//  user data

static void deserialize_pre17( const JsonObjectCtx & iwadSettingsJs, IwadSettings & iwadSettings )
{
	iwadSettings.updateFromDir = iwadSettingsJs.getBool( "auto_update", iwadSettings.updateFromDir );
	iwadSettings.dir = iwadSettingsJs.getString( "directory" );
	iwadSettings.searchSubdirs = iwadSettingsJs.getBool( "subdirs", iwadSettings.searchSubdirs );
}

static void deserialize_pre17( const JsonObjectCtx & optsJs, GameplayOptions & opts )
{
	opts.skillNum = optsJs.getInt( "skill_num", opts.skillNum );
	opts.skillIdx = opts.skillNum;
	opts.noMonsters = optsJs.getBool( "no_monsters", opts.noMonsters );
	opts.fastMonsters = optsJs.getBool( "fast_monsters", opts.fastMonsters );
	opts.monstersRespawn = optsJs.getBool( "monsters_respawn", opts.monstersRespawn );
	opts.dmflags1 = optsJs.getInt( "dmflags1", opts.dmflags1 );
	opts.dmflags2 = optsJs.getInt( "dmflags2", opts.dmflags2 );
	opts.allowCheats = optsJs.getBool( "allow_cheats", opts.allowCheats );
}

static void deserialize_pre17( const JsonObjectCtx & presetJs, Preset & preset, const StorageSettings & settings )
{
	preset.name = presetJs.getString( "name", "<missing name>" );

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
		preset.selectedMapPacks = deserializeStringVec( selectedMapPacksJs );
	}

	if (JsonArrayCtx modsJs = presetJs.getArray( "mods" ))
	{
		// iterate manually, so that we can filter-out invalid items
		for (int i = 0; i < modsJs.size(); i++)
		{
			JsonObjectCtx modJs = modsJs.getObject( i );
			if (!modJs)  // wrong type on position i - skip this entry
				continue;

			Mod mod( /*checked*/false );
			deserialize( modJs, mod );

			if (mod.isSeparator && mod.fileName.isEmpty())  // element isn't present in JSON -> skip this entry
				continue;
			else if (!mod.isSeparator && mod.path.isEmpty())  // element isn't present in JSON -> skip this entry
				continue;

			// check path or not?

			preset.mods.append( std::move( mod ) );
		}
	}

	// options

	if (settings.launchOptsStorage == StoreToPreset)
	{
		if (JsonObjectCtx optsJs = presetJs.getObject( "launch_options" ))
		{
			deserialize( optsJs, preset.launchOpts );

			deserialize( optsJs, preset.multOpts );

			deserialize_pre17( optsJs, preset.gameOpts );

			deserialize( optsJs, preset.compatOpts );

			deserialize( optsJs, preset.altPaths );
		}
	}

	// preset-specific args

	preset.cmdArgs = presetJs.getString( "additional_args" );
}

static void deserialize_pre17( const JsonObjectCtx & settingsJs, LauncherSettings & settings )
{
	// leave appStyle and colorScheme at their defaults

	bool useAbsolutePaths = settingsJs.getBool( "use_absolute_paths", settings.pathStyle == PathStyle::Absolute );
	settings.pathStyle = useAbsolutePaths ? PathStyle::Absolute : PathStyle::Relative;

	settings.showEngineOutput = settingsJs.getBool( "show_engine_output", settings.showEngineOutput, DontShowError );
	settings.closeOnLaunch = settingsJs.getBool( "close_on_launch", settings.closeOnLaunch, DontShowError );
	settings.checkForUpdates = settingsJs.getBool( "check_for_updates", settings.checkForUpdates, DontShowError );
	settings.askForSandboxPermissions = settingsJs.getBool( "ask_for_sandbox_permissions", settings.askForSandboxPermissions, DontShowError );

	OptionsStorage storage = settingsJs.getEnum< OptionsStorage >( "options_storage", settings.launchOptsStorage );
	settings.launchOptsStorage = storage;
	settings.gameOptsStorage = storage;
	settings.compatOptsStorage = storage;
	settings.videoOptsStorage = storage;
	settings.audioOptsStorage = storage;
}


//======================================================================================================================
//  options file stucture

static void deserialize_pre17( const JsonObjectCtx & optsJs, OptionsToLoad & opts )
{
	// global settings - deserialize directly from root, so that we don't have to break compatibility with older options

	// This must be loaded early, because we need to know whether to attempt loading the opts from the presets or globally.
	deserialize_pre17( optsJs, opts.settings );

	// files and related settings

	if (JsonObjectCtx enginesJs = optsJs.getObject( "engines" ))
	{
		if (JsonArrayCtx engineArrayJs = enginesJs.getArray( "engines" ))
		{
			// iterate manually, so that we can filter-out invalid items
			for (int i = 0; i < engineArrayJs.size(); i++)
			{
				JsonObjectCtx engineJs = engineArrayJs.getObject( i );
				if (!engineJs)  // wrong type on position i -> skip this entry
					continue;

				Engine engine;
				deserialize( engineJs, engine );

				if (engine.executablePath.isEmpty())  // element isn't present in JSON -> skip this entry
					continue;

				if (!PathChecker::checkFilePath( engine.executablePath, true, "an Engine from the saved options", "Please update it in Menu -> Setup." ))
					highlightListItemAsInvalid( engine );

				opts.engines.append( EngineInfo( std::move(engine) ) );
			}
		}
	}

	if (JsonObjectCtx iwadsJs = optsJs.getObject( "IWADs" ))
	{
		deserialize_pre17( iwadsJs, opts.iwadSettings );

		if (opts.iwadSettings.updateFromDir)
		{
			PathChecker::checkNonEmptyDirPath( opts.iwadSettings.dir, true, "IWAD directory from the saved options", "Please update it in Menu -> Setup." );
		}
		else
		{
			if (JsonArrayCtx iwadArrayJs = iwadsJs.getArray( "IWADs" ))
			{
				// iterate manually, so that we can filter-out invalid items
				for (int i = 0; i < iwadArrayJs.size(); i++)
				{
					JsonObjectCtx iwadJs = iwadArrayJs.getObject( i );
					if (!iwadJs)  // wrong type on position i - skip this entry
						continue;

					IWAD iwad;
					deserialize( iwadJs, iwad );

					if (iwad.name.isEmpty() || iwad.path.isEmpty())  // element isn't present in JSON -> skip this entry
						continue;

					if (!PathChecker::checkFilePath( iwad.path, true, "an IWAD from the saved options", "Please update it in Menu -> Setup." ))
						highlightListItemAsInvalid( iwad );

					opts.iwads.append( std::move( iwad ) );
				}
			}
		}
	}

	if (JsonObjectCtx mapsJs = optsJs.getObject( "maps" ))
	{
		deserialize( mapsJs, opts.mapSettings );

		PathChecker::checkNonEmptyDirPath( opts.mapSettings.dir, true, "map directory from the saved options", "Please update it in Menu -> Setup." );
	}

	if (JsonObjectCtx modsJs = optsJs.getObject( "mods" ))
	{
		deserialize( modsJs, opts.modSettings );
	}

	// options

	if (opts.settings.launchOptsStorage == StoreGlobally)
	{
		if (JsonObjectCtx optsJs = optsJs.getObject( "launch_options" ))
		{
			deserialize( optsJs, opts.launchOpts );

			deserialize( optsJs, opts.multOpts );

			deserialize_pre17( optsJs, opts.gameOpts );

			deserialize( optsJs, opts.compatOpts );
		}
	}

	if (JsonObjectCtx optsJs = optsJs.getObject( "output_options" ))
	{
		deserialize( optsJs, opts.videoOpts );

		deserialize( optsJs, opts.audioOpts );
	}

	deserialize( optsJs, opts.globalOpts );

	// presets

	if (JsonArrayCtx presetArrayJs = optsJs.getArray( "presets" ))
	{
		for (int i = 0; i < presetArrayJs.size(); i++)
		{
			JsonObjectCtx presetJs = presetArrayJs.getObject( i );
			if (!presetJs)  // wrong type on position i - skip this entry
				continue;

			Preset preset;
			deserialize_pre17( presetJs, preset, opts.settings );

			opts.presets.append( std::move( preset ) );
		}
	}

	opts.selectedPreset = optsJs.getString( "selected_preset" );
}
