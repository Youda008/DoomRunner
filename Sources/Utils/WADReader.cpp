//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: WAD file parsing and information extraction
//======================================================================================================================

#include "WADReader.hpp"

#include "DoomFiles.hpp"  // identifyGame
#include "ErrorHandling.hpp"

#include <QSet>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>

#include <cctype>
#include <memory>


namespace doom {


//======================================================================================================================
// implementation

// logging helper
class LoggingWadReader : protected LoggingComponent {

 public:

	LoggingWadReader( QString filePath ) : LoggingComponent( u"WadReader" ), _filePath( std::move(filePath) ) {}

	UncertainWadInfo readWadInfo();

 private:

	QString _filePath;

};


//----------------------------------------------------------------------------------------------------------------------
// WAD format parsing

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
	const auto strLatin = str.toLatin1();
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

UncertainWadInfo LoggingWadReader::readWadInfo()
{
	UncertainWadInfo wadInfo;

	if (!fs::isValidFile( _filePath ))
	{
		wadInfo.status = ReadStatus::NotFound;
		return wadInfo;
	}

	QFile file( _filePath );
	if (!file.open( QIODevice::ReadOnly ))
	{
		logRuntimeError().noquote() << "Cannot open \""<<_filePath<<"\": "<<file.errorString();
		wadInfo.status = ReadStatus::CantOpen;
		return wadInfo;
	}

	const qint64 fileSize = file.size();
	if (fileSize < 0)
	{
		logLogicError() << "file size is negative ("<<fileSize<<"), wtf??";
		wadInfo.status = ReadStatus::FailedToRead;
		return wadInfo;
	}

	// read and validate WAD header

	WadHeader header;
	if (qint64( sizeof(header) ) > fileSize)
	{
		logDebug() << _filePath << " is smaller than WAD header";
		wadInfo.status = ReadStatus::InvalidFormat;
		return wadInfo;
	}
	else if (file.read( (char*)&header, sizeof(header) ) < qint64( sizeof(header) ))
	{
		logRuntimeError() << _filePath << ": failed to read WAD header";
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
		logDebug() << _filePath << ": invalid WAD signature";
		wadInfo.status = ReadStatus::InvalidFormat;
		return wadInfo;
	}

	// read all lumps

	if (header.numLumps < 1 || header.numLumps > 65536)  // some garbage -> not a WAD
	{
		logDebug() << _filePath << ": invalid number of lumps";
		wadInfo.status = ReadStatus::InvalidFormat;
		return wadInfo;
	}
	qint64 lumpDirSize = header.numLumps * sizeof(LumpEntry);
	if (header.lumpDirOffset + lumpDirSize > fileSize)
	{
		logDebug() << _filePath << ": lump header points beyond the end of file";
		wadInfo.status = ReadStatus::InvalidFormat;
		return wadInfo;
	}
	// the lump directory is basically an array of LumpEntry structs, so let's read it all at once
	std::unique_ptr< LumpEntry [] > lumpDir( new LumpEntry [header.numLumps] );
	if (!file.seek( header.lumpDirOffset ) || file.read( (char*)lumpDir.get(), lumpDirSize ) < lumpDirSize)
	{
		logRuntimeError() << _filePath << ": failed to read the lump directory";
		wadInfo.status = ReadStatus::FailedToRead;
		return wadInfo;
	}

	QSet< QString > lumpNames;

	for (uint32_t i = 0; i < header.numLumps; ++i)
	{
		LumpEntry & lump = lumpDir[i];
		QString lumpName = charArrayToString( lump.name );

		if (lump.dataOffset + lump.size > fileSize)  // some garbage -> not a WAD
		{
			logDebug() << _filePath << ": lump points beyond the end of file";
			wadInfo.status = ReadStatus::InvalidFormat;
			return wadInfo;
		}
		else if (!isPrintableAsciiString( lumpName ))  // some garbage -> not a WAD
		{
			logDebug() << _filePath << ": lump name is not a printable text";
			wadInfo.status = ReadStatus::InvalidFormat;
			return wadInfo;
		}

		lumpNames.insert( lumpName );

		// try to gather the map names from the marker lumps,
		// but if we find a MAPINFO lump, let that one override the markers

		if (isMapMarker( lump, lumpName ))
		{
			wadInfo.mapInfo.mapNames.append( lumpName );
		}

		if (lumpName == "MAPINFO")
		{
			qint64 origPos = file.pos();

			if (!file.seek( lump.dataOffset ))
			{
				logRuntimeError() << _filePath << ": failed to seek to lump offset";
				wadInfo.status = ReadStatus::FailedToRead;
				return wadInfo;
			}

			QByteArray lumpData = file.read( lump.size );
			if (lumpData.size() < qsize_t( lump.size ))
			{
				file.seek( origPos );
				continue;
			}

			wadInfo.mapInfo = parseMapInfo( lumpData );

			// If it's PWAD, we are done, list of maps is all we need.
			// If it's IWAD, we need to go through all the lumps, in order to identify the game.
			if (wadInfo.type != WadType::IWAD)
			{
				break;
			}
		}
	}

	wadInfo.status = ReadStatus::Success;

	if (wadInfo.type == WadType::IWAD)
	{
		wadInfo.game = identifyGame( lumpNames );
	}

	return wadInfo;
}


//======================================================================================================================
// public API

UncertainWadInfo readWadInfo( const QString & filePath )
{
	LoggingWadReader wadReader( filePath );
	return wadReader.readWadInfo();
}

FileInfoCache< WadInfo > g_cachedWadInfo( readWadInfo );


//======================================================================================================================


} // namespace doom
