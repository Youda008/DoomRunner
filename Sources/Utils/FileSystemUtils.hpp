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
// general

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
// general helper functions

namespace fs {

template< typename Appendable >
inline QString appendToPath( const QString & part1, const Appendable & part2 )
{
	if (!part1.isEmpty() && !part2.isEmpty())
		return part1%"/"%part2;
	else if (!part2.isEmpty())
		return part2;
	else // even if part1.isEmpty()
		return part1;
}

template< typename Appendable, typename ... Args >
inline QString appendToPath( const QString & part1, const Appendable & part2, const Args & ... otherParts )
{
	return appendToPath( appendToPath( part1, part2 ), otherParts ... );
}

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

inline bool exists( const QString & entryPath )
{
	return QFileInfo::exists( entryPath );
}

inline bool isDirectory( const QString & entryPath )
{
	return QFileInfo( entryPath ).isDir();
}

inline bool isFile( const QString & entryPath )
{
	return QFileInfo( entryPath ).isFile();
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

inline bool isValidEntry( const QString & entryPath )
{
	return !entryPath.isEmpty() && QFileInfo::exists( entryPath );
}

inline QString getAbsolutePath( const QString & path )
{
	// works even if the path is already absolute, also gets rid of any redundant "/./" or "/../" in the middle of the path
	return QFileInfo( path ).absoluteFilePath();
}

inline QString getNormalizedPath( const QString & path )
{
	// gets rid of any redundant "/./" or "/../" in the middle of the path
	return QFileInfo( path ).absoluteFilePath();  // canonicalFilePath() doesn't work if the FS entry doesn't exist yet
}

inline QString getPathFromFileName( const QString & dirPath, const QString & fileName )
{
	return !dirPath.isEmpty() ? QDir( dirPath ).filePath( fileName ) : fileName;
}

inline QString getAbsolutePathFromFileName( const QString & dirPath, const QString & fileName )
{
	// gets rid of any redundant "/./" or "/../" in the middle of dirPath
	return QDir( dirPath ).absoluteFilePath( fileName );
}

inline QString getFileNameFromPath( const QString & filePath )
{
	return QFileInfo( filePath ).fileName();
}

inline QString getFileBasenameFromPath( const QString & filePath )
{
	return QFileInfo( filePath ).completeBaseName();
}

inline QString getParentDir( const QString & entryPath )
{
	return QFileInfo( entryPath ).path();
}

inline QString getAbsoluteParentDir( const QString & entryPath )
{
	return QFileInfo( entryPath ).absolutePath();
}

inline QString getParentDirName( const QString & entryPath )
{
	return QFileInfo( entryPath ).dir().dirName();
}

inline QString replaceFileSuffix( const QString & filePath, const QString & newSuffix )
{
	QFileInfo fileInfo( filePath );
	QString newFileName = fileInfo.completeBaseName() % '.' % newSuffix;
	return fileInfo.dir().filePath( newFileName );
}

inline bool isInsideDir( const QDir & dir, const QString & entryPath )
{
	return QFileInfo( entryPath ).absoluteFilePath().startsWith( dir.absolutePath() );
}

/// Takes a path native to the current OS and converts it to a Qt internal path.
/** Works even if the path is already in Qt format. */
inline QString fromNativePath( const QString & path )
{
	return QDir::fromNativeSeparators( path );
}

/// Takes a Qt internal path and converts it to a path native to the current OS.
inline QString toNativePath( const QString & path )
{
	return QDir::toNativeSeparators( path );
}

/// Creates directory if it doesn't exist already, returns false if it doesn't exist and cannot be created.
inline bool createDirIfDoesntExist( const QString & dirPath )
{
	return QDir( dirPath ).mkpath(".");
}

/// Returns if it's possible to write files into a directory.
bool isDirectoryWritable( const QString & dirPath );

/// Returns a regular expression that can be used to validate file-system paths entered by the user.
const QRegularExpression & getPathRegex();

/// Removes any disallowed path characters from a string.
QString sanitizePath( const QString & path );

/// Removes anything but a strict set of safe characters.
QString sanitizePath_strict( const QString & path );

/// Reads the whole content of a file into a byte array.
/** Returns description of an error that might potentially happen, or empty string on success. */
QString readWholeFile( const QString & filePath, QByteArray & dest );

/// Safely updates a file in a way that prevents content loss in the event of unexpected OS shutdown.
/** First saves the new content under a new name, then deletes the old file and then renames the new file to the old name.
  * Returns description of an error that might potentially happen, or empty string on success. */
QString updateFileSafely( const QString & filePath, const QByteArray & newContent );

} // namespace fs


//======================================================================================================================
// traversing directory content

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
/** Helper for calculating relative and absolute paths according to a working directory and path style settings. */

class PathConvertor {

