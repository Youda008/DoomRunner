//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  13.5.2019
// Description: general utilities
//======================================================================================================================

#ifndef UTILS_INCLUDED
#define UTILS_INCLUDED


#include "Common.hpp"

#include <QString>
#include <QDir>
#include <QFileInfo>


//======================================================================================================================
//  container helpers

// Because it's too annoying having to specify the container manually every call like this:
//   containsSuch< QList<Elem>, Elem >( list, []( ... ){ ... } )
// and because it's not nice to copy-implement this for every container, we implement it once in container-generic way
// and provide simple delegating wrappers for the specific containers

template< typename Container, typename ElemType >
bool _containsSuch( const Container & list, std::function< bool ( const ElemType & elem ) > condition )
{
	for (const ElemType & elem : list)
		if (condition( elem ))
			return true;
	return false;
}

template< typename Container, typename ElemType >
int _findSuch( const Container & list, std::function< bool ( const ElemType & elem ) > condition )
{
	int i = 0;
	for (const ElemType & elem : list) {
		if (condition( elem ))
			return i;
		i++;
	}
	return -1;
}

/** checks whether the vector contains such an element that satisfies condition */
template< typename ElemType >
bool containsSuch( const QVector< ElemType > & list, std::function< bool ( const ElemType & elem ) > condition )
{
	return _containsSuch< QList< ElemType >, ElemType >( list, condition );
}
/** checks whether the list contains such an element that satisfies condition */
template< typename ElemType >
bool containsSuch( const QList< ElemType > & list, std::function< bool ( const ElemType & elem ) > condition )
{
	return _containsSuch< QList< ElemType >, ElemType >( list, condition );
}

/** finds such element in the vector that satisfies condition */
template< typename ElemType >
int findSuch( const QVector< ElemType > & list, std::function< bool ( const ElemType & elem ) > condition )
{
	return _findSuch< QVector< ElemType >, ElemType >( list, condition );
}
/** finds such element in the list that satisfies condition */
template< typename ElemType >
int findSuch( const QList< ElemType > & list, std::function< bool ( const ElemType & elem ) > condition )
{
	return _findSuch< QList< ElemType >, ElemType >( list, condition );
}


template< typename Indexable >
void reverse( Indexable & container )
{
	const int count = container.count();
	for (int i = 0; i < count / 2; ++i)
		std::swap( container[ i ], container[ count - 1 - i ] );
}


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

	QString getAbsolutePath( QString path ) const
	{
		return path.isEmpty() ? "" : QFileInfo( _baseDir, path ).absoluteFilePath();
	}
	QString getRelativePath( QString path ) const
	{
		return path.isEmpty() ? "" : _baseDir.relativeFilePath( path );
	}
	QString convertPath( QString path ) const
	{
		return _useAbsolutePaths ? getAbsolutePath( path ) : getRelativePath( path );
	}

	QString rebasePath( QString path ) const
	{
		if (path.isEmpty())
			return {};
		if (QDir::isAbsolutePath( path ))
			return path;
		QString absPath = _currentDir.filePath( path );
		return _baseDir.relativeFilePath( absPath );
	}

};


//======================================================================================================================
//  misc

QString getMapNumber( QString mapName );

bool isDoom1( QString iwadName );


#endif // UTILS_INCLUDED
