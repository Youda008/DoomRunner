//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: utilities concerning paths, directories and files
//======================================================================================================================

#ifndef FILE_SYSTEM_UTILS_INCLUDED
#define FILE_SYSTEM_UTILS_INCLUDED


#include "Common.hpp"

class QModelIndex;

#include <QString>
#include <QStringBuilder>
#include <QDir>
#include <QFileInfo>

#include <functional>


//======================================================================================================================
//  general

enum class PathStyle
{
	Relative,
	Absolute
};

constexpr bool QuotePaths = true;
constexpr bool DontQuotePaths = false;


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
	PathContext & operator=( const PathContext & other ) = default;

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

	QString rebasePath( const QString & path ) const
	{
		if (path.isEmpty())
			return {};

		QString absPath = QDir::isAbsolutePath( path ) ? path : _prevBaseDir.filePath( path );

		return usingAbsolutePaths() ? absPath : _baseDir.relativeFilePath( absPath );
	}

	QString rebasePathToRelative( const QString & path ) const
	{
		if (path.isEmpty())
			return {};

		QString absPath = QDir::isAbsolutePath( path ) ? path : _prevBaseDir.filePath( path );

		return _baseDir.relativeFilePath( absPath );
	}

	QString rebaseAndQuotePath( const QString & path ) const
	{
		return maybeQuoted( rebasePath( path ) );
	}

 private:

	QString maybeQuoted( const QString & path ) const
	{
		if (_quotePaths)
			return '"' % path % '"';
		else
			return path;
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

/// On Unix, to run an executable file inside current working directory, the relative path needs to be prepended by "./"
QString fixExePath( const QString & exePath );

/// Safely updates a file in a way that prevents content loss in the event of unexpected OS shutdown.
/** First saves the new content under a new name, then deletes the old file and then renames the new file to the old name. */
QString updateFile( const QString & filePath, const QByteArray & newContent );

/// Opens a directory of a file in a new File Explorer window.
bool openFileLocation( const QString & filePath );

/// Creates a file filter for the QFileDialog::getOpenFileNames.
QString makeFileFilter( const char * filterName, const QVector<QString> & suffixes );


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
