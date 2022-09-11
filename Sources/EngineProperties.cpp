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
	/*ZDoom*/         { "-savedir", CompatLevelStyle::ZDoom },
	/*Boom*/          { "-save",    CompatLevelStyle::Boom },
	/*ChocolateDoom*/ { "-savedir", CompatLevelStyle::None },
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

const QVector<QString> zdoomCompatLevels =
{
	"0 - Default",        // All compatibility options are turned off.
	"1 - Doom",           // Enables a set of options that should allow nearly all maps made for vanilla Doom to work in ZDoom:
	                      //   crossdropoff, dehhealth, light, missileclip, nodoorlight, shorttex, soundtarget, spritesort, stairs, trace, useblocking, floormove, maskedmidtex
	"2 - Doom (Strict)",  // Sets all of the above options and also sets these:
	                      //   corpsegibs, hitscan, invisibility, limitpain, nopassover, notossdrop, wallrun
	"3 - Boom",           // Allows maps made specifically for Boom to function correctly by enabling the following options:
	                      //   boomscroll, missileclip, soundtarget, trace, maskedmidtex
	"4 - ZDoom 2.0.63",   // Sets the two following options to be true, restoring the behavior of version 2.0.63:
	                      //   light, soundtarget
	"5 - MBF",            // As Boom above, but also sets these for closer imitation of MBF behavior:
	                      //   mushroom, mbfmonstermove, noblockfriends, maskedmidtex
	"6 - Boom (Strict)",  // As Boom above, but also sets these:
	                      //   corpsegibs, hitscan, invisibility, nopassover, notossdrop, wallrun, maskedmidtex
};

const QVector<QString> boomCompatLevels =
{
	"0  - Doom v1.2",     // (note: flawed; use PrBoom+ 2.5.0.8 or higher instead if this complevel is desired)
	"1  - Doom v1.666",
	"2  - Doom v1.9",
	"3  - Ultimate Doom",
	"4  - Final Doom & Doom95",
	"5  - DOSDoom",
	"6  - TASDOOM",
	"7  - Boom's inaccurate vanilla compatibility mode",
	"8  - Boom v2.01",
	"9  - Boom v2.02",
	"10 - LxDoom",
	"11 - MBF",
	"12 - PrBoom (older version)",
	"13 - PrBoom (older version)",
	"14 - PrBoom (older version)",
	"15 - PrBoom (older version)",
	"16 - PrBoom (older version)",
	"17 - PrBoom (current)",
	"18 - unused",
	"19 - unused",
	"20 - unused",
	"21 - MBF21",
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
