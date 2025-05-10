//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: custom type traits
//======================================================================================================================

#ifndef TYPE_TRAITS_INCLUDED
#define TYPE_TRAITS_INCLUDED


#include "Essential.hpp"

#include <type_traits>
#include <iterator>  // begin, end


//======================================================================================================================
// macros

/// a syntax simplification for constraining template types before C++20
#define REQUIRES( ... ) std::enable_if_t< __VA_ARGS__, int > = 0


//======================================================================================================================
// type traits from the standard library of a newer C++ standard

namespace fut {

// C++20
template< typename T >
struct remove_cvref
{
    using type = std::remove_cv_t< std::remove_reference_t<T> >;
};
template< typename T >
using remove_cvref_t = typename remove_cvref<T>::type;

} // namespace fut


//======================================================================================================================
// custom type traits

namespace types {


/// To make a template type non-deducible from a function parameter
namespace impl
{
	template< typename T >
	struct identity
	{
		using type = T;
	};
}
template< typename T >
using identity = typename impl::identity<T>::type;

template< bool addConst, typename T >
using maybe_add_const = std::conditional_t<
	/*if*/ addConst,
	/*then*/ std::add_const_t< T >,
	/*else*/ T
>;

namespace impl
{
	template< typename T, typename = void >
	struct is_range : std::false_type {};

	template< typename T >
	struct is_range< T,
		std::void_t< decltype( std::begin( std::declval< T & >() ) ), decltype( std::end( std::declval< T & >() ) ) >
	> : std::true_type {};
}
/// Whether a data structure is a range of elements that can be iterated over
template< typename T >
inline constexpr bool is_range = impl::is_range< T >::value;

template< typename T >
using range_element = std::remove_reference_t< decltype( *std::begin( std::declval< T & >() ) ) >;

template< typename T >
using range_value = fut::remove_cvref_t< decltype( *std::begin( std::declval< T & >() ) ) >;

namespace impl
{
	template< typename T, typename E, REQUIRES( types::is_range<T> ) >
	static constexpr bool is_range_of()
	{
		return std::is_same_v< range_value<T>, std::remove_cv_t<E> >;
	}

	template< typename T, typename E, REQUIRES( !types::is_range<T> ) >
	static constexpr bool is_range_of()
	{
		return false;
	}
}
/// Whether a data structure T is a range of values E (ignores cv qualifiers)
template< typename T, typename E >
inline constexpr bool is_range_of = impl::is_range_of< T, E >();

namespace impl
{
	template< typename T, REQUIRES( !std::is_pointer_v<T> ) >
	typename T::iterator_category get_iterator_category( const T & );

	template< typename T, REQUIRES( std::is_pointer_v<T> ) >
	std::random_access_iterator_tag get_iterator_category( const T & );
}
template< typename T >
using iterator_category = decltype( impl::get_iterator_category( std::declval<T>() ) );

namespace impl
{
	template< typename T, REQUIRES( !std::is_pointer_v<T> ) >
	typename T::difference_type get_difference_type( const T & );

	template< typename T, REQUIRES( std::is_pointer_v<T> ) >
	decltype( std::declval<T>() - std::declval<T>() ) get_difference_type( const T & );
}
template< typename T >
using difference_type = decltype( impl::get_difference_type( std::declval<T>() ) );


} // namespace types


//======================================================================================================================


#endif // TYPE_TRAITS_INCLUDED