	QDir _workingDir;  ///< directory which relative paths are relative to
	PathStyle _pathStyle;  ///< whether to store paths to engines, IWADs, maps and mods in absolute or relative form

 public:

	PathConvertor( const QDir & workingDir, PathStyle pathStyle )
		: _workingDir( workingDir ), _pathStyle( pathStyle ) {}

	PathConvertor( const QDir & baseDir, bool useAbsolutePaths )
		: PathConvertor( baseDir, useAbsolutePaths ? PathStyle::Absolute : PathStyle::Relative ) {}

	PathConvertor( const PathConvertor & other ) = default;
	PathConvertor( PathConvertor && other ) = default;
	PathConvertor & operator=( const PathConvertor & other ) = default;
	PathConvertor & operator=( PathConvertor && other ) = default;

	const QDir & workingDir() const                    { return _workingDir; }
	PathStyle pathStyle() const                        { return _pathStyle; }
	bool usingAbsolutePaths() const                    { return _pathStyle == PathStyle::Absolute; }
	bool usingRelativePaths() const                    { return _pathStyle == PathStyle::Relative; }

	void setWorkingDir( const QDir & workingDir )      { _workingDir = workingDir; }
	void setPathStyle( PathStyle pathStyle )           { _pathStyle = pathStyle; }
	void toggleAbsolutePaths( bool useAbsolutePaths )  { _pathStyle = useAbsolutePaths ? PathStyle::Absolute : PathStyle::Relative; }

	QString getAbsolutePath( const QString & path ) const
	{
		// _workingDir.absoluteFilePath( fileName ) only appends path to the absolute path of _workingDir,
		// QDir::cleanPath() gets rid of any redundant "/./" or "/../" in the middle of the input path.
		// Works even if path is already absolute.
		return path.isEmpty() ? QString() : QDir::cleanPath( _workingDir.absoluteFilePath( path ) );
	}
	QString getRelativePath( const QString & path ) const
	{
		// Gets rid of any redundant "/./" or "/../" in the middle.
		// Works even if path is already relative.
		return path.isEmpty() ? QString() : _workingDir.relativeFilePath( path );
	}
	QString convertPath( const QString & path ) const
	{
		return usingAbsolutePaths() ? getAbsolutePath( path ) : getRelativePath( path );
	}

};


/** Helper that allows rebasing paths from one base directory to another. */
class PathRebaser {

	QDir _origBaseDir;    ///< original base dir for the relative input paths
	QDir _targetBaseDir;   ///< target base dir for the relative output paths
	PathStyle _outPathStyle;   ///< whether the output paths should be relative or absolute
	bool _quotePaths;  ///< whether to surround all output paths with quotes (needed when generating a batch)
	                   // !!IMPORTANT!! Never store the quoted paths and pass them back to PathConvertor, they are output-only.
 public:

	PathRebaser( const QDir & origBaseDir, const QDir & targetBaseDir, PathStyle outPathStyle, bool quotePaths = false )
		: _origBaseDir( origBaseDir ), _targetBaseDir( targetBaseDir ), _outPathStyle( outPathStyle ), _quotePaths( quotePaths ) {}

	PathRebaser( const PathRebaser & other ) = default;
	PathRebaser( PathRebaser && other ) = default;
	PathRebaser & operator=( const PathRebaser & other ) = default;
	PathRebaser & operator=( PathRebaser && other ) = default;

