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

static void deserialize_pre17( const JsonObjectCtx & jsIWADs, IwadSettings & iwadSettings )
{
	iwadSettings.updateFromDir = jsIWADs.getBool( "auto_update", iwadSettings.updateFromDir );
	iwadSettings.dir = jsIWADs.getString( "directory" );
	iwadSettings.searchSubdirs = jsIWADs.getBool( "subdirs", iwadSettings.searchSubdirs );
}

static void deserialize_pre17( const JsonObjectCtx & jsOptions, GameplayOptions & opts )
{
	opts.skillNum = jsOptions.getInt( "skill_num", opts.skillNum );
	opts.skillIdx = opts.skillNum;
	opts.noMonsters = jsOptions.getBool( "no_monsters", opts.noMonsters );
	opts.fastMonsters = jsOptions.getBool( "fast_monsters", opts.fastMonsters );
	opts.monstersRespawn = jsOptions.getBool( "monsters_respawn", opts.monstersRespawn );
	opts.dmflags1 = jsOptions.getInt( "dmflags1", opts.dmflags1 );
	opts.dmflags2 = jsOptions.getInt( "dmflags2", opts.dmflags2 );
	opts.allowCheats = jsOptions.getBool( "allow_cheats", opts.allowCheats );
}

static void deserialize_pre17( const JsonObjectCtx & jsPreset, Preset & preset, const StorageSettings & settings )
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

			Mod mod( /*checked*/false );
			deserialize( jsMod, mod );

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
		if (JsonObjectCtx jsOptions = jsPreset.getObject( "launch_options" ))
		{
			deserialize( jsOptions, preset.launchOpts );

			deserialize( jsOptions, preset.multOpts );

			deserialize_pre17( jsOptions, preset.gameOpts );

			deserialize( jsOptions, preset.compatOpts );

			deserialize( jsOptions, preset.altPaths );
		}
	}

	// preset-specific args

	preset.cmdArgs = jsPreset.getString( "additional_args" );
}

static void deserialize_pre17( const JsonObjectCtx & jsSettings, LauncherSettings & settings )
{
	// leave appStyle and colorScheme at their defaults

	bool useAbsolutePaths = jsSettings.getBool( "use_absolute_paths", settings.pathStyle == PathStyle::Absolute );
	settings.pathStyle = useAbsolutePaths ? PathStyle::Absolute : PathStyle::Relative;

	settings.showEngineOutput = jsSettings.getBool( "show_engine_output", settings.showEngineOutput, DontShowError );
	settings.closeOnLaunch = jsSettings.getBool( "close_on_launch", settings.closeOnLaunch, DontShowError );
	settings.checkForUpdates = jsSettings.getBool( "check_for_updates", settings.checkForUpdates, DontShowError );
	settings.askForSandboxPermissions = jsSettings.getBool( "ask_for_sandbox_permissions", settings.askForSandboxPermissions, DontShowError );

	OptionsStorage storage = jsSettings.getEnum< OptionsStorage >( "options_storage", settings.launchOptsStorage );
	settings.launchOptsStorage = storage;
	settings.gameOptsStorage = storage;
	settings.compatOptsStorage = storage;
	settings.videoOptsStorage = storage;
	settings.audioOptsStorage = storage;
}


//======================================================================================================================
//  options file stucture

static void deserialize_pre17( const JsonObjectCtx & jsOpts, OptionsToLoad & opts )
{
	// global settings - deserialize directly from root, so that we don't have to break compatibility with older options

	// This must be loaded early, because we need to know whether to attempt loading the opts from the presets or globally.
	deserialize_pre17( jsOpts, opts.settings );

	// files and related settings

	if (JsonObjectCtx jsEngines = jsOpts.getObject( "engines" ))
	{
		if (JsonArrayCtx jsEngineArray = jsEngines.getArray( "engines" ))
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

				if (!PathChecker::checkFilePath( engine.executablePath, true, "an Engine from the saved options", "Please update it in Menu -> Setup." ))
					highlightInvalidListItem( engine );

				opts.engines.append( EngineInfo( std::move(engine) ) );
			}
		}
	}

	if (JsonObjectCtx jsIWADs = jsOpts.getObject( "IWADs" ))
	{
		deserialize_pre17( jsIWADs, opts.iwadSettings );

		if (opts.iwadSettings.updateFromDir)
		{
			PathChecker::checkNonEmptyDirPath( opts.iwadSettings.dir, true, "IWAD directory from the saved options", "Please update it in Menu -> Setup." );
		}
		else
		{
			if (JsonArrayCtx jsIWADArray = jsIWADs.getArray( "IWADs" ))
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

					if (!PathChecker::checkFilePath( iwad.path, true, "an IWAD from the saved options", "Please update it in Menu -> Setup." ))
						highlightInvalidListItem( iwad );

					opts.iwads.append( std::move( iwad ) );
				}
			}
		}
	}

	if (JsonObjectCtx jsMaps = jsOpts.getObject( "maps" ))
	{
		deserialize( jsMaps, opts.mapSettings );

		PathChecker::checkNonEmptyDirPath( opts.mapSettings.dir, true, "map directory from the saved options", "Please update it in Menu -> Setup." );
	}

	if (JsonObjectCtx jsMods = jsOpts.getObject( "mods" ))
	{
		deserialize( jsMods, opts.modSettings );
	}

	// options

	if (opts.settings.launchOptsStorage == StoreGlobally)
	{
		if (JsonObjectCtx jsOptions = jsOpts.getObject( "launch_options" ))
		{
			deserialize( jsOptions, opts.launchOpts );

			deserialize( jsOptions, opts.multOpts );

			deserialize_pre17( jsOptions, opts.gameOpts );

			deserialize( jsOptions, opts.compatOpts );
		}
	}

	if (JsonObjectCtx jsOptions = jsOpts.getObject( "output_options" ))
	{
		deserialize( jsOptions, opts.videoOpts );

		deserialize( jsOptions, opts.audioOpts );
	}

	deserialize( jsOpts, opts.globalOpts );

	// presets

	if (JsonArrayCtx jsPresetArray = jsOpts.getArray( "presets" ))
	{
		for (int i = 0; i < jsPresetArray.size(); i++)
		{
			JsonObjectCtx jsPreset = jsPresetArray.getObject( i );
			if (!jsPreset)  // wrong type on position i - skip this entry
				continue;

			Preset preset;
			deserialize_pre17( jsPreset, preset, opts.settings );

			opts.presets.append( std::move( preset ) );
		}
	}

	opts.selectedPreset = jsOpts.getString( "selected_preset" );
}
