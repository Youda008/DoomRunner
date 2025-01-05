//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: miscellaneous string helpers
//======================================================================================================================

#ifndef STRING_UTILS_INCLUDED
#define STRING_UTILS_INCLUDED


#include "Essential.hpp"

#include <QString>


// to be used when we want to pass empty string, but a reference or pointer is required
extern const QString emptyString;


/// Makes the first letter of a string capital.
inline QString & capitalize( QString & str )
{
	str[0] = str[0].toUpper();
	return str;
}

inline QString capitalize( const QString & str )
{
	auto strCopy = str;
	return capitalize( strCopy );
}

/// Replaces everything between startingChar and endingChar with replaceWith
QString replaceStringBetween( QString source, char startingChar, char endingChar, const QString & replaceWith );


#endif // STRING_UTILS_INCLUDED
