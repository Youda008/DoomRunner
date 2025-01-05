//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: miscellaneous string helpers
//======================================================================================================================

#include "StringUtils.hpp"


//======================================================================================================================

const QString emptyString;


//----------------------------------------------------------------------------------------------------------------------

QString replaceStringBetween( QString source, char startingChar, char endingChar, const QString & replaceWith )
{
	int startIdx = source.indexOf( startingChar );
	if (startIdx < 0 || startIdx == source.size() - 1)
		return source;
	int endIdx = source.indexOf( endingChar, startIdx + 1 );
	if (endIdx < 0)
		return source;

	source.replace( startIdx + 1, endIdx - startIdx - 1, replaceWith );

	return source;
}
