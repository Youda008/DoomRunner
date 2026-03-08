//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: deserialization of user data from older options.json structure to maintain backward compatibility
//======================================================================================================================

#include "OptionsSerializer.hpp"  // OptionsToLoad

#include "Utils/ContainerUtils.hpp"
#include "Utils/JsonUtils.hpp"
#include "Utils/PathCheckUtils.hpp"  // checkPath, highlightInvalidListItem
#include "Utils/ErrorHandling.hpp"

#include <QStringBuilder>


//======================================================================================================================
// options

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

static void deserialize_pre17( Preset & preset, const JsonObjectCtx & presetJs, const StorageSettings & settings )
{
	preset.name = presetJs.getString( "name", InvalidItemName, MustBePresent, MustNotBeEmpty );

	preset.isSeparator = presetJs.getBool( "separator", false, AllowMissing );
	if (preset.isSeparator)
	{
		return;
	}

	// files

	preset.selectedEngine = presetJs.getString( "selected_engine" );
	preset.selectedConfig = presetJs.getString( "selected_config" );
	preset.selectedIWAD = presetJs.getString( "selected_IWAD" );

	if (JsonArrayCtx selectedMapPacksJs = presetJs.getArray( "selected_mappacks" ))
	{
		preset.selectedMapPacks = deserializeStringList( selectedMapPacksJs );
	}

	if (JsonArrayCtx modsJs = presetJs.getArray( "mods" ))
	{
		// iterate manually, so that we can filter-out invalid items
		for (qsizetype i = 0; i < modsJs.size(); i++)
		{
			JsonObjectCtx modJs = modsJs.getObject( i );
			if (!modJs)  // wrong type on position i - skip this entry
				continue;

			Mod mod( /*checked*/false );
			bool isValid = mod.deserialize( modJs );
			if (!isValid)
				highlightListItemAsInvalid( mod );

			// check path or not?

			preset.mods.append( std::move( mod ) );
		}
	}

	// options

	if (settings.launchOptsStorage == StoreToPreset)
	{
		if (JsonObjectCtx optsJs = presetJs.getObject( "launch_options" ))
		{
			preset.launchOpts.deserialize( optsJs );

			preset.multOpts.deserialize( optsJs );

			deserialize_pre17( optsJs, preset.gameOpts );

			preset.compatOpts.deserialize( optsJs );

			preset.altPaths.deserialize( optsJs );
		}
	}

	// preset-specific args

	preset.cmdArgs = presetJs.getString( "additional_args" );
}


//======================================================================================================================
// settings

static void deserialize_pre17( const JsonObjectCtx & iwadSettingsJs, IwadSettings & iwadSettings )
{
	iwadSettings.updateFromDir = iwadSettingsJs.getBool( "auto_update", iwadSettings.updateFromDir );
	iwadSettings.dir = iwadSettingsJs.getString( "directory" );
	iwadSettings.searchSubdirs = iwadSettingsJs.getBool( "subdirs", iwadSettings.searchSubdirs );
}

static void deserialize_pre17( const JsonObjectCtx & settingsJs, LauncherSettings & settings )
{
	// leave appStyle and colorScheme at their defaults

	settings.pathStyle.toggleAbsolute( settingsJs.getBool( "use_absolute_paths", settings.pathStyle.isAbsolute() ) );

	settings.showEngineOutput = settingsJs.getBool( "show_engine_output", settings.showEngineOutput, AllowMissing );
	settings.closeOnLaunch = settingsJs.getBool( "close_on_launch", settings.closeOnLaunch, AllowMissing );
	settings.checkForUpdates = settingsJs.getBool( "check_for_updates", settings.checkForUpdates, AllowMissing );
	settings.askForSandboxPermissions = settingsJs.getBool( "ask_for_sandbox_permissions", settings.askForSandboxPermissions, AllowMissing );

	OptionsStorage storage = settingsJs.getEnum< OptionsStorage >( "options_storage", settings.launchOptsStorage );
	settings.launchOptsStorage = storage;
	settings.gameOptsStorage = storage;
	settings.compatOptsStorage = storage;
	settings.videoOptsStorage = storage;
	settings.audioOptsStorage = storage;
}


