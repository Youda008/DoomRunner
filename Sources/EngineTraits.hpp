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
#include "Utils/OSUtils.hpp"    // SandboxInfo
#include "Utils/ExeReader.hpp"  // ExeVersionInfo

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

enum class CompatLevelStyle
{
	None,
	ZDoom,   // https://zdoom.org/wiki/CVARs:Configuration#compatmode
	PrBoom,  // https://doom.fandom.com/wiki/PrBoom#Compatibility_modes_and_settings
};

const QStringList & getCompatLevels( CompatLevelStyle style );

//----------------------------------------------------------------------------------------------------------------------

// https://upload.wikimedia.org/wikipedia/commons/a/a8/Doom-ports.svg
enum class EngineFamily
{
	ZDoom,
	PrBoom,
	ChocolateDoom,

	_EnumEnd  ///< indicates an error
};
const char * familyToStr( EngineFamily family );
EngineFamily familyFromStr( const QString & familyStr );

// EngineFamily is user-overridable in EngineDialog, but this is our default automatic detection
EngineFamily guessEngineFamily( const QString & appNameNormalized );

//----------------------------------------------------------------------------------------------------------------------

/// Traits that are shared among different engines belonging to the same family.
struct EngineFamilyTraits
{
	MapParamStyle mapParamStyle;
	CompatLevelStyle compLvlStyle;
	const char * saveDirParam;
	bool hasScreenshotDirParam;
	bool needsStdoutParam;
};

/// Properties and capabilities of a particular engine that decide what command-line parameters will be used.
class EngineTraits
{
	// application info
	QString _exePath;             ///< path of the file from which the application info was constructed
	QString _exeBaseName;         ///< executable file name without file suffix
	os::ExeVersionInfo _exeVersionInfo;
	QString _appNameNormalized;   ///< application name normalized for indexing engine property tables
	// family traits
	const EngineFamilyTraits * _familyTraits;

	static const Version emptyVersion;

 public:

	// initialization

	EngineTraits();

	/// Initializes application info.
	/** This may open and read the executable file if needed. */
	void loadAppInfo( const QString & executablePath );
	bool hasAppInfo() const                     { return !_exePath.isEmpty(); }

	/// Initializes family traits according to specified engine family.
	void assignFamilyTraits( EngineFamily family );
	bool hasFamilyTraits() const                { return _familyTraits != nullptr; }

	// application properties - requires application info to be loaded

	const QString & appInfoSrcExePath() const   { assert( hasAppInfo() ); return _exePath; }
	const QString & exeBaseName() const         { assert( hasAppInfo() ); return _exeBaseName; }

	const QString & exeAppName() const          { assert( hasAppInfo() ); return _exeVersionInfo.appName; }
	const QString & exeDescription() const      { assert( hasAppInfo() ); return _exeVersionInfo.description; }
	const Version & exeVersion() const          { assert( hasAppInfo() ); return _exeVersionInfo.version; }

	const QString & appNameNormalized() const   { assert( hasAppInfo() ); return _appNameNormalized; }

	// command line parameters deduction - requires application info and family traits to be initialized

	CompatLevelStyle compatLevelStyle() const   { assert( hasFamilyTraits() ); return _familyTraits->compLvlStyle; }
	const char * saveDirParam() const           { assert( hasFamilyTraits() ); return _familyTraits->saveDirParam; }
	bool hasScreenshotDirParam() const          { assert( hasFamilyTraits() ); return _familyTraits->hasScreenshotDirParam; }
	bool needsStdoutParam() const               { assert( hasFamilyTraits() ); return _familyTraits->needsStdoutParam; }
	bool supportsCustomMapNames() const         { assert( hasFamilyTraits() ); return _familyTraits->mapParamStyle == MapParamStyle::Map; }

	// generates either "-warp 2 5" or "+map E2M5" depending on the engine capabilities
	QStringVec getMapArgs( int mapIdx, const QString & mapName ) const;

	// generates either "-complevel x" or "+compatmode x" depending on the engine capabilities
	QStringVec getCompatLevelArgs( int compatLevel ) const;

	// some engines index monitors from 1 and others from 0
	QString getCmdMonitorIndex( int ownIndex ) const;
};


#endif // ENGINE_TRAITS_INCLUDED
