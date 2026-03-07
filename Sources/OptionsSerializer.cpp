//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: serialization/deserialization of the user data into/from a file
//======================================================================================================================

#include "OptionsSerializer.hpp"

#include "UserData.hpp"
#include "AppVersion.hpp"
#include "Utils/JsonUtils.hpp"
#include "Utils/PathCheckUtils.hpp"  // checkPath, highlightInvalidListItem
#include "Utils/ErrorHandling.hpp"

#include <QFileInfo>


const QString InvalidItemName = "<invalid name>";


//======================================================================================================================
// preset

QJsonObject serialize( const Preset & preset, const StorageSettings & settings )
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
		presetJs["launch_options"] = preset.launchOpts.serialize();

	if (settings.launchOptsStorage == StoreToPreset)
		presetJs["multiplayer_options"] = preset.multOpts.serialize();

	if (settings.gameOptsStorage == StoreToPreset)
		presetJs["gameplay_options"] = preset.gameOpts.serialize();

	if (settings.compatOptsStorage == StoreToPreset)
		presetJs["compatibility_options"] = preset.compatOpts.serialize();

	if (settings.videoOptsStorage == StoreToPreset)
		presetJs["video_options"] = preset.videoOpts.serialize();

	if (settings.audioOptsStorage == StoreToPreset)
		presetJs["audio_options"] = preset.audioOpts.serialize();

	presetJs["alternative_paths"] = preset.altPaths.serialize();

	// preset-specific args

	presetJs["additional_args"] = preset.cmdArgs;
	presetJs["env_vars"] = serialize( preset.envVars );

	return presetJs;
}

void deserialize( Preset & preset, const JsonObjectCtx & presetJs, const StorageSettings & settings )
{
	preset.name = presetJs.getString( "name", InvalidItemName, MustBePresent, MustNotBeEmpty );

	preset.isSeparator = presetJs.getBool( "separator", false, AllowMissing );
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
			bool isValid = mod.deserialize( modJs );
			if (!mod.isSeparator)
				//isValid = isValid && PathChecker::checkFilePath( mod.path, true, "a Mod from the saved options", "Please update it in the corresponding preset." );
			if (!isValid)
				highlightListItemAsInvalid( mod );

			preset.mods.append( std::move( mod ) );
		}
	}

	preset.loadMapsAfterMods = presetJs.getBool( "load_maps_after_mods", preset.loadMapsAfterMods );

	// options

	if (settings.launchOptsStorage == StoreToPreset)
		if (JsonObjectCtx optsJs = presetJs.getObject( "launch_options" ))
			preset.launchOpts.deserialize( optsJs );

	if (settings.launchOptsStorage == StoreToPreset)
		if (JsonObjectCtx optsJs = presetJs.getObject( "multiplayer_options" ))
			preset.multOpts.deserialize( optsJs );

	if (settings.gameOptsStorage == StoreToPreset)
		if (JsonObjectCtx optsJs = presetJs.getObject( "gameplay_options" ))
			preset.gameOpts.deserialize( optsJs );

	if (settings.compatOptsStorage == StoreToPreset)
		if (JsonObjectCtx optsJs = presetJs.getObject( "compatibility_options" ))
			preset.compatOpts.deserialize( optsJs );

	if (settings.videoOptsStorage == StoreToPreset)
		if (JsonObjectCtx optsJs = presetJs.getObject( "video_options" ))
			preset.videoOpts.deserialize( optsJs );

	if (settings.audioOptsStorage == StoreToPreset)
		if (JsonObjectCtx optsJs = presetJs.getObject( "audio_options" ))
			preset.audioOpts.deserialize( optsJs );

	if (JsonObjectCtx optsJs = presetJs.getObject( "alternative_paths" ))
		preset.altPaths.deserialize( optsJs );

	// preset-specific args

	preset.cmdArgs = presetJs.getString( "additional_args" );
	if (JsonObjectCtx envVarsJs = presetJs.getObject( "env_vars" ))
		deserialize( envVarsJs, preset.envVars );
}


//======================================================================================================================
// top-level JSON stucture

static void serialize( QJsonObject & rootJs, const OptionsToSave & opts )
{
	// files and related settings

	{
		// better keep room for adding some engine settings later, so that we don't have to break compatibility again
		QJsonObject enginesJs = opts.engineSettings.serialize();

		enginesJs["engine_list"] = serializeList( opts.engines );  // serializes only Engine fields, leaves other EngineInfo fields alone

		rootJs["engines"] = enginesJs;
	}

	{
		QJsonObject iwadsJs = opts.iwadSettings.serialize();

		if (!opts.iwadSettings.updateFromDir)
			iwadsJs["IWAD_list"] = serializeList( opts.iwads );

		rootJs["IWADs"] = iwadsJs;
	}

	{
		QJsonObject mapsJs = opts.mapSettings.serialize();

		rootJs["maps"] = mapsJs;
	}

	{
		QJsonObject modsJs = opts.modSettings.serialize();

		rootJs["mods"] = modsJs;
	}

	// options

	if (opts.settings.launchOptsStorage == StoreGlobally)
		rootJs["launch_options"] = opts.launchOpts.serialize();

	if (opts.settings.launchOptsStorage == StoreGlobally)
		rootJs["multiplayer_options"] = opts.multOpts.serialize();

	if (opts.settings.gameOptsStorage == StoreGlobally)
		rootJs["gameplay_options"] = opts.gameOpts.serialize();

	if (opts.settings.compatOptsStorage == StoreGlobally)
		rootJs["compatibility_options"] = opts.compatOpts.serialize();

	if (opts.settings.videoOptsStorage == StoreGlobally)
		rootJs["video_options"] = opts.videoOpts.serialize();

	if (opts.settings.audioOptsStorage == StoreGlobally)
		rootJs["audio_options"] = opts.audioOpts.serialize();

	rootJs["global_options"] = opts.globalOpts.serialize();

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

	opts.settings.serialize( rootJs );

	opts.appearance.serialize( rootJs );
	opts.uiState.serialize( rootJs );
}