//======================================================================================================================
// top-level JSON stucture

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
			for (qsizetype i = 0; i < engineArrayJs.size(); i++)
			{
				JsonObjectCtx engineJs = engineArrayJs.getObject( i );
				if (!engineJs)  // wrong type on position i -> skip this entry
					continue;

				Engine engine;
				bool isValid = engine.deserialize( engineJs );
				if (!engine.isSeparator)
					isValid = isValid && PathChecker::checkFilePath( engine.executablePath, true, "an Engine from the saved options", "Please update it in Menu -> Initial Setup." );
				if (!isValid)
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
			PathChecker::checkOnlyNonEmptyDirPath( opts.iwadSettings.dir, true, "IWAD directory from the saved options", "Please update it in Menu -> Setup." );
		}
		else
		{
			if (JsonArrayCtx iwadArrayJs = iwadsJs.getArray( "IWADs" ))
			{
				// iterate manually, so that we can filter-out invalid items
				for (qsizetype i = 0; i < iwadArrayJs.size(); i++)
				{
					JsonObjectCtx iwadJs = iwadArrayJs.getObject( i );
					if (!iwadJs)  // wrong type on position i - skip this entry
						continue;

					IWAD iwad;
					bool isValid = iwad.deserialize( iwadJs );
					if (!iwad.isSeparator)
						isValid = isValid && PathChecker::checkFilePath( iwad.path, true, "an IWAD from the saved options", "Please update it in Menu -> Initial Setup." );
					if (!isValid)
						highlightListItemAsInvalid( iwad );

					opts.iwads.append( std::move( iwad ) );
				}
			}
		}
	}

	if (JsonObjectCtx mapsJs = optsJs.getObject( "maps" ))
	{
		opts.mapSettings.deserialize( mapsJs );

		PathChecker::checkOnlyNonEmptyDirPath( opts.mapSettings.dir, true, "map directory from the saved options", "Please update it in Menu -> Setup." );
	}

	if (JsonObjectCtx modsJs = optsJs.getObject( "mods" ))
	{
		opts.modSettings.deserialize( modsJs );
	}

	// options

	if (opts.settings.launchOptsStorage == StoreGlobally)
	{
		if (JsonObjectCtx launchOptsJs = optsJs.getObject( "launch_options" ))
		{
			opts.launchOpts.deserialize( launchOptsJs );

			opts.multOpts.deserialize( launchOptsJs );

			deserialize_pre17( launchOptsJs, opts.gameOpts );

			opts.compatOpts.deserialize( launchOptsJs );
		}
	}

	if (JsonObjectCtx outOptsJs = optsJs.getObject( "output_options" ))
	{
		opts.videoOpts.deserialize( outOptsJs );

		opts.audioOpts.deserialize( outOptsJs );
	}

	opts.globalOpts.deserialize( optsJs );

	// presets

	if (JsonArrayCtx presetArrayJs = optsJs.getArray( "presets" ))
	{
		for (qsizetype i = 0; i < presetArrayJs.size(); i++)
		{
			JsonObjectCtx presetJs = presetArrayJs.getObject( i );
			if (!presetJs)  // wrong type on position i - skip this entry
				continue;

			Preset preset;
			deserialize_pre17( preset, presetJs, opts.settings );

			// Until 1.9.2 the preset.selectedEngine contained engine.executablePath, but we need it to be engine.id.
			if (!preset.isSeparator && !preset.selectedEngine.isEmpty())
			{
				int engineIdx = findSuch( opts.engines, [ &preset ]( const EngineInfo & engine )
				                                        { return engine.executablePath == preset.selectedEngine; } );
				if (engineIdx >= 0)
				{
					preset.selectedEngine = opts.engines[ engineIdx ].getID();
				}
				else
				{
					reportUserError( nullptr, "Engine no longer exists",
						"Engine \""%preset.selectedEngine%"\" selected for preset \""%preset.name%"\" "
						"was removed from engine list. Please select another one."
					);
				}
			}

			opts.presets.append( std::move( preset ) );
		}
	}

	opts.selectedPreset = optsJs.getString( "selected_preset" );
}
