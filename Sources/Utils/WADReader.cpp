//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: WAD file parsing and information extraction
//======================================================================================================================

#include "WADReader.hpp"

#include "JsonUtils.hpp"
#include "ErrorHandling.hpp"

#include <QHash>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QTextStream>
#include <QRegularExpression>

#include <cctype>


namespace doom {


//======================================================================================================================
//  WAD info loading

//  https://doomwiki.org/wiki/WAD

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

template< size_t N >
QString charArrayToString( const char (&arr) [N] )
{
	// we need to make sure we have a null-terminated string, because the original one isn't when it's 8 chars long
	char arr0 [N+1];
	strncpy( arr0, arr, std::min( sizeof(arr0), sizeof(arr) ) );
	arr0[N] = '\0';
	return QString( arr0 );
}

static bool isPrintableAsciiString( const QString & str )
{
	auto strLatin = str.toLatin1();
	for (auto c : strLatin)
		if (!isprint( c ))
			return false;
	return true;
}

static const QSet< QString > blacklistedNames =
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

static bool isMapMarker( const LumpEntry & lump, const QString & lumpName )
{
	return lump.size == 0
		&& !lumpName.endsWith("_START") && !lumpName.endsWith("_END")
		&& !lumpName.endsWith("_S") && !lumpName.endsWith("_E")
		&& !blacklistedNames.contains( lumpName );
}

static void getMapNamesFromMAPINFO( const QByteArray & lumpData, QStringVec & mapNames )
{
	QTextStream lumpText( lumpData, QIODevice::ReadOnly );

	static const QRegularExpression mapDefRegex("map\\s+(\\w+)\\s+\"([^\"]+)\"");

	QString line;
	while (lumpText.readLineInto( &line ))
	{
		auto match = mapDefRegex.match( line );
		if (match.hasMatch())
		{
			mapNames.append( match.captured(1) );
		}
	}
}

WadInfo readWadInfo( const QString & filePath )
{
	WadInfo wadInfo;

	QFile file( filePath );
	if (!file.open( QIODevice::ReadOnly ))
	{
		logRuntimeError("WadReader").noquote() << "Cannot open \""<<filePath<<"\": "<<file.errorString();
		wadInfo.status = ReadStatus::CantOpen;
		return wadInfo;
	}

	const qint64 fileSize = file.size();
	if (fileSize < 0)
	{
		logLogicError("WadReader") << "file size is negative ("<<fileSize<<"), wtf??";
		wadInfo.status = ReadStatus::FailedToRead;
		return wadInfo;
	}

	// read and validate WAD header

	WadHeader header;
	if (qint64( sizeof(header) ) > fileSize)
	{
		logDebug("WadReader") << filePath << " is smaller than WAD header";
		wadInfo.status = ReadStatus::InvalidFormat;
		return wadInfo;
	}
	else if (file.read( (char*)&header, sizeof(header) ) < qint64( sizeof(header) ))
	{
		logDebug("WadReader") << filePath << ": failed to read WAD header";
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
		logDebug("WadReader") << filePath << ": invalid WAD signature";
		wadInfo.status = ReadStatus::InvalidFormat;
		return wadInfo;
	}

	// read all lumps

	if (header.numLumps < 1 || header.numLumps > 65536)  // some garbage -> not a WAD
	{
		logDebug("WadReader") << filePath << ": invalid number of lumps";
		wadInfo.status = ReadStatus::InvalidFormat;
		return wadInfo;
	}
	qint64 lumpDirSize = header.numLumps * sizeof(LumpEntry);
	if (header.lumpDirOffset + lumpDirSize > fileSize)
	{
		logDebug("WadReader") << filePath << ": lump header points beyond the end of file";
		wadInfo.status = ReadStatus::InvalidFormat;
		return wadInfo;
	}
	// the lump directory is basically an array of LumpEntry structs, so let's read it all at once
	std::unique_ptr< LumpEntry [] > lumpDir( new LumpEntry [header.numLumps] );
	if (!file.seek( header.lumpDirOffset ) || file.read( (char*)lumpDir.get(), lumpDirSize ) < lumpDirSize)
	{
		logDebug("WadReader") << filePath << ": failed to read the lump directory";
		wadInfo.status = ReadStatus::FailedToRead;
		return wadInfo;
	}

	for (uint32_t i = 0; i < header.numLumps; ++i)
	{
		LumpEntry & lump = lumpDir[i];
		QString lumpName = charArrayToString( lump.name );

		if (lump.dataOffset + lump.size > fileSize || !isPrintableAsciiString( lumpName ))  // some garbage -> not a WAD
		{
			logDebug("WadReader") << filePath << ": lump points beyond the end of file";
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
				logDebug("WadReader") << filePath << ": failed to seek to lump offset";
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


FileInfoCache< WadInfo_ > g_cachedWadInfo( readWadInfo );


//----------------------------------------------------------------------------------------------------------------------
//  serialization

void WadInfo_::serialize( QJsonObject & jsWadInfo ) const
{
	jsWadInfo["type"] = int( type );
	jsWadInfo["map_names"] = serializeStringVec( mapNames );
}

void WadInfo_::deserialize( const JsonObjectCtx & jsWadInfo )
{
	type = jsWadInfo.getEnum< doom::WadType >( "type", doom::WadType::Neither );
	if (JsonArrayCtx jsMapNames = jsWadInfo.getArray( "map_names" ))
		mapNames = deserializeStringVec( jsMapNames );
}


} // namespace doom
