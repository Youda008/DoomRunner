//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: properties and capabilities of different engines
//======================================================================================================================

#ifndef ENGINE_TRAITS_INCLUDED
#define ENGINE_TRAITS_INCLUDED


#include "Common.hpp"

#include <QVector>
#include <QString>
#include <QStringList>

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

const QVector< QString > & getCompatLevels( CompatLevelStyle style );

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
EngineFamily guessEngineFamily( const QString & executableName );

//----------------------------------------------------------------------------------------------------------------------

/// traits that are shared among different engines belonging to the same family
struct EngineFamilyTraits
{
	MapParamStyle mapParamStyle;
	CompatLevelStyle compLvlStyle;
	const char * saveDirParam;
	bool hasScreenshotDirParam;
	bool hasStdoutParam;
};

/// Properties and capabilities of a particular engine that decide what command-line parameters will be used.
class EngineTraits : private EngineFamilyTraits
{
	QString executableName;

 public:

	EngineTraits( const EngineFamilyTraits & familyTraits, const QString & executableName )
		: EngineFamilyTraits( familyTraits ), executableName( executableName ) {}

	CompatLevelStyle compatLevelStyle() const   { return EngineFamilyTraits::compLvlStyle; }
	const char * saveDirParam() const           { return EngineFamilyTraits::saveDirParam; }
	bool hasScreenshotDirParam() const          { return EngineFamilyTraits::hasScreenshotDirParam; }
	bool hasStdoutParam() const                 { return EngineFamilyTraits::hasStdoutParam; }

	bool supportsCustomMapNames() const  { return mapParamStyle == MapParamStyle::Map; }

	// generates either "-warp 2 5" or "+map E2M5" depending on the engine capabilities
	QStringList getMapArgs( int mapIdx, const QString & mapName ) const;

	// generates either "-complevel x" or "+compatmode x" depending on the engine capabilities
	QStringList getCompatLevelArgs( int compatLevel ) const;

	// some engines index monitors from 1 and others from 0
	QString getCmdMonitorIndex( int ownIndex ) const;
};

EngineTraits getEngineTraits( const Engine & engine );


#endif // ENGINE_TRAITS_INCLUDED
