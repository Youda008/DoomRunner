//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: utilities concerning paths, directories and files
//======================================================================================================================

#include "FileSystemUtils.hpp"

#include <QDirIterator>
#include <QFile>
#include <QSaveFile>
#include <QStringBuilder>
#include <QRegularExpression>
#include <QThread>  // sleep

#include <mutex>

#if IS_WINDOWS
// Writing into this external global variable is the official way to enable querying NTFS permissions. Seriously Qt?!?
extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
static std::mutex qt_ntfs_permission_mtx;
#endif

#include <QDebug>


//======================================================================================================================

namespace fs {


const QString currentDir(".");


void forEachParentDir( const QString & path, const std::function< void ( const QString & parentDir ) > & loopBody )
{
	QString parentDirPath = fs::getNormalizedPath( path );
	int lastSlashPos = 0;
	while ((lastSlashPos = parentDirPath.lastIndexOf( '/', lastSlashPos - 1 )) >= 0)
	{
		parentDirPath.resize( lastSlashPos );
		loopBody( parentDirPath );
	}
}

bool isDirectoryWritable( const QString & dirPath )
{
	bool isWritable = false;
	QFileInfo dir( dirPath );
	{
 #if IS_WINDOWS
		std::unique_lock< std::mutex > lock( qt_ntfs_permission_mtx );
		qt_ntfs_permission_lookup = 1;
 #endif
		isWritable = dir.exists() && dir.isWritable();
 #if IS_WINDOWS
		qt_ntfs_permission_lookup = 0;
 #endif
	}
	return isWritable;
}

#define DISALLOWED_PATH_SYMBOLS ":*?\"<>|"

const QRegularExpression & getPathRegex()
{
	static const QRegularExpression pathRegex("^[^" DISALLOWED_PATH_SYMBOLS "]*$");
	return pathRegex;
}

static QString sanitizePath_impl( const QString & path, const QRegularExpression & invalidChars )
{
	QString sanitizedPath = path;
	sanitizedPath.remove( invalidChars );
	return sanitizedPath;
}

QString sanitizePath( const QString & path )
{
	static const QRegularExpression invalidChars("[" DISALLOWED_PATH_SYMBOLS "]");
	return sanitizePath_impl( path, invalidChars );
}

QString sanitizePath_strict( const QString & path )
{
	// Newer engines such as GZDoom 4.x can handle advanced Unicode characters such as emojis,
	// but the old ones are pretty much limited to ASCII, so it's easier to just stick to a "safe" white-list.
	static const QRegularExpression invalidChars("[^a-zA-Z0-9_ !#$&'\\(\\)+,\\-.;=@\\[\\]\\^~]");
	return sanitizePath_impl( path, invalidChars );
}

QString readWholeFile( const QString & filePath, QByteArray & dest )
{
	QFile file( filePath );
	if (!file.open( QIODevice::ReadOnly ))
	{
		return "Could not open file "%filePath%" for reading ("%file.errorString()%")";
	}

	dest = file.readAll();
	if (file.error() != QFile::NoError)
	{
		return "Error occured while reading a file "%filePath%" ("%file.errorString()%")";
	}

	file.close();
	return {};
}

QString updateFileSafely( const QString & filePath, const QByteArray & newContent )
{
	QSaveFile file( filePath );
	if (!file.open( QIODevice::WriteOnly ))
	{
		return "Could not open file "%filePath%" for writing ("%file.errorString()%")";
	}

	file.write( newContent );
	if (file.error() != QFile::NoError)
	{
		return "Could not write to file "%filePath%" ("%file.errorString()%")";
	}

	if (!file.commit())
	{
		return "Could not commit the changes to file "%filePath%" ("%file.errorString()%")";
	}

	return {};
}

void traverseDirectory(
	const QString & dir, bool recursively, EntryTypes typesToVisit,
	const PathConvertor & pathConvertor, const std::function< void ( const QFileInfo & entry ) > & visitEntry
)
{
	if (dir.isEmpty())
		return;

	QDir dir_( dir );
	if (!dir_.exists())
		return;

	QDirIterator dirIt( dir_ );
	while (dirIt.hasNext())
	{
		QString entryPath = pathConvertor.convertPath( dirIt.next() );
		QFileInfo entry( entryPath );
		if (entry.isDir())
		{
			QString dirName = dirIt.fileName();  // we need the original entry name including "." and "..", entry is already converted
			if (dirName != "." && dirName != "..")
			{
				if (typesToVisit.isSet( EntryType::DIR ))
					visitEntry( entry );
				if (recursively)
					traverseDirectory( entry.filePath(), recursively, typesToVisit, pathConvertor, visitEntry );
			}
		}
		else
		{
			if (typesToVisit.isSet( EntryType::FILE ))
				visitEntry( entry );
		}
	}
}


} // namespace fs
