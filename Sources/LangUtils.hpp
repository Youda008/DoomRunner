//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  13.5.2019
// Description: general helper functions
//======================================================================================================================

#ifndef LANG_UTILS_INCLUDED
#define LANG_UTILS_INCLUDED


#include "Common.hpp"

#include <QVector>
#include <QList>
#include <QString>
#include <QDir>
#include <QFileInfo>

#include <functional>


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


#endif // LANG_UTILS_INCLUDED
