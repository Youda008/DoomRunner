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

enum class CompatLevelStyle
{
	None,
	ZDoom,  // https://zdoom.org/wiki/CVARs:Configuration#compatmode
	Boom,   // https://doom.fandom.com/wiki/PrBoom#Compatibility_modes_and_settings
};

const QVector<QString> & getCompatLevels( CompatLevelStyle style );
QStringList getCompatLevelArgs( CompatLevelStyle style, int compatLevel );

//----------------------------------------------------------------------------------------------------------------------

// https://upload.wikimedia.org/wikipedia/commons/a/a8/Doom-ports.svg
enum class EngineFamily
{
	ZDoom,
	Boom,
	ChocolateDoom,

	_EnumEnd  ///< indicates an error
};
const char * familyToStr( EngineFamily family );
EngineFamily familyFromStr( const QString & familyStr );

EngineFamily guessEngineFamily( QString executableName );

//----------------------------------------------------------------------------------------------------------------------

struct EngineProperties
{
	const char * saveDirParam;
	CompatLevelStyle compLvlStyle;
};

const EngineProperties & getEngineProperties( EngineFamily family );

//----------------------------------------------------------------------------------------------------------------------

// some engines index monitors from 1 and others from 0
int getFirstMonitorIndex( QString executableName );


#endif // ENGINE_PROPERTIES_INCLUDED
