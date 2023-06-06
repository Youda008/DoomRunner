//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: WAD file parsing and information extraction
//======================================================================================================================

#include "WADReader.hpp"

#include "LangUtils.hpp"
#include "FileSystemUtils.hpp"

#include <QHash>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>

#include <cctype>


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

static bool isPrintableAsciiString( const QString & str )
{
	for (auto c : str.toLatin1())
		if (!isprint( c ))
			return false;
	return true;
}

static bool isMapMarker( const LumpEntry & lump, const QString & lumpName )
{
	static QSet< QString > blacklistedNames =
	{
		"SEGS",
		"SECTORS",
		"SSECTORS",
		"LINEDEFS",
		"SIDEDEFS",
		"VERTEXES",
		"NODES",
		"BLOCKMAP",
		"REJECT",
	};
	return lump.size == 0
		&& !lumpName.endsWith("_START") && !lumpName.endsWith("_END")
		&& !lumpName.endsWith("_S") && !lumpName.endsWith("_E")
		&& !blacklistedNames.contains( lumpName );
}

static void getMapNamesFromMAPINFO( const QByteArray & lumpData, QStringList & mapNames )
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

	QFile file( filePath );
	if (!file.open( QIODevice::ReadOnly ))
	{
		wadInfo.status = ReadStatus::FailedToRead;
		return wadInfo;
	}

	WadHeader header;
	if (file.read( (char*)&header, sizeof(header) ) < qint64( sizeof(header) ))
	{
		wadInfo.status = ReadStatus::FailedToRead;
		return wadInfo;
	}

	if (strncmp( header.wadType, "IWAD", sizeof(header.wadType) ) == 0)
		wadInfo.type = WadType::IWAD;
	else if (strncmp( header.wadType, "PWAD", sizeof(header.wadType) ) == 0)
		wadInfo.type = WadType::PWAD;
	else
		wadInfo.type = WadType::Neither;

	if (wadInfo.type == WadType::Neither)  // not a WAD format
	{
		wadInfo.status = ReadStatus::InvalidFormat;
		return wadInfo;
	}

	if (header.lumpDirOffset >= file.size() || header.numLumps < 1 || header.numLumps > 65536)  // some garbage -> not a WAD
	{
		wadInfo.status = ReadStatus::InvalidFormat;
		return wadInfo;
	}

	if (!file.seek( header.lumpDirOffset ))
	{
		wadInfo.status = ReadStatus::FailedToRead;
		return wadInfo;
	}

	// the lump directory is basically an array of LumpEntry structs, so let's read it all at once
	qint64 lumpDirSize = header.numLumps * sizeof(LumpEntry);
	std::unique_ptr< LumpEntry [] > lumpDir( new LumpEntry [header.numLumps] );
	if (file.read( (char*)lumpDir.get(), lumpDirSize ) < lumpDirSize)
	{
		wadInfo.status = ReadStatus::FailedToRead;
		return wadInfo;
	}

	for (uint32_t i = 0; i < header.numLumps; ++i)
	{
		LumpEntry & lump = lumpDir[i];

		// we need to make sure we have a null-terminated string, because the original one isn't when it's 8 chars long
		char lumpName0 [9];
		strncpy( lumpName0, lump.name, std::min( sizeof(lumpName0), sizeof(lump.name) ) );
		lumpName0[8] = '\0';
		QString lumpName( lumpName0 );

		if (lump.dataOffset > file.size() || !isPrintableAsciiString( lumpName ))  // some garbage -> not a WAD
		{
			wadInfo.status = ReadStatus::InvalidFormat;
			return wadInfo;
		}

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
				wadInfo.status = ReadStatus::FailedToRead;
				return wadInfo;
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

	wadInfo.status = ReadStatus::Success;
	return wadInfo;
}

const WadInfo & getCachedWadInfo( const QString & filePath )
{
  // Opening and reading from a file is expensive, so we cache the results here so that subsequent calls are fast.
  // The cache is global for the whole process because why not.
  static QHash< QString, WadInfo > g_cachedWadTypes;

	auto pos = g_cachedWadTypes.find( filePath );
	if (pos == g_cachedWadTypes.end() || pos->status == ReadStatus::FailedToRead)  // if it failed previously, try again
	{
		WadInfo wadInfo = readWadInfoFromFile( filePath );
		pos = g_cachedWadTypes.insert( filePath, std::move(wadInfo) );
	}

	if (pos->status == ReadStatus::FailedToRead)
	{
		qWarning() << "failed to read file" << filePath;
	}
	else if (pos->status == ReadStatus::InvalidFormat)
	{
		//qDebug() << "file" << filePath << "is not a WAD";
	}

	return pos.value();
}
