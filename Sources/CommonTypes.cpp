//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: commonly used types and type aliases
//======================================================================================================================

#include "CommonTypes.hpp"

#include <QTextStream>


const QString emptyString;


QString QStringVec::join( QChar delimiter ) const
{
	QString result;

	// calculate required size
	int size = this->size() - 1;  // number of delimiters
	for (const auto & str : *this)
		size += str.size();

	// get enough memory in a single allocation
	result.reserve( size );

	for (const auto & str : *this)
	{
		if (result.isEmpty())
			result = str;
		else
			result.append( delimiter ).append( str );
	}

	return result;
}

QTextStream & operator<<( QTextStream & stream, const QStringVec & vec )
{
	stream << "[ ";
	bool firstWritten = false;
	for (const auto & str : vec)
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
