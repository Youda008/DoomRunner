//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: miscellaneous string helpers
//======================================================================================================================

#include "StringUtils.hpp"

#include "CommonTypes.hpp"  // qsize_t

#include <QStringList>
#include <QTextStream>


//======================================================================================================================

const QString emptyString;


QString replaceStringBetween( QString source, char startingChar, char endingChar, const QString & replaceWith )
{
	qsize_t startIdx = source.indexOf( startingChar );
	if (startIdx < 0 || startIdx == source.size() - 1)
		return source;
	qsize_t endIdx = source.indexOf( endingChar, startIdx + 1 );
	if (endIdx < 0)
		return source;

	source.replace( startIdx + 1, endIdx - startIdx - 1, replaceWith );

	return source;
}

QTextStream & operator<<( QTextStream & stream, const QStringList & list )
{
	stream << "[ ";
	bool firstWritten = false;
	for (const auto & str : list)
	{
		if (!firstWritten)
			firstWritten = true;
		else
			stream << ", ";
		stream << '"' << str << '"';
	}
	stream << " ]";

	return stream;
}
