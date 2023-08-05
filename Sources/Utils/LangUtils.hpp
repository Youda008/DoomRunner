//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: general language-specific helper functions and classes
//======================================================================================================================

#ifndef LANG_UTILS_INCLUDED
#define LANG_UTILS_INCLUDED


#include "Essential.hpp"

#include <algorithm>


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
