//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: miscellaneous utilities that are needed in multiple places but don't belong anywhere else
//======================================================================================================================

#include "MiscUtils.hpp"

#include <QTextStream>
#include <QIODevice>  // QIODevice::OpenMode
#include <QGuiApplication>
#include <QScreen>


//----------------------------------------------------------------------------------------------------------------------

QString makeFileFilter( const char * filterName, const QStringList & suffixes )
{
	QString filter;
	QTextStream filterStream( &filter, QIODevice::WriteOnly );

	filterStream << filterName << " (";
	for (const QString & suffix : suffixes)
		if (&suffix == &suffixes[0])
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

	bool escaped = false;
	bool inQuotes = false;
	for (qsizetype currentPos = 0; currentPos < argsStr.size(); ++currentPos)
	{
		QChar currentChar = argsStr[ currentPos ];

		if (escaped)
		{
			escaped = false;
			currentArg += currentChar;
			// We should handle all the special characters like '\n', '\t', '\b', but screw it, it's not needed.
		}
		else if (inQuotes) // and not escaped
		{
			if (currentChar == '\\') {
				escaped = true;
			} else if (currentChar == '"') {
				inQuotes = false;
				args.append( Argument{ std::move(currentArg), true } );
				currentArg.clear();
			} else {
				currentArg += currentChar;
			}
		}
		else // not escaped and not in quotes
		{
			if (currentChar == '\\') {
				escaped = true;
			} else if (currentChar == '"') {
				inQuotes = true;
				if (!currentArg.isEmpty()) {
					args.append( Argument{ std::move(currentArg), false } );
					currentArg.clear();
				}
			} else if (currentChar == ' ') {
				if (!currentArg.isEmpty()) {
					args.append( Argument{ std::move(currentArg), false } );
					currentArg.clear();
				}
			} else {
				currentArg += currentChar;
			}
		}
	}

	if (!currentArg.isEmpty())
	{
		args.append( Argument{ std::move(currentArg), (inQuotes && argsStr.back() != '"') } );
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
