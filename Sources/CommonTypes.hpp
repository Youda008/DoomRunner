//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: commonly used types and type aliases
//======================================================================================================================

#ifndef COMMON_TYPES_INCLUDED
#define COMMON_TYPES_INCLUDED


#include "Essential.hpp"

#include "Utils/TypeTraits.hpp"      // maybe_add_const, ...
#include "Utils/ContainerUtils.hpp"  // reserveSpace, insertCopies, insertMultiple, removeCountAt

#include <QVector>


//======================================================================================================================
// PtrList and related

/// Wrapper around iterator to container of pointers that skips the additional needed dereference and returns a reference directly
template< typename WrappedIter >
class DerefIterator
{
	WrappedIter wrappedIter;

	// the pointer the wrapped iterator directly points to
	using underlying_pointer_type = typename std::remove_reference< decltype( *wrappedIter ) >::type;
	// the element the underlying pointer points to
	using underlying_element_type = typename std::remove_reference< decltype( **wrappedIter ) >::type;

 public:

	using iterator_category = types::iterator_category< WrappedIter >;
	using difference_type = types::difference_type< WrappedIter >;
	using value_type = std::remove_cv_t< underlying_element_type >;
	using element_type = types::maybe_add_const< std::is_const_v< underlying_pointer_type >, underlying_element_type >;
	using pointer = value_type *;
	using reference = value_type &;

	DerefIterator( const WrappedIter & origIter ) : wrappedIter( origIter ) {}
	DerefIterator( WrappedIter && origIter ) : wrappedIter( std::move(origIter) ) {}

	element_type & operator*() const  { return **wrappedIter; }
	element_type * operator->() const  { return *wrappedIter; }

	DerefIterator & operator++()    { ++wrappedIter; return *this; }
	DerefIterator operator++(int)   { auto tmp = *this; ++wrappedIter; return tmp; }
	DerefIterator & operator--()    { --wrappedIter; return *this; }
	DerefIterator operator--(int)   { auto tmp = *this; --wrappedIter; return tmp; }
	friend bool operator==( const DerefIterator & a, const DerefIterator & b )  { return a.wrappedIter == b.wrappedIter; }
	friend bool operator!=( const DerefIterator & a, const DerefIterator & b )  { return a.wrappedIter != b.wrappedIter; }
	friend bool operator< ( const DerefIterator & a, const DerefIterator & b )  { return a.wrappedIter <  b.wrappedIter; }
	friend bool operator> ( const DerefIterator & a, const DerefIterator & b )  { return a.wrappedIter >  b.wrappedIter; }
	friend auto operator- ( const DerefIterator & a, const DerefIterator & b )  { return a.wrappedIter -  b.wrappedIter; }
	friend auto operator+ ( const DerefIterator & a, difference_type b )        { return DerefIterator( a.wrappedIter + b ); }
	friend auto operator- ( const DerefIterator & a, difference_type b )        { return DerefIterator( a.wrappedIter - b ); }
};

// Extended unique_ptr that can be copied by allocating a new copy of the element and binding the new pointer to it.
template< typename Elem >
class DeepCopyableUniquePtr {

	std::unique_ptr< Elem > _ptr;

 public:

	DeepCopyableUniquePtr() = default;

	explicit DeepCopyableUniquePtr( Elem * newElem ) noexcept : _ptr( newElem ) {}

	// moves only the pointer, doesn't touch Elem, as expected
	DeepCopyableUniquePtr( DeepCopyableUniquePtr && other ) noexcept = default;
	DeepCopyableUniquePtr & operator=( DeepCopyableUniquePtr && other ) noexcept = default;
	DeepCopyableUniquePtr( std::unique_ptr< Elem > && uptr ) noexcept : _ptr( std::move(uptr) ) {}
	DeepCopyableUniquePtr & operator=( std::unique_ptr< Elem > && uptr ) noexcept { _ptr = std::move(uptr); return *this; }

	// makes a copy of the Elem itself, not the pointer to Elem
	DeepCopyableUniquePtr( const DeepCopyableUniquePtr & other )
	:
		_ptr( new Elem( *other ) )
	{}
	DeepCopyableUniquePtr & operator=( const DeepCopyableUniquePtr & other )
	{
		_ptr.reset( new Elem( *other ) );
		return *this;
	}

	template< typename ... Args >
	static DeepCopyableUniquePtr allocNew( Args && ... args )
	{
		return DeepCopyableUniquePtr( new Elem( std::forward< Args >( args ) ... ) );
	}

	Elem * get() const noexcept            { return _ptr.get(); }
	Elem & operator*()  const noexcept     { return _ptr.operator*(); }
	Elem * operator->()  const noexcept    { return _ptr.operator->(); }
	operator bool() const noexcept         { return static_cast< bool >( _ptr ); }

	std::unique_ptr< Elem > to_unique_ptr() &&  { return std::move( _ptr ); }
};

/// Replacement for QList from Qt5 with some enhancements.
/** Stores pointers to elements internally, so that reallocation or moving the elements does not invalidate references. */
template< typename Elem >
class PtrList {

