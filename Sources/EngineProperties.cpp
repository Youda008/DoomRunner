//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: doom-specific utilities
//======================================================================================================================

#include "EngineProperties.hpp"

#include "LangUtils.hpp"  // find

#include <QHash>


//======================================================================================================================
//  properties and capabilities of different engines

static const char * const engineFamilyStrings [] =
{
	"ZDoom",
	"Boom",
	"ChocolateDoom",
};
static_assert( std::size(engineFamilyStrings) == size_t(EngineFamily::_EnumEnd), "Please update this table too" );

static const EngineProperties engineFamilyProperties [] =
{
	/*ZDoom*/         { "-savedir" },
	/*Boom*/          { "-save" },
	/*ChocolateDoom*/ { "-savedir" },
};
static_assert( std::size(engineFamilyProperties) == std::size(engineFamilyStrings), "Please update this table too" );

static const QHash< QString, EngineFamily > knownEngineFamilies =
{
	{ "zdoom",           EngineFamily::ZDoom },
	{ "lzdoom",          EngineFamily::ZDoom },
	{ "gzdoom",          EngineFamily::ZDoom },
	{ "qzdoom",          EngineFamily::ZDoom },
	{ "skulltag",        EngineFamily::ZDoom },
	{ "zandronum",       EngineFamily::ZDoom },
	{ "boom",            EngineFamily::Boom },
	{ "prboom",          EngineFamily::Boom },
	{ "prboom-plus",     EngineFamily::Boom },
	{ "glboom",          EngineFamily::Boom },
	{ "eternity",        EngineFamily::Boom },
	{ "dsda-doom",       EngineFamily::Boom },
	{ "chocolate-doom",  EngineFamily::ChocolateDoom },
	{ "crispy-doom",     EngineFamily::ChocolateDoom },
	{ "doomretro",       EngineFamily::ChocolateDoom },
};

static const QHash< QString, int > startingMonitorIndexes =
{
	{ "zdoom", 1 },
};

//----------------------------------------------------------------------------------------------------------------------

const char * familyToStr( EngineFamily family )
{
	if (size_t(family) < std::size(engineFamilyStrings))
		return engineFamilyStrings[ size_t(family) ];
	else
		return "<invalid>";
}

EngineFamily familyFromStr( const QString & familyStr )
{
	int idx = find( engineFamilyStrings, familyStr );
	if (idx >= 0)
		return EngineFamily( idx );
	else
		return EngineFamily::_EnumEnd;
}

EngineFamily guessEngineFamily( QString executableName )
{
	executableName = executableName.toLower();
	auto iter = knownEngineFamilies.find( executableName );
	if (iter != knownEngineFamilies.end())
		return iter.value();
	else
		return EngineFamily::ZDoom;
}

const EngineProperties & getEngineProperties( EngineFamily family )
{
	if (size_t(family) < std::size(engineFamilyProperties))
		return engineFamilyProperties[ size_t(family) ];
	else
		return engineFamilyProperties[ 0 ];
}

int getFirstMonitorIndex( QString executableName )
{
	executableName = executableName.toLower();
	auto iter = startingMonitorIndexes.find( executableName );
	if (iter != startingMonitorIndexes.end())
		return iter.value();
	else
		return 0;
}
