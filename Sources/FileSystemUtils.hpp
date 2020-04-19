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

class DirTreeModel;
class QModelIndex;

#include <QString>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>


//======================================================================================================================
/** helper for calculating relative and absolute paths according to current directory and settings */

class PathHelper {

	QDir _baseDir;  ///< directory which relative paths are relative to
	QDir _prevBaseDir;  ///< original base dir for rebasing paths to another base
	bool _useAbsolutePaths;  ///< whether to store paths to engines, IWADs, maps and mods in absolute or relative form

 public:

	PathHelper( bool useAbsolutePaths, const QDir & baseDir, const QDir & prevBaseDir = QDir() )
		: _baseDir( baseDir ), _prevBaseDir( prevBaseDir ), _useAbsolutePaths( useAbsolutePaths ) {}

	PathHelper( const PathHelper & other )
		: _baseDir( other._baseDir ), _prevBaseDir( other._prevBaseDir ), _useAbsolutePaths( other._useAbsolutePaths ) {}

	void operator=( const PathHelper & other )
		{ _baseDir = other._baseDir; _prevBaseDir = other._prevBaseDir; _useAbsolutePaths = other._useAbsolutePaths; }

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
		QString absPath = _prevBaseDir.filePath( path );
		return _baseDir.relativeFilePath( absPath );
	}

};


//======================================================================================================================
//  misc helper functions

template< typename Item >
void fillListFromDir( QList< Item > & list, const QString & dir, bool recursively, const PathHelper & pathHelper,
                      std::function< bool ( const QFileInfo & file ) > isDesiredFile )
{
	QDir dir_( dir );
	if (!dir_.exists())
		return;

	QDirIterator dirIt( dir_ );
	while (dirIt.hasNext()) {
		QString entryPath = pathHelper.convertPath( dirIt.next() );
		QFileInfo entry( entryPath );
		if (entry.isDir()) {
			QString dirName = dirIt.fileName();  // we need the original entry name including "." and "..", entry is already converted
			if (recursively && dirName != "." && dirName != "..") {
				fillListFromDir( list, entry.filePath(), recursively, pathHelper, isDesiredFile );
			}
		} else {
			if (isDesiredFile( entry )) {
				list.append( Item( entry ) );
			}
		}
	}
}

void fillTreeFromDir( DirTreeModel & model, const QModelIndex & parent, const QString & dir, const PathHelper & pathHelper,
                      std::function< bool ( const QFileInfo & file ) > isDesiredFile );


#endif // FILE_SYSTEM_UTILS_INCLUDED
