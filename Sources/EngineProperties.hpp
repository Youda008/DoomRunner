//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: doom-specific utilities
//======================================================================================================================

#ifndef ENGINE_PROPERTIES_INCLUDED
#define ENGINE_PROPERTIES_INCLUDED


#include "Common.hpp"

#include <QString>


//======================================================================================================================
//  properties and capabilities of different engines

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

struct EngineProperties
{
	const char * saveDirParam;
};

EngineFamily guessEngineFamily( QString executableName );

const EngineProperties & getEngineProperties( EngineFamily family );

// some engines index monitors from 1 and others from 0
int getFirstMonitorIndex( QString executableName );


#endif // ENGINE_PROPERTIES_INCLUDED
