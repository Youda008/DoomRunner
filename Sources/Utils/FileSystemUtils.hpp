//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: utilities concerning paths, directories and files
//======================================================================================================================

#ifndef FILE_SYSTEM_UTILS_INCLUDED
#define FILE_SYSTEM_UTILS_INCLUDED


#include "Common.hpp"

#include "OSUtils.hpp"

#include <QString>
#include <QStringBuilder>
#include <QByteArray>
#include <QDir>
#include <QFileInfo>

class QModelIndex;

#include <functional>


//======================================================================================================================
//  general

enum class PathStyle : uint8_t
{
	Relative,
	Absolute
};

constexpr bool QuotePaths = true;
constexpr bool DontQuotePaths = false;

inline QString quoted( const QString & path )
{
	return '"' % path % '"';
}


//======================================================================================================================
/** Helper for calculating relative and absolute paths according to current directory and settings. */

class PathContext {

	QDir _baseDir;  ///< directory which relative paths are relative to
	QDir _prevBaseDir;  ///< original base dir for rebasing paths to another base
	PathStyle _pathStyle;  ///< whether to store paths to engines, IWADs, maps and mods in absolute or relative form
	bool _quotePaths;  ///< whether to surround all paths with quotes (needed when generating a batch)
	                   // !!IMPORTANT!! Never store the quoted paths and pass them back to PathContext, they are output-only.
 public:

	PathContext( const QDir & baseDir, bool useAbsolutePaths, bool quotePaths = false )
		: PathContext( baseDir, useAbsolutePaths ? PathStyle::Absolute : PathStyle::Relative, quotePaths ) {}

	PathContext( const QDir & baseDir, PathStyle pathStyle, bool quotePaths = false )
		: _baseDir( baseDir ), _prevBaseDir(), _pathStyle( pathStyle ), _quotePaths( quotePaths ) {}

	PathContext( const QDir & baseDir, const QDir & prevBaseDir, PathStyle pathStyle, bool quotePaths = false )
		: _baseDir( baseDir ), _prevBaseDir( prevBaseDir ), _pathStyle( pathStyle ), _quotePaths( quotePaths ) {}

	PathContext( const PathContext & other ) = default;
	PathContext( PathContext && other ) = default;
	PathContext & operator=( const PathContext & other ) = default;
	PathContext & operator=( PathContext && other ) = default;

	const QDir & baseDir() const                       { return _baseDir; }
	PathStyle pathStyle() const                        { return _pathStyle; }
	bool usingAbsolutePaths() const                    { return _pathStyle == PathStyle::Absolute; }
	bool usingRelativePaths() const                    { return _pathStyle == PathStyle::Relative; }

	void setBaseDir( const QDir & baseDir )            { _baseDir = baseDir; }
	void setPathStyle( PathStyle pathStyle )           { _pathStyle = pathStyle; }
	void toggleAbsolutePaths( bool useAbsolutePaths )  { _pathStyle = useAbsolutePaths ? PathStyle::Absolute : PathStyle::Relative; }

	QString getAbsolutePath( const QString & path ) const
	{
		return path.isEmpty() ? QString() : QFileInfo( _baseDir, path ).absoluteFilePath();
	}
	QString getRelativePath( const QString & path ) const
	{
		return path.isEmpty() ? QString() : _baseDir.relativeFilePath( path );
	}
	QString convertPath( const QString & path ) const
	{
		return usingAbsolutePaths() ? getAbsolutePath( path ) : getRelativePath( path );
	}

	QString rebasePath( const QString & path, bool isExecutable = false ) const
	{
		if (path.isEmpty())
			return {};

		QString absPath = QDir::isAbsolutePath( path ) ? path : _prevBaseDir.filePath( path );
		QString newPath = usingAbsolutePaths() ? absPath : _baseDir.relativeFilePath( absPath );

		return isExecutable ? fixExePath( newPath ) : newPath;
	}

	QString rebaseAndQuotePath( const QString & path ) const
	{
		return maybeQuoted( rebasePath( path, false ) );
	}
	QString rebaseAndQuoteExePath( const QString & path ) const
	{
		return maybeQuoted( rebasePath( path, true ) );
	}

 private:

	QString maybeQuoted( const QString & path ) const
	{
		if (_quotePaths)
			return quoted( path );
		else
			return path;
	}

