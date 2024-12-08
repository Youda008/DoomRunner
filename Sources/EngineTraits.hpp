//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: properties and capabilities of different engines
//======================================================================================================================

#ifndef ENGINE_TRAITS_INCLUDED
#define ENGINE_TRAITS_INCLUDED


#include "Essential.hpp"

#include "CommonTypes.hpp"
#include "Utils/OSUtils.hpp"  // AppInfo

#include <QString>
#include <QStringList>

#include <cassert>

struct Engine;


//----------------------------------------------------------------------------------------------------------------------

enum class MapParamStyle
{
	Warp,  // -warp 1 8
	Map,   // +map E1M8
};

//----------------------------------------------------------------------------------------------------------------------

enum class CompatModeStyle
{
	None,
	ZDoom,   // +compatmode  https://zdoom.org/wiki/CVARs:Configuration#compatmode
	PrBoom,  // -complevel   https://doom.fandom.com/wiki/PrBoom#Compatibility_modes_and_settings
};

const QStringList & getCompatModes( CompatModeStyle style );

//----------------------------------------------------------------------------------------------------------------------

// https://upload.wikimedia.org/wikipedia/commons/a/a8/Doom-ports.svg
enum class EngineFamily
{
	ZDoom,
	PrBoom,
	MBF,  // Marine's Best Friend
	ChocolateDoom,
	EDGE,

	_EnumEnd  ///< indicates an error
};
const char * familyToStr( EngineFamily family );
EngineFamily familyFromStr( const QString & familyStr );

//----------------------------------------------------------------------------------------------------------------------

/// Traits that are shared among different engines belonging to the same family.
struct EngineFamilyTraits
{
	const char * configFileSuffix;  ///< which file name suffix the engine uses for its save files
	const char * saveFileSuffix;    ///< which file name suffix the engine uses for its save files
	const char * saveDirParam;      ///< which command line parameter is used for overriding the save directory
	MapParamStyle mapParamStyle;    ///< which command line parameter is used for choosing the starting map
	CompatModeStyle compModeStyle;  ///< which command line parameter is used for choosing the compatibility mode
	bool hasScreenshotDirParam;     ///< whether the screenshot directory override parameter +screenshot_dir is supported
	bool needsStdoutParam;          ///< whether the engine needs -stdout option to send its output to stdout where it can be read by this launcher
};

/// Properties and capabilities of a particular engine that decide what command-line parameters will be used.
class EngineTraits {

	std::optional< os::AppInfo > _appInfo;
	EngineFamily _family = EngineFamily::_EnumEnd;
	const EngineFamilyTraits * _familyTraits = nullptr;
	QString _configFileName;
	QString _commonSaveSubdir;  ///< pre-calculated common part of the save sub-directory

 public:

	// initialization

	/// Attempts to auto-detect engine traits from a given executable.
	/** This may open and read the executable file, which may be a time-expensive operation. */
	void autoDetectTraits( const QString & executablePath );

	/// Call this in case the family needs to be changed after the auto-detection.
	void setFamilyTraits( EngineFamily family );

	bool isInitialized() const            { return _appInfo.has_value(); }
	bool isCorrectlyInitialized() const   { return _appInfo && _familyTraits; }

	// application properties - requires application info to be loaded

	const auto & exePath() const                { assert( _appInfo ); return _appInfo->exePath; }
	const auto & exeBaseName() const            { assert( _appInfo ); return _appInfo->exeBaseName; }

	const auto & sandboxType() const            { assert( _appInfo ); return _appInfo->sandboxEnv.type; }
	const auto & sandboxAppName() const         { assert( _appInfo ); return _appInfo->sandboxEnv.appName; }
	const auto & sandboxHomeDir() const         { assert( _appInfo ); return _appInfo->sandboxEnv.homeDir; }

	const auto & exeAppName() const             { assert( _appInfo ); return _appInfo->versionInfo.appName; }
	const auto & exeDescription() const         { assert( _appInfo ); return _appInfo->versionInfo.description; }
	const auto & exeVersion() const             { assert( _appInfo ); return _appInfo->versionInfo.version; }

	const auto & displayName() const            { assert( _appInfo ); return _appInfo->displayName; }
	const auto & normalizedName() const         { assert( _appInfo ); return _appInfo->normalizedName; }

	// default directories and path requirements - requires application info to be loaded

	// all of these paths are absolute
	QString getDefaultConfigDir() const;
	QString getDefaultDataDir() const;
	QString getDefaultScreenshotDir() const;

	/// Whether the save directory depends on the IWAD in use.
	/** If true, the path of the selected IWAD must be supplied to the getSaveSubdir(). */
	bool saveDirDependsOnIWAD() const;

	/// Returns a relative sub-directory inside a data directory dedicated for save files.
	QString getDefaultSaveSubdir( const QString & IWADPath = {} ) const;

	enum class SaveBaseDir
	{
		WorkingDir,  ///< path of save file must be relative to the current working directory
		SaveDir,     ///< path of save file must be relative to the -savedir argument if present or the default save dir otherwise
	};
	SaveBaseDir baseDirStyleForSaveFiles() const;

	// default data files names and file suffixes

	const QString & defaultConfigFileName() const  { assert( !_configFileName.isEmpty() ); return _configFileName; }

	const char * configFileSuffix() const       { assert( _familyTraits ); return _familyTraits->configFileSuffix; }
	const char * saveFileSuffix() const         { assert( _familyTraits ); return _familyTraits->saveFileSuffix; }

	// command line parameters deduction - requires application info and family traits to be initialized

	const char * saveDirParam() const           { assert( _familyTraits ); return _familyTraits->saveDirParam; }
	bool hasScreenshotDirParam() const          { assert( _familyTraits ); return _familyTraits->hasScreenshotDirParam; }

	bool supportsCustomMapNames() const         { assert( _familyTraits ); return _familyTraits->mapParamStyle == MapParamStyle::Map; }
	CompatModeStyle compatModeStyle() const     { assert( _familyTraits ); return _familyTraits->compModeStyle; }

	bool needsStdoutParam() const               { assert( _familyTraits ); return _familyTraits->needsStdoutParam; }

	// generates either "-warp 2 5" or "+map E2M5" depending on the engine capabilities
	QStringVec getMapArgs( int mapIdx, const QString & mapName ) const;

	// generates either "-complevel x" or "+compatmode x" depending on the engine capabilities
	QStringVec getCompatModeArgs( int compatMode ) const;

	// some engines index monitors from 1 and others from 0
	QString getCmdMonitorIndex( int ownIndex ) const;

	// miscellaneous

	EngineFamily currentEngineFamily() const    { assert( _family != EngineFamily::_EnumEnd ); return _family; }

 private:

	EngineFamily guessEngineFamily() const;

	Version getExeVersionOrAssumeLatest() const;

	bool isBasedOnGZDoomVersionOrLater( Version atLeastVersion ) const;
	bool isPortableZDoom() const;

	QString getCommonSaveSubdir() const;
	QString getDefaultConfigFileName() const;
};


#endif // ENGINE_TRAITS_INCLUDED