	const QDir & origBaseDir() const                   { return _origBaseDir; }
	const QDir & targetBaseDir() const                 { return _targetBaseDir; }
	PathStyle outputPathStyle() const                  { return _outPathStyle; }
	bool outputAbsolutePaths() const                   { return _outPathStyle == PathStyle::Absolute; }
	bool quotePaths() const                            { return _quotePaths; }

	void setOrigBaseDir( const QDir & baseDir )        { _origBaseDir = baseDir; }
	void setTargetBaseDir( const QDir & baseDir )      { _targetBaseDir = baseDir; }
	void setOutputPathStyle( PathStyle pathStyle )     { _outPathStyle = pathStyle; }

	QString maybeQuoted( const QString & path ) const
	{
		return _quotePaths ? quoted( path ) : path;
	}

	/// Rebases a path relative to the original base directory to be relative to the target base directory,
	/// absolute path is kept as is.
	QString rebase( const QString & path ) const
	{
		return rebaseFromTo( path, _origBaseDir, _targetBaseDir );
	}

	/// Rebases a path relative to the target base directory back to relative to the original base directory,
	/// absolute path is kept as is.
	QString rebaseBack( const QString & path ) const
	{
		return rebaseFromTo( path, _targetBaseDir, _origBaseDir );
	}

	/// Converts a path that is either absolute or relative to the original base directory
	/// to a path either absolute or relative to the target base directory (based on the output path style).
	QString rebaseAndConvert( const QString & path ) const
	{
		return convertAndRebaseFromTo( path, _origBaseDir, _targetBaseDir );
	}

	/// Converts a path that is either absolute or relative to the target base directory
	/// to a path either absolute or relative to the original base directory (based on the output path style).
	QString rebaseBackAndConvert( const QString & path ) const
	{
		return convertAndRebaseFromTo( path, _targetBaseDir, _origBaseDir );
	}

	/// Converts a path to a path relative to the target base directory.
	QString rebaseAndMakeRelative( const QString & path ) const
	{
		if (fs::isAbsolutePath( path ))
			return _targetBaseDir.relativeFilePath( path );
		else
			return rebase( path );
	}

	/// Converts a path relative to the target base directory to an absolute path.
	QString rebaseBackAndMakeAbsolute( const QString & rebasedPath ) const
	{
		return !rebasedPath.isEmpty() ? _targetBaseDir.absoluteFilePath( rebasedPath ) : QString();
	}

	/// Takes a Qt internal path and outputs a path suitable for an OS shell command.
	QString makeCmdPath( const QString & path ) const
	{
		return maybeQuoted( fs::toNativePath( path ) );
	}

	/// Takes a Qt internal path and outputs an always quoted path suitable for an OS shell command.
	QString makeQuotedCmdPath( const QString & path ) const
	{
		return quoted( fs::toNativePath( path ) );
	}

	/// Takes a Qt internal path and outputs a rebased path suitable for an OS shell command.
	/** If the path is relative, it is rebased to the target base dir, otherwise it's kept absolute. */
	QString makeRebasedCmdPath( const QString & path ) const
	{
		return maybeQuoted( fs::toNativePath( rebase( path ) ) );
	}

	/// Takes a Qt internal path and outputs a rebased relative path suitable for an OS shell command.
	QString makeRebasedRelativeCmdPath( const QString & path ) const
	{
		return maybeQuoted( fs::toNativePath( rebaseAndMakeRelative( path ) ) );
	}

 private:

	QString rebaseFromTo( const QString & path, const QDir & inputBaseDir, const QDir & outputBaseDir ) const
	{
		if (path.isEmpty() || fs::isAbsolutePath( path ))
			return path;

		QString absPath = inputBaseDir.absoluteFilePath( path );
		QString newPath = outputBaseDir.relativeFilePath( absPath );

		return newPath;
	}

	QString convertAndRebaseFromTo( const QString & path, const QDir & inputBaseDir, const QDir & outputBaseDir ) const
	{
		if (path.isEmpty())
			return {};

		QString absPath = fs::isAbsolutePath( path ) ? path : inputBaseDir.absoluteFilePath( path );
		QString newPath = outputAbsolutePaths() ? absPath : outputBaseDir.relativeFilePath( absPath );

		return newPath;
	}

};


#endif // FILE_SYSTEM_UTILS_INCLUDED
