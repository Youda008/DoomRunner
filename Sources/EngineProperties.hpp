//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: properties and capabilities of different engines
//======================================================================================================================

#ifndef ENGINE_PROPERTIES_INCLUDED
#define ENGINE_PROPERTIES_INCLUDED


#include "Common.hpp"

#include <QVector>
#include <QString>
#include <QStringList>


//----------------------------------------------------------------------------------------------------------------------

enum class MapParamStyle
{
	Warp,  // -warp 1 8
	Map,   // +map E1M8
};

QStringList getMapArgs( MapParamStyle style, int mapIdx, const QString & mapName );

//----------------------------------------------------------------------------------------------------------------------

enum class CompatLevelStyle
{
	None,
	ZDoom,  // https://zdoom.org/wiki/CVARs:Configuration#compatmode
	Boom,   // https://doom.fandom.com/wiki/PrBoom#Compatibility_modes_and_settings
};

const QVector< QString > & getCompatLevels( CompatLevelStyle style );
QStringList getCompatLevelArgs( const QString & executableName, CompatLevelStyle style, int compatLevel );

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

EngineFamily guessEngineFamily( const QString & executableName );

//----------------------------------------------------------------------------------------------------------------------

struct EngineProperties
{
	MapParamStyle mapParamStyle;
	CompatLevelStyle compLvlStyle;
	const char * saveDirParam;
};

const EngineProperties & getEngineProperties( EngineFamily family );

//----------------------------------------------------------------------------------------------------------------------

// some engines index monitors from 1 and others from 0
int getFirstMonitorIndex( const QString & executableName );


#endif // ENGINE_PROPERTIES_INCLUDED
