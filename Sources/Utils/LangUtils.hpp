//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: general language-specific helper functions
//======================================================================================================================

#ifndef LANG_UTILS_INCLUDED
#define LANG_UTILS_INCLUDED


#include "Common.hpp"

#include <algorithm>


//======================================================================================================================
//  utils for Qt containers

template< typename Container1, typename Container2 >
bool equal( const Container1 & cont1, const Container2 & cont2 )
{
	return std::equal( std::begin(cont1), std::end(cont1), std::begin(cont2), std::end(cont2) );
}

template< typename Container, typename Element >
bool contains( const Container & cont, const Element & elem )
{
	return std::find( std::begin(cont), std::end(cont), elem ) != std::end(cont);
}

template< typename Container, typename Condition >
bool containsSuch( const Container & cont, Condition condition )
{
	return std::find_if( std::begin(cont), std::end(cont), condition ) != std::end(cont);
}

// own implementation because Qt works with int indexes for position instead of iterators like std

template< typename Container, typename Element >
int find( const Container & cont, const Element & elem )
{
	int i = 0;
	for (const auto & elem2 : cont)
	{
		if (elem == elem2)
			return i;
		i++;
	}
	return -1;
}

template< typename Container, typename Condition >
int findSuch( const Container & list, Condition condition )
{
	int i = 0;
	for (const auto & elem : list)
	{
		if (condition( elem ))
			return i;
		i++;
	}
	return -1;
}

template< typename Container >
void reverse( Container & cont )
{
	std::reverse( std::begin(cont), std::end(cont) );
}


//======================================================================================================================
//  scope guards

template< typename EndFunc >
class ScopeGuard
{
	EndFunc _atEnd;
 public:
	ScopeGuard( const EndFunc & endFunc ) : _atEnd( endFunc ) {}
	ScopeGuard( EndFunc && endFunc ) : _atEnd( std::move(endFunc) ) {}
	~ScopeGuard() noexcept { _atEnd(); }
};

template< typename EndFunc >
ScopeGuard< EndFunc > atScopeEndDo( const EndFunc & endFunc )
{
	return ScopeGuard< EndFunc >( endFunc );
}

template< typename EndFunc >
ScopeGuard< EndFunc > atScopeEndDo( EndFunc && endFunc )
{
	return ScopeGuard< EndFunc >( std::move(endFunc) );
}


//======================================================================================================================
//  value matching

template< typename Source, typename Result >
struct CorrespondingPair
{
	Source possibleValue;
	Result correspondingResult;
};

template< typename Source, typename Result >
CorrespondingPair< Source, Result > corresponds( Source source, Result result )
{
	return { source, result };
}

template< typename Source, typename Result, typename ... Args >
Result correspondingValue( Source source, CorrespondingPair< Source, Result > && lastPair )
{
	if (source == lastPair.possibleValue)
		return std::move( lastPair.correspondingResult );
	else
		return Result();
}

template< typename Source, typename Result, typename ... Args >
Result correspondingValue( Source source, CorrespondingPair< Source, Result > && firstPair, Args && ... otherPairs )
{
	if (source == firstPair.possibleValue)
		return std::move( firstPair.correspondingResult );
	else
		return correspondingValue( source, std::move( otherPairs ) ... );
}


//======================================================================================================================
//  other

// just to be little more explicit when needed
template< typename Type >
bool isSet( const Type & obj )
{
	return static_cast< bool >( obj );
}


#endif // LANG_UTILS_INCLUDED
