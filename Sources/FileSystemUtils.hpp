//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  5.4.2020
// Description: miscellaneous utilities
//======================================================================================================================

#ifndef MISC_UTILS_INCLUDED
#define MISC_UTILS_INCLUDED


#include "Common.hpp"

#include <QString>
#include <QDir>
#include <QFileInfo>


//======================================================================================================================
/** helper for calculating relative and absolute paths according to current directory and settings */

class PathHelper {

	QDir _baseDir;  ///< directory which relative paths are relative to
	QDir _currentDir;  ///< cached current directory - original base dir for path rebasing
	bool _useAbsolutePaths;  ///< whether to store paths to engines, IWADs, maps and mods in absolute or relative form

 public:

	PathHelper( bool useAbsolutePaths, const QDir & baseDir )
		: _baseDir( baseDir ), _currentDir( QDir::current() ), _useAbsolutePaths( useAbsolutePaths ) {}

	PathHelper( const PathHelper & other )
		: _baseDir( other._baseDir ), _currentDir( other._currentDir ), _useAbsolutePaths( other._useAbsolutePaths ) {}

	void operator=( const PathHelper & other )
		{ _baseDir = other._baseDir; _currentDir = other._currentDir; _useAbsolutePaths = other._useAbsolutePaths; }

	const QDir & baseDir() const                       { return _baseDir; }
	bool useAbsolutePaths() const                      { return _useAbsolutePaths; }
	bool useRelativePaths() const                      { return !_useAbsolutePaths; }

	void toggleAbsolutePaths( bool useAbsolutePaths )  { _useAbsolutePaths = useAbsolutePaths; }
	void setBaseDir( const QDir & baseDir )            { _baseDir = baseDir; }

	QString getAbsolutePath( const QString & path ) const
	{
		return path.isEmpty() ? "" : QFileInfo( _baseDir, path ).absoluteFilePath();
	}
	QString getRelativePath( const QString & path ) const
	{
		return path.isEmpty() ? "" : _baseDir.relativeFilePath( path );
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
		QString absPath = _currentDir.filePath( path );
		return _baseDir.relativeFilePath( absPath );
	}

};


#endif // MISC_UTILS_INCLUDED
