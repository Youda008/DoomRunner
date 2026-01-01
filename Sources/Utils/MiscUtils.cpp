//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: miscellaneous utilities that are needed in multiple places but don't belong anywhere else
//======================================================================================================================

#include "MiscUtils.hpp"

#include "CommonTypes.hpp"  // qsize_t

#include <QTextStream>
#include <QIODevice>  // QIODevice::OpenMode
#include <QGuiApplication>
#include <QScreen>


//----------------------------------------------------------------------------------------------------------------------

QStringList makeFileSystemModelFilter( const QStringList & suffixes )
{
	QStringList filter;
	filter.reserve( suffixes.size() );
	for (const QString & suffix : suffixes)
		filter.append( "*."+suffix );
	return filter;
}

QString makeFileDialogFilter( const char * filterName, const QStringList & suffixes )
{
	QString filter;
	QTextStream filterStream( &filter, QIODevice::WriteOnly );

	filterStream << filterName << " (";
	for (const QString & suffix : suffixes)
		if (&suffix == &suffixes.first())
			filterStream <<  "*." << suffix << " *." << suffix.toUpper();
		else
			filterStream << " *." << suffix << " *." << suffix.toUpper();
	filterStream << ");;";

	filterStream.flush();
	return filter;
}

QList< Argument > splitCommandLineArguments( const QString & argsStr )
{
	QList< Argument > args;

	QString currentArg;
	currentArg.reserve( 32 );

	auto flushCurrentArg = [ &currentArg, &args ]( bool wasQuoted )
	{
		args.append( Argument{ std::move(currentArg), wasQuoted } );
		currentArg.resize( 0 );  // clear the content without freeing the allocated buffer
	};

	if constexpr (IS_WINDOWS)  // parse according to the Windows cmd escaping rules
	{
		bool insideQuotes = false;
		bool wasClosingQuotesChar = false;

		for (qsize_t currentPos = 0; currentPos < argsStr.size(); ++currentPos)
		{
			QChar currentChar = argsStr[ currentPos ];

			if (insideQuotes)
			{
				if (currentChar == '"') {
					insideQuotes = false;
					wasClosingQuotesChar = true;
				} else {
					currentArg += currentChar;
				}
			}
			else // not inside quotes
			{
				if (currentChar == '"') {
					insideQuotes = true;
					if (wasClosingQuotesChar) {
						// 2 consequent quote characters produce 1 quote character inside the quoted string
						currentArg += '"';
					}
				} else if (currentChar == ' ') {
					if (!currentArg.isEmpty() || wasClosingQuotesChar) {
						flushCurrentArg( wasClosingQuotesChar );
					}
				} else {
					currentArg += currentChar;
				}
				wasClosingQuotesChar = false;
			}
		}

		// We reached the end of the command line without encountering the final terminating space, flush the last word.
		if (!currentArg.isEmpty())
		{
			// accept also unterminated quoted argument
			flushCurrentArg( insideQuotes || wasClosingQuotesChar );
		}
	}
	else  // parse according to the bash escaping rules
	{
		bool insideSingleQuotes = false;
		bool insideDoubleQuotes = false;
		bool wasClosingQuotesChar = false;
		bool wasEscapeChar = false;

		for (qsize_t currentPos = 0; currentPos < argsStr.size(); ++currentPos)
		{
			QChar currentChar = argsStr[ currentPos ];

			if (insideSingleQuotes)
			{
				if (currentChar == '\'') {
					insideSingleQuotes = false;
					wasClosingQuotesChar = true;
				} else {
					currentArg += currentChar;
				}
			}
			else if (wasEscapeChar)
			{
				// 2 consequent escape characters produce 1 escape character inside the quoted string and don't escape any further
				wasEscapeChar = false;
				currentArg += currentChar;
				// We should handle all the special characters like '\n', '\t', '\b', but screw it, it's not needed.
			}
			else if (insideDoubleQuotes) // and wasn't escape char
			{
				if (currentChar == '\\') {
					wasEscapeChar = true;
				} else if (currentChar == '"') {
					insideDoubleQuotes = false;
					wasClosingQuotesChar = true;
				} else {
					currentArg += currentChar;
				}
			}
			else // wasn't escape char and not in quotes
			{
				if (currentChar == '\\') {
					wasEscapeChar = true;
				} else if (currentChar == '\'') {
					insideSingleQuotes = true;
				} else if (currentChar == '"') {
					insideDoubleQuotes = true;
				} else if (currentChar == ' ') {
					if (!currentArg.isEmpty() || wasClosingQuotesChar) {
						flushCurrentArg( wasClosingQuotesChar );
					}
				} else {
					currentArg += currentChar;
				}
				wasClosingQuotesChar = false;
			}
		}

		// We reached the end of the command line without encountering the final terminating space, flush the last word.
		if (!currentArg.isEmpty())
		{
			// accept also unterminated quoted argument
			flushCurrentArg( insideSingleQuotes || insideDoubleQuotes || wasClosingQuotesChar );
		}
	}

	return args;
}

bool areScreenCoordinatesValid( int x, int y )
{
	// find if the coordinates belong to any of the currently active virtual screens
	const auto screens = qApp->screens();
	for (QScreen * screen : screens)
	{
		auto availableCoordinates = screen->availableGeometry();
		if (x >= availableCoordinates.left() && x <= availableCoordinates.right()
		 && y >= availableCoordinates.top() && y <= availableCoordinates.bottom())
		{
			return true;
		}
	}

	// found no screen to which these coordinates belong to (secondary monitor might have been disconnected)
	return false;
}