static void deserialize( const JsonObjectCtx & rootJs, OptionsToLoad & opts )
{
	// global settings - deserialize directly from root, so that we don't have to break compatibility with older options

	// This must be loaded early, because we need to know whether to attempt loading the opts from the presets or globally.
	opts.settings.deserialize( rootJs );

	// files and related settings

	if (JsonObjectCtx enginesJs = rootJs.getObject( "engines" ))
	{
		opts.engineSettings.deserialize( enginesJs );

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
				bool isValid = engine.deserialize( engineJs );
				if (!engine.isSeparator)
					isValid = isValid && PathChecker::checkFilePath( engine.executablePath, true, "an Engine from the saved options", "Please update it in Menu -> Initial Setup." );
				if (!isValid)
					highlightListItemAsInvalid( engine );

				opts.engines.append( EngineInfo( std::move(engine) ) );  // populates only Engine fields, leaves other EngineInfo fields empty
			}
		}
	}

	if (JsonObjectCtx iwadsJs = rootJs.getObject( "IWADs" ))
	{
		opts.iwadSettings.deserialize( iwadsJs );

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

	if (JsonObjectCtx mapsJs = rootJs.getObject( "maps" ))
	{
		opts.mapSettings.deserialize( mapsJs );

		PathChecker::checkOnlyNonEmptyDirPath( opts.mapSettings.dir, true, "map directory from the saved options", "Please update it in Menu -> Initial Setup." );
	}

	if (JsonObjectCtx modsJs = rootJs.getObject( "mods" ))
	{
		opts.modSettings.deserialize( modsJs );
	}

	// options

	if (opts.settings.launchOptsStorage == StoreGlobally)
		if (JsonObjectCtx optsJs = rootJs.getObject( "launch_options" ))
			opts.launchOpts.deserialize( optsJs );

	if (opts.settings.launchOptsStorage == StoreGlobally)
		if (JsonObjectCtx optsJs = rootJs.getObject( "multiplayer_options" ))
			opts.multOpts.deserialize( optsJs );

	if (opts.settings.gameOptsStorage == StoreGlobally)
		if (JsonObjectCtx optsJs = rootJs.getObject( "gameplay_options" ))
			opts.gameOpts.deserialize( optsJs );

	if (opts.settings.compatOptsStorage == StoreGlobally)
		if (JsonObjectCtx optsJs = rootJs.getObject( "compatibility_options" ))
			opts.compatOpts.deserialize( optsJs );

	if (opts.settings.videoOptsStorage == StoreGlobally)
		if (JsonObjectCtx optsJs = rootJs.getObject( "video_options" ))
			opts.videoOpts.deserialize( optsJs );

	if (opts.settings.audioOptsStorage == StoreGlobally)
		if (JsonObjectCtx optsJs = rootJs.getObject( "audio_options" ))
			opts.audioOpts.deserialize( optsJs );

	if (JsonObjectCtx optsJs = rootJs.getObject( "global_options" ))
		opts.globalOpts.deserialize( optsJs );

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
			deserialize( preset, presetJs, opts.settings );

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

bool deserializeAppearanceFromJsonDoc( const JsonDocumentCtx & jsonDoc, AppearanceToLoad & opts, bool loadGeometry )
{
	// report potential parsing errors via message boxes, in case the user messed up with the options file
	jsonDoc.enableErrorPopUps();

	// We use this contextual mechanism instead of standard JSON getters, because when something fails to load
	// we want to print a useful error message with information exactly which JSON element is broken.
	JsonObjectCtx rootJs = jsonDoc.getRootObject();
	if (!rootJs.isValid())
	{
		return false;
	}

	// deserialize directly from root, so that we don't have to break compatibility with older options
	opts.appearance.deserialize( rootJs, loadGeometry );
	opts.uiState.deserialize( rootJs );

	return true;
}

bool deserializeOptionsFromJsonDoc( const JsonDocumentCtx & jsonDoc, OptionsToLoad & opts )
{
	// report potential parsing errors via message boxes, in case the user messed up with the options file
	jsonDoc.enableErrorPopUps();

	// We use this contextual mechanism instead of standard JSON getters, because when something fails to load
	// we want to print a useful error message with information exactly which JSON element is broken.
	JsonObjectCtx rootJs = jsonDoc.getRootObject();
	if (!rootJs.isValid())
	{
		return false;
	}

	QString optsVersionStr = rootJs.getString( "version", {}, AllowMissing );
	opts.version = Version( optsVersionStr );

	if (!optsVersionStr.isEmpty() && opts.version > appVersion)  // empty version means pre-1.4 version
	{
		reportRuntimeError( nullptr, "Loading options from newer version",
			"Detected saved options from newer version of DoomRunner. "
			"Some settings might not be compatible. Expect errors."
		);
	}

	// backward compatibility with older options format
	if (optsVersionStr.isEmpty() || opts.version < Version{1,7})
	{
		jsonDoc.disableErrorPopUps();  // supress "missing element" warnings when loading older version

		// try to load as the 1.6.3 format, older versions will have to accept resetting some values to defaults
		deserialize_pre17( rootJs, opts );
	}
	else
	{
		if (opts.version < appVersion)
			jsonDoc.disableErrorPopUps();  // supress "missing element" warnings when loading older version

		deserialize( rootJs, opts );
	}

	return true;
}