	/// On Unix, to run an executable file inside current working directory, the relative path needs to be prepended by "./"
	QString fixExePath( const QString & exePath ) const
	{
		if (!isWindows() && !exePath.contains("/"))  // the file is in the current working directory
		{
			return "./" + exePath;
		}
		return exePath;
	}

};


//======================================================================================================================
//  misc helper functions

inline bool isDirectory( const QString & path )
{
	return QFileInfo( path ).isDir();
}

inline bool isValidDir( const QString & dirPath )
{
	return !dirPath.isEmpty() && QDir( dirPath ).exists();
}

inline bool isInvalidDir( const QString & dirPath )
{
	return !dirPath.isEmpty() && !QDir( dirPath ).exists();  // either doesn't exist or it's a file
}

inline bool isValidFile( const QString & filePath )
{
	return !filePath.isEmpty() && QFileInfo( filePath ).isFile();
}

inline bool isInvalidFile( const QString & filePath )
{
	return !filePath.isEmpty() && !QFileInfo( filePath ).isFile();  // either doesn't exist or it's a directory
}

inline bool isValidEntry( const QString & path )
{
	return !path.isEmpty() && QFileInfo::exists( path );
}

inline QString getAbsolutePath( const QString & path )
{
	return QFileInfo( path ).absoluteFilePath();
}

inline QString getPathFromFileName( const QString & dirPath, const QString & fileName )
{
	return QDir( dirPath ).filePath( fileName );
}

inline QString getFileNameFromPath( const QString & filePath )
{
	return QFileInfo( filePath ).fileName();
}

inline QString getFileBasenameFromPath( const QString & filePath )
{
	return QFileInfo( filePath ).baseName();
}

inline QString getDirOfFile( const QString & filePath )
{
	return QFileInfo( filePath ).path();
}

inline QString getAbsoluteDirOfFile( const QString & filePath )
{
	return QFileInfo( filePath ).absolutePath();
}

inline QString getDirnameOfFile( const QString & filePath )
{
	return QFileInfo( filePath ).dir().dirName();
}

inline bool isInsideDir( const QString & entryPath, const QDir & dir )
{
	return QFileInfo( entryPath ).absoluteFilePath().startsWith( dir.absolutePath() );
}

/// Creates directory if it doesn't exist already, returns false if it doesn't exist and cannot be created.
inline bool createDirIfDoesntExist( const QString & dirPath )
{
	return QDir( dirPath ).mkpath(".");
}

/// Returns if it's possible to write files into a directory.
bool isDirectoryWritable( const QString & dirPath );

/// Replaces any disallowed path characters from a string.
QString sanitizePath( const QString & path );

/// Reads the whole content of a file into a byte array.
/** Returns description of an error that might potentially happen, or empty string on success. */
QString readWholeFile( const QString & filePath, QByteArray & dest );

/// Safely updates a file in a way that prevents content loss in the event of unexpected OS shutdown.
/** First saves the new content under a new name, then deletes the old file and then renames the new file to the old name.
  * Returns description of an error that might potentially happen, or empty string on success. */
QString updateFileSafely( const QString & filePath, const QByteArray & newContent );


//======================================================================================================================
//  traversing directory content

// Plain enum just makes the elements polute global namespace and makes the FILE collide with declaration in stdio.h,
// and enum class would make all bit operations incredibly painful.
class EntryTypes
{
	uint8_t _types;
 public:
	constexpr EntryTypes( uint8_t types ) : _types( types ) {}
	constexpr friend EntryTypes operator|( EntryTypes a, EntryTypes b ) { return EntryTypes( a._types | b._types ); }
	constexpr bool isSet( EntryTypes types ) const { return (_types & types._types) != 0; }
};
struct EntryType
{
	static constexpr EntryTypes DIR  = { 1 << 0 };
	static constexpr EntryTypes FILE = { 1 << 1 };
	static constexpr EntryTypes BOTH = DIR | FILE;
};

void traverseDirectory(
	const QString & dir, bool recursively, EntryTypes typesToVisit,
	const PathContext & pathContext, const std::function< void ( const QFileInfo & entry ) > & visitEntry
);


#endif // FILE_SYSTEM_UTILS_INCLUDED
