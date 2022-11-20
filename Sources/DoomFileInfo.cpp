//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: Doom file type recognition and known WAD detection
//======================================================================================================================

#include "DoomFileInfo.hpp"

#include "LangUtils.hpp"
#include "FileSystemUtils.hpp"

#include <QVector>  // TODO: cleanup
#include <QHash>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

#include <QDebug>


//======================================================================================================================
//  file type recognition

const QVector< QString > configFileSuffixes = {"ini", "cfg"};
const QString saveFileSuffix = "zds";
const QString demoFileSuffix = "lmp";

const QVector< QString > iwadSuffixes = {"wad", "iwad", "pk3", "ipk3", "pk7", "ipk7", "pkz", "pke"};
const QVector< QString > pwadSuffixes = {"wad", "pwad", "pk3", "pk7", "pkz", "pke", "zip", "7z", "deh", "bex"};
const QVector< QString > dukeSuffixes = {"grp", "rff"};

// The correct way would be to recognize the type by file header, but there are incorrectly made mods
// that present themselfs as IWADs, so in order to support those we need to use the file suffix
bool isIWAD( const QFileInfo & file )
{
	return (iwadSuffixes.contains( file.suffix().toLower() ))
	     || dukeSuffixes.contains( file.suffix().toLower() );  // i did not want this, but the guy was insisting on it
}

bool isMapPack( const QFileInfo & file )
{
	return (pwadSuffixes.contains( file.suffix().toLower() ))
	     || dukeSuffixes.contains( file.suffix().toLower() );  // i did not want this, but the guy was insisting on it
}

QStringList getModFileSuffixes()
{
	QStringList suffixes;
	for (const QString & suffix : pwadSuffixes)
		suffixes.append( "*."+suffix );
	for (const QString & suffix : dukeSuffixes)
		suffixes.append( "*."+suffix );
	return suffixes;
}


//======================================================================================================================
//  known WAD info

QStringList getStandardMapNames( const QString & iwadFileName )
{
	QStringList mapNames;

	QString iwadFileNameLower = iwadFileName.toLower();

	if (iwadFileNameLower == "doom.wad" || iwadFileNameLower == "doom1.wad")
	{
		for (int e = 1; e <= 4; e++)
			for (int m = 1; m <= 9; m++)
				mapNames.push_back( QStringLiteral("E%1M%2").arg(e).arg(m) );
	}
	else
	{
		for (int i = 1; i <= 32; i++)
			mapNames.push_back( QStringLiteral("MAP%1").arg( i, 2, 10, QChar('0') ) );
	}

	return mapNames;
}

//----------------------------------------------------------------------------------------------------------------------
//  starting maps

// fast lookup table that can be used for WADs whose name can be matched exactly
static const QHash< QString, QString > startingMapsLookup =
{
	// MasterLevels
	{ "virgil.wad",    "MAP03" },
	{ "minos.wad",     "MAP05" },
	{ "bloodsea.wad",  "MAP07" },
	{ "mephisto.wad",  "MAP07" },
	{ "nessus.wad",    "MAP07" },
	{ "geryon.wad",    "MAP08" },
	{ "vesperas.wad",  "MAP09" },
	{ "blacktwr.wad",  "MAP25" },
	{ "teeth.wad",     "MAP31" },

	// unofficial MasterLevels
	{ "dante25.wad",   "MAP02" },
	{ "derelict.wad",  "MAP02" },
	{ "achron22.wad",  "MAP03" },
	{ "flood.wad",     "MAP03" },
	{ "twm01.wad",     "MAP03" },
	{ "watchtwr.wad",  "MAP04" },
	{ "todeath.wad",   "MAP05" },
	{ "arena.wad",     "MAP06" },
	{ "storm.wad",     "MAP09" },
	{ "the_evil.wad",  "MAP30" },

	// Also include the MasterLevels that start from MAP01, because otherwise when user switches from non-MAP01 level
	// to MAP01 level, the launcher will retain its previous values, which will be incorrect.
	{ "attack.wad",    "MAP01" },
	{ "canyon.wad",    "MAP01" },
	{ "catwalk.wad",   "MAP01" },
	{ "combine.wad",   "MAP01" },
	{ "fistula.wad",   "MAP01" },
	{ "garrison.wad",  "MAP01" },
	{ "manor.wad",     "MAP01" },
	{ "paradox.wad",   "MAP01" },
	{ "subspace.wad",  "MAP01" },
	{ "subterra.wad",  "MAP01" },
	{ "ttrap.wad",     "MAP01" },

	// unofficial MasterLevels starting from MAP01
	{ "anomaly.wad",   "MAP01" },
	{ "cdk_fury.wad",  "MAP01" },
	{ "cpu.wad",       "MAP01" },
	{ "device_1.wad",  "MAP01" },
	{ "dmz.wad",       "MAP01" },
	{ "e_inside.wad",  "MAP01" },
	{ "farside.wad",   "MAP01" },
	{ "hive.wad",      "MAP01" },
	{ "mines.wad",     "MAP01" },
	{ "trouble.wad",   "MAP01" },
};

// slow regex search for WADs whose name follows specfic format, for example those with postfixed version number
static const QPair< QRegularExpression, QString > startingMapsRegexes [] =
{
	{ QRegularExpression("sigil[^.]*\\.wad"), "E5M1" },  // SIGIL_v1_21.wad
};

QString getStartingMap( const QString & wadFileName )
{
	QString wadFileNameLower = wadFileName.toLower();

	// first do a fast search if the file name can be matched directly
	auto iter = startingMapsLookup.find( wadFileNameLower );
	if (iter != startingMapsLookup.end())
		return iter.value();

	// if not found, do a slow search if it's in one of known formats
	for (const auto & regexPair : startingMapsRegexes)
		if (regexPair.first.match( wadFileNameLower ).hasMatch())
			return regexPair.second;

	return {};
}
