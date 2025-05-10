//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: compile-time unit tests for our type traits
//======================================================================================================================

#include "TypeTraits.hpp"

#include "ContainerUtils.hpp"  // span, range
#include "PtrList.hpp"

#include <array>
#include <vector>
#include <list>
#include <string>


// remove_cvref_t
static_assert( std::is_same_v< int, fut::remove_cvref_t< int > > );
static_assert( std::is_same_v< int, fut::remove_cvref_t< const int > > );
static_assert( std::is_same_v< int, fut::remove_cvref_t< volatile int > > );
static_assert( std::is_same_v< int, fut::remove_cvref_t< int & > > );
static_assert( std::is_same_v< int, fut::remove_cvref_t< const int & > > );

// maybe_add_const
static_assert( std::is_same_v< types::maybe_add_const< false, int >, int > );
static_assert( std::is_same_v< types::maybe_add_const< true,  int >, const int > );
static_assert( std::is_same_v< types::maybe_add_const< false, const int >, const int > );
static_assert( std::is_same_v< types::maybe_add_const< true,  const int >, const int > );

// is_range, is_range_of
static_assert( !types::is_range< int > );
using CharCArr = char [4];
static_assert( types::is_range< CharCArr > );
static_assert( types::is_range_of< CharCArr, char > );
static_assert( !types::is_range_of< CharCArr, uint8_t > );
using ConstCharCArr = const char [4];
static_assert( types::is_range< ConstCharCArr > );
static_assert( types::is_range_of< ConstCharCArr, char > );
using CharArr = std::array< char, 4 >;
static_assert( types::is_range< CharArr > );
static_assert( types::is_range_of< CharArr, char > );
using CharVec = std::vector< char >;
static_assert( types::is_range< CharVec > );
static_assert( types::is_range_of< CharVec, char > );
using CharList = std::list< char >;
static_assert( types::is_range< CharList > );
static_assert( types::is_range_of< CharList, char > );
using StringVec = std::vector< std::string >;
static_assert( types::is_range< StringVec > );
static_assert( types::is_range_of< StringVec, std::string > );
using CharSpan = span< char >;
static_assert( types::is_range< CharSpan > );
static_assert( types::is_range_of< CharSpan, char > );
using ConstCharSpan = span< const char >;
static_assert( types::is_range< ConstCharSpan > );
static_assert( types::is_range_of< ConstCharSpan, char > );
using CharSubrange = subrange< CharVec::iterator >;
static_assert( types::is_range< CharSubrange > );
static_assert( types::is_range_of< CharSubrange, char > );
using ConstCharSubrange = subrange< CharVec::const_iterator >;
static_assert( types::is_range< ConstCharSubrange > );
static_assert( types::is_range_of< ConstCharSubrange, char > );
using IndirectCharRange = PtrList< char >;
static_assert( types::is_range< IndirectCharRange > );
static_assert( types::is_range_of< IndirectCharRange, char > );
