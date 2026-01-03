//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: generic container helpers
//======================================================================================================================

#ifndef CONTAINER_UTILS_INCLUDED
#define CONTAINER_UTILS_INCLUDED


#include "Essential.hpp"

#include "CommonTypes.hpp"  // qsize_t
#include "TypeTraits.hpp"

#include <algorithm>
#include <iterator>
#include <cassert>


//======================================================================================================================
// utils for Qt containers

template< typename Range1, typename Range2 >
bool equal( const Range1 & range1, const Range2 & range2 )
{
	return std::equal( std::begin(range1), std::end(range1), std::begin(range2), std::end(range2) );
}

template< typename Range, typename Element >
bool contains( const Range & range, const Element & elem )
{
	return std::find( std::begin(range), std::end(range), elem ) != std::end(range);
}

template< typename Range, typename Condition >
bool containsSuch( const Range & range, Condition condition )
{
	return std::find_if( std::begin(range), std::end(range), condition ) != std::end(range);
}

// own implementation because Qt works with int indexes for position instead of iterators like std

template< typename Range, typename Element >
int find( const Range & range, const Element & elem )
{
	int i = 0;
	for (const auto & elem2 : range)
	{
		if (elem == elem2)
			return i;
		i++;
	}
	return -1;
}

template< typename Range, typename Condition >
int findSuch( const Range & range, Condition condition )
{
	int i = 0;
	for (const auto & elem : range)
	{
		if (condition( elem ))
			return i;
		i++;
	}
	return -1;
}

template< typename Range >
void reverse( Range & range )
{
	std::reverse( std::begin(range), std::end(range) );
}

/// Clears the content of the container and forces it to free its main buffer.
template< typename Container >
void clearAndDeallocate( Container & cont )
{
	Container empty;
	cont.swap( empty );
}

/// In most container implementations, this clears the content of the container but keeps the main buffer allocated.
template< typename Container >
void clearButKeepAllocated( Container & cont )
{
	cont.resize( 0 );
}

/** Shifts existing elements in the list towards the end,
  * so that there is \p count elements starting from index \p where which are moved from,
  * which means they are unusable, but ready to be assigned to. */
template< typename List >
void reserveSpace( List & list, const qsize_t where, const qsize_t count )
{
	const auto origSize = qsize_t( list.size() );
	assert( size_t( where ) <= size_t( origSize ) );
	assert( count > 0 );

	// allocate space for the new elements
	list.resize( origSize + count );

	// reserve space by shifting the existing elements count steps towards the end
	for (qsize_t idx = origSize - 1; idx >= where; idx--)
		list[ idx + count ] = std::move( list[ idx ] );
}

/// Inserts \p count copies of \p val to the list at index \p where.
/** The \p val is copied \p count - 1 times and then moved to the last slot. */
template< typename List, typename Value >
void insertCopies( List & list, const qsize_t where, const qsize_t count, Value val )
{
	reserveSpace( list, where, count );

	for (qsize_t i = 0; i < count - 1; i++)
		list[ where + i ] = val;  // copy the val to all but the last slot
	list[ where + count - 1 ] = std::move( val );  // then move the original val to the last slot to save 1 copy
}

/** Inserts elements from \p range into the \p list at index \p where.
  * If the \p range is an rvalue, the elements are moved from the range. If it's an lvalue, the elements are copied. */
template< typename List, typename Range >
void insertMultiple( List & list, qsize_t where, Range && range )
{
	using Elem = types::range_element< Range >;
	const auto rangeSize = qsize_t( std::size( range ) );

	reserveSpace( list, where, rangeSize );

	for (auto && elem : range)
	{
		list[ where ] = std::forward< Elem >( elem );
		where++;
	}
}

template< typename List >
void removeCountAt( List & list, const qsize_t from, const qsize_t count )
{
	const auto listSize = qsize_t( list.size() );
	[[maybe_unused]] const auto removeRangeBeg = from;
	[[maybe_unused]] const auto removeRangeEnd = from + count;
	assert( removeRangeBeg < removeRangeEnd );     // the remove range is valid
	assert( removeRangeBeg >= 0 );                 // beginning of the remove range is within the list range
	assert( removeRangeEnd <= listSize );          //       end of the remove range is within the list range

	// shift the existing elements <count> steps towards the beginning
	for (qsize_t idx = removeRangeEnd; idx < listSize; idx++)
		list[ idx - count ] = std::move( list[ idx ] );

	// cut the last empty elements from the list
	list.resize( list.size() - count );
}


//======================================================================================================================
// span and range

template< typename Elem >
class span
{
	Elem * _begin;
	Elem * _end;

 public:

	span() : _begin( nullptr ), _end( nullptr ) {}
	span( Elem * begin, Elem * end ) : _begin( begin ), _end( end ) {}
	span( Elem * data, qsize_t size ) : _begin( data ), _end( data + size ) {}
	span( Elem * data, size_t size ) : _begin( data ), _end( data + size ) {}

	Elem * begin() const                         { return _begin; }
	Elem * end() const                           { return _end; }
	Elem * data() const                          { return _begin; }
	qsize_t size() const                         { return _end - _begin; }
	bool empty() const                           { return _begin == _end; }
	Elem & operator[]( qsize_t index ) const     { return _begin[ index ]; }
};

template< typename Iter >
class subrange
{
	Iter _begin;
	Iter _end;

 public:

	subrange() : _begin(), _end() {}
	subrange( Iter begin, Iter end ) : _begin( begin ), _end( end ) {}

	const Iter & begin() const                   { return _begin; }
	const Iter & end() const                     { return _end; }
	const Iter & cbegin() const                  { return _begin; }
	const Iter & cend() const                    { return _end; }

	qsize_t size() const                         { return _end - _begin; }
	bool empty() const                           { return _begin == _end; }
};

template< typename Iter >
subrange( Iter begin, Iter end ) -> subrange< Iter >;


//======================================================================================================================
// sequence generation

template< typename Type >
QList< Type > makeSequence( const Type & first, size_t count )
{
	QList< Type > sequence;
	sequence.reserve( count );

	Type current = first;
	while (count-- > 0)
	{
		sequence.append( current );
		++current;
	}

	return sequence;
}


//======================================================================================================================
// miscellaneous

template< typename Type >
struct IndexValue
{
	int index;
	Type value;
};

template< typename Type >
struct IndexValueRef
{
	int index;
	Type & value;
};


//======================================================================================================================


#endif // CONTAINER_UTILS_INCLUDED
