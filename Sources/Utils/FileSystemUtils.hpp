//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: utilities concerning paths, directories and files
//======================================================================================================================

#ifndef FILE_SYSTEM_UTILS_INCLUDED
#define FILE_SYSTEM_UTILS_INCLUDED


#include "Essential.hpp"

#include <QString>
#include <QStringBuilder>
#include <QByteArray>
#include <QDir>
#include <QFileInfo>

class QModelIndex;
class PathConvertor;

#include <functional>


//======================================================================================================================
//  general

constexpr bool QuotePaths = true;
constexpr bool DontQuotePaths = false;

inline QString quoted( const QString & path )
{
	return '"' % path % '"';
}

enum class PathStyle : uint8_t
{
	Relative,
	Absolute
};


//======================================================================================================================
//  general helper functions

namespace fs {

inline bool isAbsolutePath( const QString & path )
{
	return QDir::isAbsolutePath( path );
}

inline bool isRelativePath( const QString & path )
{
	return !QDir::isAbsolutePath( path );
}

inline PathStyle getPathStyle( const QString & path )
{
	return isAbsolutePath( path ) ? PathStyle::Absolute : PathStyle::Relative;
}

inline bool isDirectory( const QString & path )
{
	return QFileInfo( path ).isDir();
}

inline bool isValidDir( const QString & dirPath )
{
	return !dirPath.isEmpty() && QFileInfo( dirPath ).isDir();
}

inline bool isInvalidDir( const QString & dirPath )
{
	return !dirPath.isEmpty() && !QFileInfo( dirPath ).isDir();  // it exists but it's not a dir
}

inline bool isValidFile( const QString & filePath )
{
	return !filePath.isEmpty() && QFileInfo( filePath ).isFile();
}

inline bool isInvalidFile( const QString & filePath )
{
	return !filePath.isEmpty() && !QFileInfo( filePath ).isFile();  // it exists but it's not a file
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
	return !dirPath.isEmpty() ? QDir( dirPath ).filePath( fileName ) : fileName;
}

inline QString getAbsolutePathFromFileName( const QString & dirPath, const QString & fileName )
{
	return QDir( dirPath ).absoluteFilePath( fileName );
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

} // namespace fs


//======================================================================================================================
//  traversing directory content

namespace fs {

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
	const PathConvertor & pathConvertor, const std::function< void ( const QFileInfo & entry ) > & visitEntry
);

} // namespace fs


//======================================================================================================================
/** Helper for calculating relative and absolute paths according to current directory and path style settings. */

class PathConvertor {

	QDir _baseDir;  ///< directory which relative paths are relative to
	PathStyle _pathStyle;  ///< whether to store paths to engines, IWADs, maps and mods in absolute or relative form

 public:

	PathConvertor( const QDir & baseDir, PathStyle pathStyle )
		: _baseDir( baseDir ), _pathStyle( pathStyle ) {}

	PathConvertor( const QDir & baseDir, bool useAbsolutePaths )
		: PathConvertor( baseDir, useAbsolutePaths ? PathStyle::Absolute : PathStyle::Relative ) {}

	PathConvertor( const PathConvertor & other ) = default;
	PathConvertor( PathConvertor && other ) = default;
	PathConvertor & operator=( const PathConvertor & other ) = default;
	PathConvertor & operator=( PathConvertor && other ) = default;

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

};


/** Helper that allows rebasing path from one baseDir to another. */
class PathRebaser {

	QDir _inBaseDir;  ///< base dir for the relative input paths
	QDir _outBaseDir;  ///< base dir for the relative output paths
	PathStyle _outPathStyle;  ///< whether the output paths should be relative or absolute
	bool _quotePaths;  ///< whether to surround all output paths with quotes (needed when generating a batch)
	                   // !!IMPORTANT!! Never store the quoted paths and pass them back to PathConvertor, they are output-only.
 public:

	PathRebaser( const QDir & inputBaseDir, const QDir & outputBaseDir, PathStyle pathStyle, bool quotePaths = false )
		: _inBaseDir( inputBaseDir ), _outBaseDir( outputBaseDir ), _outPathStyle( pathStyle ), _quotePaths( quotePaths ) {}

	PathRebaser( const PathRebaser & other ) = default;
	PathRebaser( PathRebaser && other ) = default;
	PathRebaser & operator=( const PathRebaser & other ) = default;
	PathRebaser & operator=( PathRebaser && other ) = default;

	const QDir & inputBaseDir() const                  { return _inBaseDir; }
	const QDir & outputBaseDir() const                 { return _outBaseDir; }
	PathStyle outputPathStyle() const                  { return _outPathStyle; }
	bool outputAbsolutePaths() const                   { return _outPathStyle == PathStyle::Absolute; }

	void setInputBaseDir( const QDir & baseDir )       { _inBaseDir = baseDir; }
	void setOutputBaseDir( const QDir & baseDir )      { _outBaseDir = baseDir; }

	QString rebasePath( const QString & path ) const
	{
		return rebasePathFromTo( path, _inBaseDir, _outBaseDir );
	}
	QString rebasePathBack( const QString & path ) const
	{
		return rebasePathFromTo( path, _outBaseDir, _inBaseDir );
	}

	QString rebaseAndQuotePath( const QString & path ) const
	{
		return maybeQuoted( rebasePath( path ) );
	}

	QString maybeQuoted( const QString & path ) const
	{
		if (_quotePaths)
			return quoted( path );
		else
			return path;
	}

 private:

	QString rebasePathFromTo( const QString & path, const QDir & inputBaseDir, const QDir & outputBaseDir ) const
	{
		if (path.isEmpty())
			return {};

		QString absPath = fs::isAbsolutePath( path ) ? path : inputBaseDir.filePath( path );
		QString newPath = outputAbsolutePaths() ? absPath : outputBaseDir.relativeFilePath( absPath );

		return newPath;
	}

};


#endif // FILE_SYSTEM_UTILS_INCLUDED
