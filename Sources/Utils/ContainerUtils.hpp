//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: generic container helpers
//======================================================================================================================

#ifndef CONTAINER_UTILS_INCLUDED
#define CONTAINER_UTILS_INCLUDED


#include "Essential.hpp"

#include <algorithm>


//======================================================================================================================
// utils for Qt containers

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

/** Convenience wrapper around iterator to container of pointers
  * that skips the additional needed dereference and returns a reference directly. */
template< typename IterType >
class PointerIterator
{
	IterType _origIter;
 public:
	PointerIterator( const IterType & origIter ) : _origIter( origIter ) {}
	auto operator*() -> decltype( **_origIter ) const { return **_origIter; }
	auto operator->() -> decltype( *_origIter ) const { return *_origIter; }
	PointerIterator & operator++() { ++_origIter; return *this; }
	PointerIterator operator++(int) { auto tmp = *this; ++_origIter; return tmp; }
	friend bool operator==( const PointerIterator & a, const PointerIterator & b ) { return a._origIter == b._origIter; }
	friend bool operator!=( const PointerIterator & a, const PointerIterator & b ) { return a._origIter != b._origIter; }
};


//======================================================================================================================
// span

template< typename Type >
class span
{
	Type * _begin;
	Type * _end;

 public:

	span() : _begin( nullptr ), _end( nullptr ) {}
	span( Type * begin, Type * end ) : _begin( begin ), _end( end ) {}
	span( Type * data, int size ) : _begin( data ), _end( data + size ) {}
	span( Type * data, size_t size ) : _begin( data ), _end( data + size ) {}

	Type * begin() const                   { return _begin; }
	Type * end() const                     { return _end; }
	Type * data() const                    { return _begin; }
	size_t size() const                    { return _end - _begin; }
	bool empty() const                     { return _begin == _end; }
	Type & operator[]( int index ) const   { return _begin[ index ]; }
};


#endif // CONTAINER_UTILS_INCLUDED
