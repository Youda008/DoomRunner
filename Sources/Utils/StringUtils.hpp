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

class QTextStream;


//----------------------------------------------------------------------------------------------------------------------

// to be used when we want to pass empty string, but a reference or pointer is required
extern const QString emptyString;


inline bool isLetter( QChar c )
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

/// Makes the first letter of a string capital.
inline QString & capitalize( QString & str )
{
	str[0] = str[0].toUpper();
	return str;
}

/// Makes the first letter of a string capital.
inline QString capitalize( const QString & str )
{
	auto strCopy = str;
	return capitalize( strCopy );
}

/// Replaces everything between startingChar and endingChar with replaceWith
QString replaceStringBetween( QString source, char startingChar, char endingChar, const QString & replaceWith );

QTextStream & operator<<( QTextStream & stream, const QStringList & list );


//----------------------------------------------------------------------------------------------------------------------


#endif // STRING_UTILS_INCLUDED
