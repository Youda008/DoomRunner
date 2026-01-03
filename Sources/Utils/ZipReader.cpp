//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: WAD file parsing and information extraction
//======================================================================================================================

#include "ZipReader.hpp"

#include "LangUtils.hpp"
#include "StringUtils.hpp"  // operator<<( QTextStream &, const QStringList & )
#include "ErrorHandling.hpp"

#include <QFile>
#include <QFileInfo>

#include <minizip/unzip.h>


namespace doom {


//======================================================================================================================
// implementation

// logging helper
class LoggingZipReader : protected LoggingComponent {

 public:

	LoggingZipReader( QString filePath ) : LoggingComponent( u"ZipReader" ), _filePath( std::move(filePath) ) {}

	UncertainFileContent readOneOfFilesInsideZip( const QStringList & innerFileNames );

 private:

	QString _filePath;

};

UncertainFileContent LoggingZipReader::readOneOfFilesInsideZip( const QStringList & innerFileNames )
{
	// we need a distinguishable error code when the file does not exist
	if (!fs::isValidFile( _filePath ))
	{
		return ReadStatus::NotFound;
	}

	// open the zip file
	unzFile zipFile = unzOpen( _filePath.toUtf8().constData() );
	if (!zipFile)
	{
		logRuntimeError() << "Cannot open "<<_filePath;
		return ReadStatus::CantOpen;
	}
	auto zipFileGuard = autoClosable( zipFile, unzClose );

	// find one of innerFileNames
	QString foundInnerFileName;
	for (const QString & innerFileName : innerFileNames)
	{
		if (unzLocateFile( zipFile, innerFileName.toUtf8().constData(), 0 ) == UNZ_OK)
		{
			foundInnerFileName = innerFileName;
			break;
		}
	}
	if (foundInnerFileName.isNull())
	{
		logDebug() << "Couldn't find "<<innerFileNames<<" within "<<_filePath;
		return ReadStatus::InfoNotPresent;
	}

	// get metadata about the selected inner file
	unz_file_info fileInfo;
	char unused [256];
	if (unzGetCurrentFileInfo( zipFile, &fileInfo, unused, sizeof(unused), nullptr, 0, nullptr, 0 ) != UNZ_OK)
	{
		logRuntimeError() << "Failed to get file info of "<<foundInnerFileName<<" within "<<_filePath;
		return ReadStatus::CantOpen;
	}

	// safety check - don't try to decompress a file that is nonsensically large
	if (fileInfo.uncompressed_size > 10*1024*1024)
	{
		logRuntimeError() << "Refusing to read file "<<foundInnerFileName<<" within "<<_filePath
		                  << ", because it is too large ("<<fileInfo.uncompressed_size<<" bytes)";
		return ReadStatus::FailedToRead;
	}
	int uncompressedSize = static_cast< int >( fileInfo.uncompressed_size );

	// open the selected inner file for reading
	if (unzOpenCurrentFile( zipFile ) != UNZ_OK)
	{
		logRuntimeError() << "Failed to open file "<<foundInnerFileName<<" within "<<_filePath;
		return ReadStatus::CantOpen;
	}
	auto currentFileGuard = atScopeEndDo( [ &zipFile ]() { unzCloseCurrentFile( zipFile ); } );

	// decompress and read the inner file
	QByteArray buffer;
	buffer.resize( uncompressedSize );
	int bytesRead = unzReadCurrentFile( zipFile, buffer.data(), buffer.size() );
	if (bytesRead < 0)
	{
		logRuntimeError() << "Failed to read file "<<foundInnerFileName<<" within "<<_filePath;
		return ReadStatus::FailedToRead;
	}
	else if (bytesRead < uncompressedSize)
	{
		logRuntimeError() << "Couldn't read the whole file "<<foundInnerFileName<<" within "<<_filePath
		                  << " (read only "<<bytesRead<<" bytes)";
		buffer.resize( bytesRead );
	}
	return buffer;
}


//======================================================================================================================
// public API

UncertainFileContent readOneOfFilesInsideZip( const QString & zipFilePath, const QStringList & innerFileNames )
{
	LoggingZipReader zipReader( zipFilePath );
	return zipReader.readOneOfFilesInsideZip( innerFileNames );
}

UncertainZipInfo readZipInfo( const QString & filePath )
{
	UncertainZipInfo zipInfo;

	LoggingZipReader zipReader( filePath );
	auto mapInfoContent = zipReader.readOneOfFilesInsideZip({ "MAPINFO", "MAPINFO.txt" });
	if (!mapInfoContent)
	{
		zipInfo.status = mapInfoContent.error();
		return zipInfo;
	}

	zipInfo.mapInfo = doom::parseMapInfo( *mapInfoContent );

	zipInfo.status = !zipInfo.mapInfo.mapNames.isEmpty() ? ReadStatus::Success : ReadStatus::InfoNotPresent;
	return zipInfo;
}


} // namespace doom


FileInfoCache< doom::ZipInfo > g_cachedZipInfo( u"cachedZipInfo", doom::readZipInfo );