	// std::unique_ptr alone cannot be used in QVector, because internally QVector uses reference counting with copy-on-write,
	// which requires being able to make a copy of the element, when the container detaches due to a modification attempt.
	// With this pointer wrapper, when a shared instance needs to detach, it will copy all the elements
	// and create new pointers to them, which is exactly how the old QList behaved.
	QVector< DeepCopyableUniquePtr< Elem > > _list;

 public:

	using iterator = DerefIterator< typename decltype( _list )::iterator >;
	using const_iterator = DerefIterator< typename decltype( _list )::const_iterator >;

	// content access

	auto count() const                                 { return _list.count(); }
	auto size() const                                  { return _list.size(); }
	auto isEmpty() const                               { return _list.isEmpty(); }

	      Elem & operator[]( qsizetype idx )           { return *_list[ idx ]; }
	const Elem & operator[]( qsizetype idx ) const     { return *_list[ idx ]; }

	      iterator begin()                             { return { _list.begin() }; }
	const_iterator begin() const                       { return { _list.begin() }; }
	const_iterator cbegin() const                      { return { _list.cbegin() }; }
	      iterator end()                               { return { _list.end() }; }
	const_iterator end() const                         { return { _list.end() }; }
	const_iterator cend() const                        { return { _list.cend() }; }

	      Elem & first()                               { return *_list.first(); }
	const Elem & first() const                         { return *_list.first(); }
	      Elem & last()                                { return *_list.last(); }
	const Elem & last() const                          { return *_list.last(); }

	// list modification

	void reserve( qsizetype newSize )                  { _list.reserve( newSize ); }
	void resize( qsizetype newSize )
	{
		const auto oldSize = _list.size();
		_list.resize( newSize );
		// To behave equally as a normal list, we have to fill the new allocated space with default-constructed items,
		for (auto idx = oldSize; idx < newSize; idx++)          // otherwise assigning would deference a null pointer.
			_list[ idx ] = DeepCopyableUniquePtr< Elem >::allocNew();
	}

	void clear()                                       { _list.clear(); }

	void append( const Elem &  elem )                  { _list.append( DeepCopyableUniquePtr< Elem >::allocNew( elem ) ); }
	void append(       Elem && elem )                  { _list.append( DeepCopyableUniquePtr< Elem >::allocNew( std::move(elem) ) ); }
	void prepend( const Elem &  elem )                 { _list.prepend( DeepCopyableUniquePtr< Elem >::allocNew( elem ) ); }
	void prepend(       Elem && elem )                 { _list.prepend( DeepCopyableUniquePtr< Elem >::allocNew( std::move(elem) ) ); }
	void insert( qsizetype idx, const Elem &  elem )   { _list.insert( idx, DeepCopyableUniquePtr< Elem >::allocNew( elem ) ); }
	void insert( qsizetype idx,       Elem && elem )   { _list.insert( idx, DeepCopyableUniquePtr< Elem >::allocNew( std::move(elem) ) ); }

	void removeAt( qsizetype idx )                     { _list.removeAt( idx ); }

	void move( qsizetype from, qsizetype to )          { _list.move( from, to ); }

	// custom high-level operations

	template< typename Range, REQUIRES( types::is_range_of< Range, Elem > ) >
	void insertMultiple( qsizetype where, Range && range ) { ::insertMultiple( *this, where, std::forward< Range >( range ) ); }

	void removeCountAt( qsizetype idx, qsizetype cnt ) { ::removeCountAt( _list, idx, cnt ); }

	// low-level pointer manipulation for implementing optimized high-level operations

	/// Moves the pointer at \p idx out of the list, leaving null at its original position.
	std::unique_ptr< Elem > takePtr( qsizetype idx )
	{
		return std::move( _list[ idx ] ).to_unique_ptr();
	}

	/// Assigns the given pointer to position at \p idx, replacing the original pointer.
	/** If the original pointer is not null, the original item is deleted. */
	void assignPtr( qsizetype idx, std::unique_ptr< Elem > ptr )
	{
		_list[ idx ] = DeepCopyableUniquePtr< Elem >( std::move(ptr) );
	}

	/// Inserts \p count allocated and default-constructed elements to position at \p idx, shifting the existing pointers count steps towards the end.
	void insertDefaults( qsizetype where, qsizetype count )
	{
		::insertCopies( _list, where, count, DeepCopyableUniquePtr< Elem >::allocNew() );
	}

	/// Inserts the given pointers to position at \p idx, shifting the existing pointers ptrs.size() steps towards the end.
	template< typename PtrRange, REQUIRES( types::is_range_of< PtrRange, std::unique_ptr< Elem > > ) >
	void insertPtrs( qsizetype where, PtrRange && ptrs )
	{
		::insertMultiple( _list, where, std::forward< PtrRange >( ptrs ) );
	}

	bool isNull( qsizetype idx ) const
	{
		return _list[ idx ].get() == nullptr;
	}

};


#endif // COMMON_TYPES_INCLUDED
