//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: commonly used types and type aliases
//======================================================================================================================

#ifndef COMMON_TYPES_INCLUDED
#define COMMON_TYPES_INCLUDED


#include "Essential.hpp"

#include <QList>

#include <memory>

#include <QDebug>


//======================================================================================================================

/// Wrapper around iterator to container of pointers that skips the additional needed dereference and returns a reference directly
template< typename WrappedIter >
class DerefIterator
{
	WrappedIter wrappedIter;

 public:

	using iterator_category = typename WrappedIter::iterator_category;
    using difference_type = typename WrappedIter::difference_type;
    using value_type = std::remove_reference_t< decltype( **wrappedIter ) >;
    using pointer = value_type *;
    using reference = value_type &;

    DerefIterator( const WrappedIter & origIter ) : wrappedIter( origIter ) {}
	DerefIterator( WrappedIter && origIter ) : wrappedIter( std::move(origIter) ) {}

	auto operator*() -> decltype( **wrappedIter ) const   { return **wrappedIter; }
	auto operator->() -> decltype( *wrappedIter ) const   { return *wrappedIter; }

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

	explicit DeepCopyableUniquePtr( Elem * newElem ) : _ptr( newElem ) {}
	explicit DeepCopyableUniquePtr( std::unique_ptr< Elem > newElem ) : _ptr( std::move(newElem) ) {}

	// moves only the pointer, doesn't touch Elem, as expected
	DeepCopyableUniquePtr( DeepCopyableUniquePtr && other ) noexcept = default;
	DeepCopyableUniquePtr & operator=( DeepCopyableUniquePtr && other ) noexcept = default;

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
		return DeepCopyableUniquePtr( std::make_unique< Elem >( std::forward< Args ... >( args ... ) ) );
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

	// std::unique_ptr alone cannot be used in QList, because internally QList uses reference counting with copy-on-write,
	// which requires being able to make a copy of the element, when the container detaches due to a modification attempt.
	// With this pointer wrapper, when a shared instance needs to detach, it will copy all the elements
	// and create new pointers to them, which is exactly how the old QList behaved.
	QList< DeepCopyableUniquePtr< Elem > > _list;

 public:

	using iterator = DerefIterator< typename decltype( _list )::iterator >;
	using const_iterator = DerefIterator< typename decltype( _list )::const_iterator >;

	// content access

	auto count() const                                { return _list.count(); }
	auto size() const                                 { return _list.size(); }
	auto isEmpty() const                              { return _list.isEmpty(); }

	      auto & operator[]( qsizetype idx )          { return *_list[ idx ]; }
	const auto & operator[]( qsizetype idx ) const    { return *_list[ idx ]; }

	      iterator begin()                            { return DerefIterator( _list.begin() ); }
	const_iterator begin() const                      { return DerefIterator( _list.begin() ); }
	      iterator end()                              { return DerefIterator( _list.end() ); }
	const_iterator end() const                        { return DerefIterator( _list.end() ); }

	      auto & first()                              { return *_list.first(); }
	const auto & first() const                        { return *_list.first(); }
	      auto & last()                               { return *_list.last(); }
	const auto & last() const                         { return *_list.last(); }

	// list modification

	void clear()                                      { _list.clear(); }

	void append( const Elem &  elem )                 { _list.append( DeepCopyableUniquePtr< Elem >::allocNew( elem ) ); }
	void append(       Elem && elem )                 { _list.append( DeepCopyableUniquePtr< Elem >::allocNew( std::move(elem) ) ); }
	void prepend( const Elem &  elem )                { _list.prepend( DeepCopyableUniquePtr< Elem >::allocNew( elem ) ); }
	void prepend(       Elem && elem )                { _list.prepend( DeepCopyableUniquePtr< Elem >::allocNew( std::move(elem) ) ); }
	void insert( qsizetype idx, const Elem &  elem )  { _list.insert( idx, DeepCopyableUniquePtr< Elem >::allocNew( elem ) ); }
	void insert( qsizetype idx,       Elem && elem )  { _list.insert( idx, DeepCopyableUniquePtr< Elem >::allocNew( std::move(elem) ) ); }

	void removeAt( qsizetype idx )                    { _list.removeAt( idx ); }
	void move( qsizetype from, qsizetype to )         { _list.move( from, to ); }

	// low-level pointer manipulation for implementing optimized high-level operations

	bool isNull( qsizetype idx ) const
	{
		return _list[ idx ].get() == nullptr;
	}

	std::unique_ptr< Elem > takePtr( qsizetype idx )
	{
		return std::move( _list[ idx ] ).to_unique_ptr();
	}

	void removePtr( qsizetype idx )
	{
		_list.removeAt( idx );
	}

	void insertPtr( qsizetype idx, std::unique_ptr< Elem > ptr )
	{
		_list.insert( idx, DeepCopyableUniquePtr< Elem >( std::move(ptr) ) );
	}
};


#endif // COMMON_TYPES_INCLUDED
