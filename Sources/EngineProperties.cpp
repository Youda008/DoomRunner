//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: properties and capabilities of different engines
//======================================================================================================================

#include "EngineProperties.hpp"

#include "LangUtils.hpp"  // find

#include <QHash>
#include <QRegularExpression>


//======================================================================================================================
//  engine definitions - add support for new engines here

static const char * const engineFamilyStrings [] =
{
	"ZDoom",
	"Boom",
	"ChocolateDoom",
};
static_assert( std::size(engineFamilyStrings) == size_t(EngineFamily::_EnumEnd), "Please update this table too" );

static const QHash< QString, EngineFamily > knownEngineFamilies =
{
	// the key is an executable name in lower case without the .exe suffix
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
	{ "mbf",             EngineFamily::Boom },
	{ "smmu",            EngineFamily::Boom },
	{ "eternity",        EngineFamily::Boom },
	{ "dsda-doom",       EngineFamily::Boom },
	{ "woof",            EngineFamily::Boom },
	{ "chocolate-doom",  EngineFamily::ChocolateDoom },
	{ "crispy-doom",     EngineFamily::ChocolateDoom },
	{ "doomretro",       EngineFamily::ChocolateDoom },
};

static const EngineProperties engineFamilyProperties [] =
{
	/*ZDoom*/     { MapParamStyle::Map,  CompatLevelStyle::ZDoom, "-savedir" },
	/*Boom*/      { MapParamStyle::Warp, CompatLevelStyle::Boom,  "-save" },
	/*Chocolate*/ { MapParamStyle::Warp, CompatLevelStyle::None,  "-savedir" },
};
static_assert( std::size(engineFamilyProperties) == std::size(engineFamilyStrings), "Please update this table too" );

static const QVector<QString> zdoomCompatLevels =
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

static const QVector<QString> boomCompatLevels =
{
	"0  - Doom v1.2",     // (note: flawed; use PrBoom+ 2.5.0.8 or higher instead if this complevel is desired)
	"1  - Doom v1.666",
	"2  - Doom v1.9",
	"3  - Ultimate Doom",
	"4  - Final Doom & Doom95",
	"5  - DOSDoom",
	"6  - TASDOOM",
	"7  - Boom's inaccurate vanilla",
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

static const QVector<QString> noCompatLevels = {};

static const QHash< QString, int > startingMonitorIndexes =
{
	{ "zdoom", 1 },
};


//======================================================================================================================
//  code

//----------------------------------------------------------------------------------------------------------------------
//  MapParamStyle

QStringList getMapArgs( MapParamStyle style, int mapIdx, const QString & mapName )
{
	if (mapName.isEmpty())
	{
		return {};
	}

	if (style == MapParamStyle::Map)  // this engine supports +map, we can use the map name directly
	{
		return { "+map", mapName };
	}
	else  // this engine only supports the old -warp, we must deduce map number
	{
		static QRegularExpression doom1MapNameRegex("E(\\d+)M(\\d+)");
		static QRegularExpression doom2MapNameRegex("MAP(\\d+)");
		QRegularExpressionMatch match;
		if ((match = doom1MapNameRegex.match( mapName )).hasMatch())
		{
			return { "-warp", match.captured(1), match.captured(2) };
		}
		else if ((match = doom2MapNameRegex.match( mapName )).hasMatch())
		{
			return { "-warp", match.captured(1) };
		}
		else  // in case the WAD defines it's own map names, we have to resort to guessing the number by using its combo-box index
		{
			return { "-warp", QString::number( mapIdx + 1 ) };
		}
	}
}

//----------------------------------------------------------------------------------------------------------------------
//  CompatLevelStyle

const QVector<QString> & getCompatLevels( CompatLevelStyle style )
{
	if (style == CompatLevelStyle::ZDoom)
		return zdoomCompatLevels;
	else if (style == CompatLevelStyle::Boom)
		return boomCompatLevels;
	else
		return noCompatLevels;
}

QStringList getCompatLevelArgs( const QString & executableName, CompatLevelStyle style, int compatLevel )
{
	// Properly working -compatmode is present only in GZDoom,
	// for other ZDoom-based engines use at least something, even if it doesn't fully work.
	if (executableName.toLower() == "gzdoom")
		return { "-compatmode", QString::number( compatLevel ) };
	if (style == CompatLevelStyle::ZDoom)
		return { "+compatmode", QString::number( compatLevel ) };
	else if (style == CompatLevelStyle::Boom)
		return { "-complevel", QString::number( compatLevel ) };
	else
		return {};
}


//----------------------------------------------------------------------------------------------------------------------
//  EngineFamily

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

EngineFamily guessEngineFamily( const QString & executableName )
{
	auto iter = knownEngineFamilies.find( executableName.toLower() );
	if (iter != knownEngineFamilies.end())
		return iter.value();
	else
		return EngineFamily::ZDoom;
}

//----------------------------------------------------------------------------------------------------------------------
//  EngineProperties

const EngineProperties & getEngineProperties( EngineFamily family )
{
	if (size_t(family) < std::size(engineFamilyProperties))
		return engineFamilyProperties[ size_t(family) ];
	else
		return engineFamilyProperties[ 0 ];
}

//----------------------------------------------------------------------------------------------------------------------
//  miscellaneous

int getFirstMonitorIndex( const QString & executableName )
{
	auto iter = startingMonitorIndexes.find( executableName.toLower() );
	if (iter != startingMonitorIndexes.end())
		return iter.value();
	else
		return 0;
}
