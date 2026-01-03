//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: general language-specific helper functions and classes
//======================================================================================================================

#ifndef LANG_UTILS_INCLUDED
#define LANG_UTILS_INCLUDED


#include "Essential.hpp"

#include "TypeTraits.hpp"

#include <algorithm>
#include <optional>


//======================================================================================================================
// utils from the standard library of a newer C++ standard

namespace fut {

// C++23
template< typename Enum >
constexpr auto to_underlying( Enum e ) noexcept
{
	return static_cast< std::underlying_type_t< Enum > >( e );
}

} // namespace fut


//======================================================================================================================
// general utility functions

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

template< typename Float, std::enable_if_t< std::is_floating_point_v<Float>, int > = 0 >
bool isFloatEqual( Float a, Float b )
{
	Float diff = a - b;
	return diff > 0.0001 && diff < 0.0001;
}

template< typename Type >
Type takeAndReplace( Type & variable, Type newVal )
{
	Type oldVal = std::move( variable );
	variable = std::move( newVal );
	return oldVal;
}


//======================================================================================================================
// flag utils

template< typename Flag >
bool isFlagSet( std::underlying_type_t< Flag > targetFlags, Flag flagToTest )
{
	return (targetFlags & flagToTest) != 0;
}

template< typename Flags >
bool isAnyOfFlagsSet( Flags targetFlags, types::identity< Flags > flagsToTest )
{
	return (targetFlags & flagsToTest) != 0;
}

template< typename Flags >
bool areAllFlagsSet( Flags targetFlags, types::identity< Flags > flagsToTest )
{
	return (targetFlags & flagsToTest) == flagsToTest;
}

template< typename Flags >
Flags withAddedFlags( Flags origFlags, types::identity< Flags > flagsToAdd )
{
	return origFlags | flagsToAdd;
}

template< typename Flags >
Flags withoutFlags( Flags origFlags, types::identity< Flags > flagsToRemove )
{
	return origFlags & ~flagsToRemove;
}

template< typename Flags >
Flags withToggledFlags( Flags origFlags, types::identity< Flags > flagsToSwitch, bool enabled )
{
	if (enabled)
		return withAddedFlags( origFlags, flagsToSwitch );
	else
		return withoutFlags( origFlags, flagsToSwitch );
}

template< typename Flags >
Flags withFlippedFlags( Flags origFlags, types::identity< Flags > flagsToFlip )
{
	return (origFlags & ~flagsToFlip) | ((origFlags & flagsToFlip) ^ flagsToFlip);
}

template< typename Flags >
void setFlags( Flags & targetFlags, types::identity< Flags > flagsToSet )
{
	targetFlags |= flagsToSet;
}

template< typename Flags >
void unsetFlags( Flags & targetFlags, types::identity< Flags > flagsToUnset )
{
	targetFlags &= ~flagsToUnset;
}

template< typename Flags >
void toggleFlags( Flags & targetFlags, types::identity< Flags > flagsToSwitch, bool enabled )
{
	targetFlags = withToggledFlags( targetFlags, flagsToSwitch, enabled );
}

template< typename Flags >
void flipFlags( Flags & targetFlags, types::identity< Flags > flagsToFlip )
{
	targetFlags = withFlippedFlags( targetFlags, flagsToFlip );
}

template< typename Flags = uint >
constexpr Flags makeBitMask( uint numOfBits )
{
	return (1 << numOfBits) - 1;
}


//======================================================================================================================
// scope guards

template< typename EndFunc >
class ScopeGuard
{
	EndFunc _atEnd;
 public:
	ScopeGuard( EndFunc && endFunc ) : _atEnd( std::forward< EndFunc >( endFunc ) ) {}
	~ScopeGuard() noexcept { _atEnd(); }
};

template< typename EndFunc >
ScopeGuard< EndFunc > atScopeEndDo( EndFunc && endFunc )
{
	return ScopeGuard< EndFunc >( std::forward< EndFunc >( endFunc ) );
}

template< typename EndFunc >
class DismissableScopeGuard
{
	EndFunc _atEnd;
	bool _active;
 public:
	DismissableScopeGuard( EndFunc endFunc ) : _atEnd( std::forward< EndFunc >( endFunc ) ), _active( true ) {}
	void dismiss() { _active = false; }
	~DismissableScopeGuard() noexcept { if (_active) _atEnd(); }
};

template< typename EndFunc >
DismissableScopeGuard< EndFunc > atScopeEndMaybeDo( EndFunc && endFunc )
{
	return DismissableScopeGuard< EndFunc >( std::forward< EndFunc >( endFunc ) );
}

template< typename Handle, typename CloseFunc, fut::remove_cvref_t<Handle> InvalidHandle = nullptr >
class AutoClosable
{
	Handle _handle;
	CloseFunc _closeFunc;

 public:

	AutoClosable( Handle && handle, CloseFunc && closeFunc )
	    : _handle( std::forward<Handle>(handle) ), _closeFunc( std::forward<CloseFunc>(closeFunc) ) {}
	AutoClosable( const AutoClosable & other ) = delete;

	void dismiss()
	{
		_handle = InvalidHandle;
		_closeFunc = {};
	}

	AutoClosable( AutoClosable && other )
	{
		_handle = other._handle;
		_closeFunc = other._closeFunc;
		other.dismiss();
	}

	~AutoClosable()
	{
		if (_closeFunc)
			_closeFunc( _handle );
	}
};

template< typename Handle, typename CloseFunc, fut::remove_cvref_t<Handle> InvalidHandle = nullptr >
AutoClosable< Handle, CloseFunc > autoClosable( Handle && handle, CloseFunc && closeFunc )
{
	return AutoClosable< Handle, CloseFunc, InvalidHandle >(
		std::forward<Handle>(handle), std::forward<CloseFunc>(closeFunc)
	);
}


//======================================================================================================================
// reporting errors via return values

/// Represents either a return value or an error that prevented returning a valid value.
/** Basically an enhanced std::optional with details why the value is not present.
  * Also a substitution for std::expected from C++23. */
template< typename Value, typename Error, Error successErrorValue = 0 >
class ValueOrError {

	Value _val;
	Error _err;

 public:

	ValueOrError( Value val ) : _val( std::move(val) ), _err( successErrorValue ) {}
	ValueOrError( Error err ) : _val(), _err( std::move(err) ) {}
	ValueOrError( Value val, Error err ) : _val( std::move(val) ), _err( std::move(err) ) {}

	ValueOrError( const ValueOrError & other ) = default;
	ValueOrError( ValueOrError && other ) = default;
	ValueOrError & operator=( const ValueOrError & other ) = default;
	ValueOrError & operator=( ValueOrError && other ) = default;

	      Value & value()             { return _val; }
	const Value & value() const       { return _val; }
	      Error & error()             { return _err; }
	const Error & error() const       { return _err; }

	bool isSuccess() const            { return _err == successErrorValue; }
	operator bool() const             { return isSuccess(); }

	      Value & operator*()         { return _val; }
	const Value & operator*() const   { return _val; }
	      Value * operator->()        { return &_val; }
	const Value * operator->() const  { return &_val; }

};


//======================================================================================================================
// value matching

template< typename Source, typename Result >
struct CorrespondingPair
{
	Source possibleValue;
	Result correspondingResult;
};

template< typename Source, typename Result >
CorrespondingPair< Source, Result > correspondsTo( Source source, Result result )
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


#endif // LANG_UTILS_INCLUDED
