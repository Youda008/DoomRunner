//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: doom-specific utilities
//======================================================================================================================

#include "DoomUtils.hpp"

#include "LangUtils.hpp"

#include <QVector>
#include <QHash>
#include <QString>
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
//  properties and capabilities of different engines

static const EngineProperties defaultEngineProperties =
	{ "-savedir", 0 };

static const QHash< QString, EngineProperties > engineProperties =
{
	{ "zdoom",          { "-savedir", 1 } },
	{ "lzdoom",         { "-savedir", 0 } },
	{ "gzdoom",         { "-savedir", 0 } },
	{ "qzdoom",         { "-savedir", 0 } },
	{ "zandronum",      { "-savedir", 0 } },
	{ "boom",           { "-save", 0 } },
	{ "prboom",         { "-save", 0 } },
	{ "glboom",         { "-save", 0 } },
	{ "doomretro",      { "-save", 0 } },
	{ "eternity",       { "-save", 0 } },
};

const EngineProperties & getEngineProperties( const QString & enginePath )
{
	QString executableName = QFileInfo( enginePath ).baseName();

	auto iter = engineProperties.find( executableName );
	if (iter != engineProperties.end())
		return *iter;
	else
		return defaultEngineProperties;
}


//======================================================================================================================
//  WAD info loading

// https://doomwiki.org/wiki/WAD

/// section that every WAD file begins with
struct WadHeader
{
	char wadType [4];  ///< either "IWAD" or "PWAD" but the string is NOT null terminated
	uint32_t numLumps;  ///< number of entries in the lump directory
	uint32_t lumpDirOffset;  ///< offset of the lump directory in the file
};

/// one entry of the lump directory
struct LumpEntry
{
	uint32_t dataOffset;
	uint32_t size;
	char name [8];  ///< might not be null-terminated when the string takes all 8 bytes
};

static bool isMapMarker( const LumpEntry & lump, const QString & lumpName )
{
	return lump.size == 0
		&& !lumpName.contains("START") && !lumpName.contains("END")
		&& !lumpName.contains("_S") && !lumpName.contains("_E");
}

static void getMapNamesFromMAPINFO( const QByteArray & lumpData, QVector< QString > & mapNames )
{
	QTextStream lumpText( lumpData, QIODevice::ReadOnly );

	QString line;
	while (lumpText.readLineInto( &line ))
	{
		static QRegularExpression mapDefRegex("map\\s+(\\w+)\\s+\"([^\"]+)\"");
		QRegularExpressionMatch match = mapDefRegex.match( line );
		if (match.hasMatch())
		{
			mapNames.append( match.captured(1) );
		}
	}
}

static WadInfo readWadInfoFromFile( const QString & filePath )
{
	WadInfo wadInfo;
	wadInfo.successfullyRead = false;

	QFile file( filePath );
	if (!file.open( QIODevice::ReadOnly ))
	{
		return wadInfo;
	}

	WadHeader header;
	if (file.read( (char*)&header, sizeof(header) ) < qint64( sizeof(header) ))
	{
		return wadInfo;
	}

	if (strncmp( header.wadType, "IWAD", sizeof(header.wadType) ) == 0)
		wadInfo.type = WadType::IWAD;
	else if (strncmp( header.wadType, "PWAD", sizeof(header.wadType) ) == 0)
		wadInfo.type = WadType::PWAD;
	else
		wadInfo.type = WadType::NEITHER;

	if (!file.seek( header.lumpDirOffset ))
	{
		return wadInfo;
	}

	// the lump directory is basically an array of LumpEntry structs, so let's read it all at once
	qint64 lumpDirSize = header.numLumps * sizeof(LumpEntry);
	std::unique_ptr< LumpEntry [] > lumpDir( new LumpEntry [header.numLumps] );
	if (file.read( (char*)lumpDir.get(), lumpDirSize ) < lumpDirSize)
	{
		return wadInfo;
	}

	for (uint32_t i = 0; i < header.numLumps; ++i)
	{
		LumpEntry & lump = lumpDir[i];

		// we need to make sure we have a null-terminated string, because the original one isn't when it's 8 chars long
		char lumpName0 [9];
		strncpy( lumpName0, lump.name, sizeof(lump.name) );  // no, g++, this code is correct, lump.name is [8], sizeof(lumpName0) would make it read out of bounds
		lumpName0[8] = '\0';
		QString lumpName( lumpName0 );

		// try to gather the map names from the marker lumps,
		// but if we find a MAPINFO lump, let that one override the markers

		if (isMapMarker( lump, lumpName ))
		{
			wadInfo.mapNames.append( lumpName );
		}

		if (lumpName == "MAPINFO")
		{
			qint64 origPos = file.pos();

			if (!file.seek( lump.dataOffset ))
			{
				continue;
			}

			QByteArray lumpData = file.read( lump.size );
			if (lumpData.size() < int( lump.size ))
			{
				file.seek( origPos );
				continue;
			}

			wadInfo.mapNames.clear();
			getMapNamesFromMAPINFO( lumpData, wadInfo.mapNames );

			break;
		}
	}

	wadInfo.successfullyRead = true;
	return wadInfo;
}



const WadInfo & getCachedWadInfo( const QString & filePath )
{
  // Opening and reading from a file is expensive, so we cache the results here so that subsequent calls are fast.
  // The cache is global for the whole process because why not.
  static QHash< QString, WadInfo > g_cachedWadTypes;

	auto pos = g_cachedWadTypes.find( filePath );
	if (pos == g_cachedWadTypes.end())
	{
		WadInfo wadInfo = readWadInfoFromFile( filePath );
		pos = g_cachedWadTypes.insert( filePath, std::move(wadInfo) );
		if (!pos->successfullyRead)
		{
			qWarning() << "failed to read from " << filePath;
		}
	}
	return pos.value();
}
