//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: types used by FileSystemUtils.hpp, separated for less recompilation
//======================================================================================================================

#ifndef FILE_SYSTEM_UTILS_TYPES_INCLUDED
#define FILE_SYSTEM_UTILS_TYPES_INCLUDED


#include "Essential.hpp"


//======================================================================================================================

// Convenience wrapper around enum for shorter code. Simplifies:
//  * if (pathStyle == PathStyle::Relative)
//  * pathStyle = isAbsolute ? PathStyle::Absolute : PathStyle::Relative;
struct PathStyle
{
	enum Value : uint8_t
	{
		Relative,
		Absolute
	};

	struct IsAbsolute
	{
		bool isAbsolute;
	};

	PathStyle() {}
	constexpr PathStyle( Value val ) : _val( val ) {}
	constexpr PathStyle( IsAbsolute val ) : _val( val.isAbsolute ? PathStyle::Absolute : PathStyle::Relative ) {}

	bool operator==( const PathStyle & other ) const   { return _val == other._val; }
	bool operator!=( const PathStyle & other ) const   { return _val != other._val; }

	bool isAbsolute() const                            { return _val == Absolute; }
	bool isRelative() const                            { return _val == Relative; }
	void toggleAbsolute( bool isAbsolute )             { _val = isAbsolute ? PathStyle::Absolute : PathStyle::Relative; }

 private:

	Value _val;
};


namespace fs {

// Plain enum just makes the elements polute global namespace and makes the FILE collide with declaration in stdio.h,
// and enum class would make all bit operations incredibly painful.
class EntryTypes
{
	uint8_t _types;
 public:
	constexpr EntryTypes( uint8_t types ) : _types( types ) {}
	constexpr friend EntryTypes operator|( EntryTypes a, EntryTypes b ) { return EntryTypes( a._types | b._types ); }
	constexpr bool isSet( EntryTypes types ) const { return (_types & types._types) != 0; }
};
struct EntryType
{
	static constexpr EntryTypes DIR  = { 1 << 0 };
	static constexpr EntryTypes FILE = { 1 << 1 };
	static constexpr EntryTypes BOTH = DIR | FILE;
};

} // namespace fs


#endif // FILE_SYSTEM_UTILS_TYPES_INCLUDED
