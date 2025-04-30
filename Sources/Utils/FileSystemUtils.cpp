//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: utilities concerning paths, directories and files
//======================================================================================================================

#include "FileSystemUtils.hpp"
#include "StringUtils.hpp"

#include <QDirIterator>
#include <QFile>
#include <QSaveFile>
#include <QStringBuilder>
#include <QTextStream>
#include <QRegularExpression>
#include <QThread>  // sleep

#if IS_WINDOWS
	#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
		#include <mutex>

		extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
		static std::mutex qt_ntfs_permission_mtx;

		struct NtfsPermissionCheckGuard
		{
			NtfsPermissionCheckGuard()
			{
				qt_ntfs_permission_mtx.lock();
				qt_ntfs_permission_lookup = 1;
			}
			~NtfsPermissionCheckGuard()
			{
				qt_ntfs_permission_mtx.unlock();
				qt_ntfs_permission_lookup = 0;
			}
		};
	#else
		using NtfsPermissionCheckGuard = QNtfsPermissionCheckGuard;
	#endif // QT_VERSION < 6.6
#endif // IS_WINDOWS


//======================================================================================================================

namespace fs {


const QString currentDir(".");


void forEachParentDir( const QString & path, const std::function< void ( const QString & parentDir ) > & loopBody )
{
	QString parentDirPath = fs::getNormalizedPath( path );
	qsizetype lastSlashPos = 0;
	while ((lastSlashPos = parentDirPath.lastIndexOf( '/', lastSlashPos - 1 )) >= 0)
	{
		parentDirPath.resize( lastSlashPos );
		loopBody( parentDirPath );
	}
}

bool isDirectoryWritable( const QString & dirPath )
{
	bool isWritable = false;
	{
 #if IS_WINDOWS
		NtfsPermissionCheckGuard ntfsGuard;
 #endif
		QFileInfo dir( dirPath );
		isWritable = dir.exists() && dir.isWritable();
	}
	return isWritable;
}

#if IS_WINDOWS
	#define PATH_BEGINNING "(\\w:)?"
#else
	#define PATH_BEGINNING
#endif
#define DISALLOWED_PATH_SYMBOLS ":*?\"<>|"

const QRegularExpression & getPathRegex()
{
	static const QRegularExpression pathRegex("^" PATH_BEGINNING "[^" DISALLOWED_PATH_SYMBOLS "]*$");
	return pathRegex;
}

static QString sanitizePath_impl( const QString & path, const QRegularExpression & invalidChars )
{
	if (IS_WINDOWS && path.size() >= 2 && isLetter( path[0] ) && path[1] == ':')
	{
		QString sanitizedDriveLocalPath = path.mid(2);  // part without the initial drive letter
		sanitizedDriveLocalPath.remove( invalidChars );  // the ':' is allowed in the driver letter, but not anywhere else
		return path[0] % path[1] % sanitizedDriveLocalPath;
	}
	else
	{
		QString sanitizedPath = path;
		sanitizedPath.remove( invalidChars );
		return sanitizedPath;
	}
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

QString readAllFileLines( const QString & filePath, QStringList & lines )
{
	QFile file( filePath );
	if (!file.open( QIODevice::Text | QIODevice::ReadOnly ))
	{
		return "Could not open file "%filePath%" for reading ("%file.errorString()%")";
	}

	QTextStream stream( &file );
	while (!stream.atEnd())
	{
		QString line = stream.readLine();
		if (file.error() != QFile::NoError)
		{
			return "Error occured while reading a file "%filePath%" ("%file.errorString()%")";
		}
		lines.append( std::move(line) );
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
