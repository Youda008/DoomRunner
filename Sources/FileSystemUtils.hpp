//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  5.4.2020
// Description: utilities concerning paths, directories and files
//======================================================================================================================

#ifndef FILE_SYSTEM_UTILS_INCLUDED
#define FILE_SYSTEM_UTILS_INCLUDED


#include "Common.hpp"

class QModelIndex;

#include <QString>
#include <QDir>
#include <QFileInfo>

#include <functional>


//======================================================================================================================
/** helper for calculating relative and absolute paths according to current directory and settings */

class PathContext {

	QDir _baseDir;  ///< directory which relative paths are relative to
	QDir _prevBaseDir;  ///< original base dir for rebasing paths to another base
	bool _useAbsolutePaths;  ///< whether to store paths to engines, IWADs, maps and mods in absolute or relative form

 public:

	PathContext( bool useAbsolutePaths, const QDir & baseDir, const QDir & prevBaseDir = QDir() )
		: _baseDir( baseDir ), _prevBaseDir( prevBaseDir ), _useAbsolutePaths( useAbsolutePaths ) {}

	PathContext( const PathContext & other )
		: _baseDir( other._baseDir ), _prevBaseDir( other._prevBaseDir ), _useAbsolutePaths( other._useAbsolutePaths ) {}

	void operator=( const PathContext & other )
		{ _baseDir = other._baseDir; _prevBaseDir = other._prevBaseDir; _useAbsolutePaths = other._useAbsolutePaths; }

	const QDir & baseDir() const                       { return _baseDir; }
	bool useAbsolutePaths() const                      { return _useAbsolutePaths; }
	bool useRelativePaths() const                      { return !_useAbsolutePaths; }

	void toggleAbsolutePaths( bool useAbsolutePaths )  { _useAbsolutePaths = useAbsolutePaths; }
	void setBaseDir( const QDir & baseDir )            { _baseDir = baseDir; }

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
		return _useAbsolutePaths ? getAbsolutePath( path ) : getRelativePath( path );
	}

	QString rebasePath( const QString & path ) const
	{
		if (path.isEmpty())
			return {};
		if (QDir::isAbsolutePath( path ))
			return path;
		QString absPath = _prevBaseDir.filePath( path );
		return _baseDir.relativeFilePath( absPath );
	}

};


//======================================================================================================================
//  misc helper functions

inline bool isValidDir( const QString & dirPath )
{
	return !dirPath.isEmpty() && QDir( dirPath ).exists();
}

inline QString getPathFromFileName( const QString & dirPath, const QString & fileName )
{
	return QDir( dirPath ).filePath( fileName );
}

inline QString getFileNameFromPath( const QString & filePath )
{
	return QFileInfo( filePath ).fileName();
}

inline QString getDirOfFile( const QString & filePath )
{
	return QFileInfo( filePath ).path();
}

inline QString getDirnameOfFile( const QString & filePath )
{
	return QFileInfo( filePath ).dir().dirName();
}

inline bool isInsideDir( const QString & entryPath, const QDir & dir )
{
	QFileInfo entry( entryPath );
	return entry.absoluteFilePath().startsWith( dir.absolutePath() );
}

QString updateFile( const QString & filePath, const QByteArray & newContent );

/// Opens a directory of a file in a new File Explorer window
bool openFileLocation( const QString & filePath );

/// Creates a file filter for the QFileDialog::getOpenFileNames
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
