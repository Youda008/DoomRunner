//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: serialization/deserialization of the user data into/from a file
//======================================================================================================================

#ifndef OPTIONS_INCLUDED
#define OPTIONS_INCLUDED


#include "Essential.hpp"

#include "CommonTypes.hpp"  // PtrList
#include "UserData.hpp"
#include "Version.hpp"
#include "Utils/JsonUtils.hpp"  // JsonDocumentCtx

#include <QList>
#include <QString>


//----------------------------------------------------------------------------------------------------------------------

// Values used to recognize items that haven't been parsed successfully and should be left out.
extern const QString InvalidItemName;
extern const QString InvalidItemPath;


//----------------------------------------------------------------------------------------------------------------------

struct OptionsToSave
{
	// files
	const PtrList< EngineInfo > & engines;  // we must accept EngineInfo, but we will serialize only Engine fields
	const PtrList< IWAD > & iwads;

	// options
	const LaunchOptions & launchOpts;
	const MultiplayerOptions & multOpts;
	const GameplayOptions & gameOpts;
	const CompatibilityOptions & compatOpts;
	const VideoOptions & videoOpts;
	const AudioOptions & audioOpts;
	const GlobalOptions & globalOpts;

	// presets
	const PtrList< Preset > & presets;
	int selectedPresetIdx;

	// global settings
	const EngineSettings & engineSettings;
	const IwadSettings & iwadSettings;
	const MapSettings & mapSettings;
	const ModSettings & modSettings;
	const LauncherSettings & settings;

	const AppearanceSettings & appearance;
};

QJsonDocument serializeOptionsToJsonDoc( const OptionsToSave & opts );


//----------------------------------------------------------------------------------------------------------------------
// For technical reasons explained at MainWindow initialization we have to separate loading
// of the appearance settings (visual style, color scheme) and window geometry (position and size)
// from loading the rest of the options.

struct OptionsToLoad
{
	Version version;  ///< version of the options format that was loaded

	// files
	PtrList< EngineInfo > engines;  // we must accept EngineInfo, but we will load only Engine fields
	PtrList< IWAD > iwads;

	// options
	LaunchOptions & launchOpts;
	MultiplayerOptions & multOpts;
	GameplayOptions & gameOpts;
	CompatibilityOptions & compatOpts;
	VideoOptions & videoOpts;
	AudioOptions & audioOpts;
	GlobalOptions & globalOpts;

	// presets
	PtrList< Preset > presets;
	QString selectedPreset;

	// global settings
	EngineSettings & engineSettings;
	IwadSettings & iwadSettings;
	MapSettings & mapSettings;
	ModSettings & modSettings;
	LauncherSettings & settings;
};

struct AppearanceToLoad
{
	AppearanceSettings & appearance;
};

void deserializeOptionsFromJsonDoc( const JsonDocumentCtx & jsonDoc, OptionsToLoad & opts );
void deserializeAppearanceFromJsonDoc( const JsonDocumentCtx & jsonDoc, AppearanceToLoad & opts, bool loadGeometry );


#endif // OPTIONS_INCLUDED
