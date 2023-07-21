//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: general language-specific helper functions
//======================================================================================================================

#ifndef LANG_UTILS_INCLUDED
#define LANG_UTILS_INCLUDED


#include "Essential.hpp"

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
//  span

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


//======================================================================================================================
//  scope guards

template< typename EndFunc >
class ScopeGuard
{
	EndFunc _atEnd;
 public:
	ScopeGuard( EndFunc endFunc ) : _atEnd( std::move(endFunc) ) {}
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

template< typename Handle, typename CloseFunc >
class AutoClosable
{
	Handle _handle;
	CloseFunc _closeFunc;

 public:

	AutoClosable( Handle handle, CloseFunc closeFunc ) : _handle( handle ), _closeFunc( closeFunc ) {}
	AutoClosable( const AutoClosable & other ) = delete;

	void letGo()
	{
		_handle = Handle(0);
		_closeFunc = nullptr;
	}

	AutoClosable( AutoClosable && other )
	{
		_handle = other._handle;
		_closeFunc = other._closeFunc;
		other.letGo();
	}

	~AutoClosable()
	{
		if (_closeFunc)
			_closeFunc( _handle );
	}
};

template< typename Handle, typename CloseFunc >
AutoClosable< Handle, CloseFunc > autoClosable( Handle handle, CloseFunc closeFunc )
{
	return AutoClosable< Handle, CloseFunc >( handle, closeFunc );
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

template< typename Type >
Type * optToPtr( std::optional< Type > & opt )
{
	return opt.has_value() ? &opt.value() : nullptr;
}
template< typename Type >
const Type * optToPtr( const std::optional< Type > & opt )
{
	return opt.has_value() ? &opt.value() : nullptr;
}

template< typename Type >
Type & unconst( const Type & obj ) noexcept
{
	return const_cast< std::remove_const_t< Type > & >( obj );
}
template< typename Type >
Type * unconst( const Type * obj ) noexcept
{
	return const_cast< std::remove_const_t< Type > * >( obj );
}

template< typename Float, std::enable_if_t< std::is_floating_point_v<Float>, int > = 0 >
bool isFloatEqual( Float a, Float b )
{
	Float diff = a - b;
	return diff > 0.0001 && diff < 0.0001;
}


#endif // LANG_UTILS_INCLUDED
