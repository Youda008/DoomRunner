//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: miscellaneous utilities that are needed in multiple places but don't belong anywhere else
//======================================================================================================================

#include "MiscUtils.hpp"

#include "LangUtils.hpp"  // correspondingValue
#include "StringUtils.hpp"  // capitalize
#include "FileSystemUtils.hpp"  // isValidDir, isValidFile
#include "WidgetUtils.hpp"  // setTextColor
#include "Themes.hpp"  // getCurrentPalette
#include "ErrorHandling.hpp"

#include <QFileInfo>
#include <QTextStream>
#include <QLineEdit>
#include <QStringBuilder>
#include <QGuiApplication>
#include <QScreen>


//----------------------------------------------------------------------------------------------------------------------
// path highlighting

bool highlightDirPathIfInvalid( QLineEdit * lineEdit, const QString & path )
{
	if (fs::isInvalidDir( path ))
	{
		wdg::setTextColor( lineEdit, themes::getCurrentPalette().invalidEntryText );
		return true;
	}
	else
	{
		wdg::restoreColors( lineEdit );
		return false;
	}
}

bool highlightFilePathIfInvalid( QLineEdit * lineEdit, const QString & path )
{
	if (fs::isInvalidFile( path ))
	{
		wdg::setTextColor( lineEdit, themes::getCurrentPalette().invalidEntryText );
		return true;
	}
	else
	{
		wdg::restoreColors( lineEdit );
		return false;
	}
}

bool highlightDirPathIfFile( QLineEdit * lineEdit, const QString & path )
{
	if (fs::isValidFile( path ))
	{
		wdg::setTextColor( lineEdit, themes::getCurrentPalette().invalidEntryText );
		return true;
	}
	else
	{
		wdg::restoreColors( lineEdit );
		return false;
	}
}

bool highlightFilePathIfDir( QLineEdit * lineEdit, const QString & path )
{
	if (fs::isValidDir( path ))
	{
		wdg::setTextColor( lineEdit, themes::getCurrentPalette().invalidEntryText );
		return true;
	}
	else
	{
		wdg::restoreColors( lineEdit );
		return false;
	}
}

bool highlightDirPathIfFileOrCanBeCreated( QLineEdit * lineEdit, const QString & path )
{
	if (path.isEmpty())
	{
		wdg::restoreColors( lineEdit );
		return false;
	}

	QFileInfo dir( path );
	if (!dir.exists())
	{
		wdg::setTextColor( lineEdit, themes::getCurrentPalette().toBeCreatedEntryText );
		return true;
	}
	else if (dir.isFile())
	{
		wdg::setTextColor( lineEdit, themes::getCurrentPalette().invalidEntryText );
		return true;
	}
	else
	{
		wdg::restoreColors( lineEdit );
		return false;
	}
}

bool highlightFilePathIfInvalidOrCanBeCreated( QLineEdit * lineEdit, const QString & path )
{
	if (path.isEmpty())
	{
		wdg::restoreColors( lineEdit );
		return false;
	}

	QFileInfo file( path );
	if (!file.exists())
	{
		wdg::setTextColor( lineEdit, themes::getCurrentPalette().toBeCreatedEntryText );
		return true;
	}
	else if (file.isDir())
	{
		wdg::setTextColor( lineEdit, themes::getCurrentPalette().invalidEntryText );
		return true;
	}
	else
	{
		wdg::restoreColors( lineEdit );
		return false;
	}
}

void highlightInvalidListItem( const ReadOnlyListModelItem & item )
{
	item.textColor = themes::getCurrentPalette().invalidEntryText;
}

void unhighlightListItem( const ReadOnlyListModelItem & item )
{
	item.textColor.reset();
}

void markItemAsDefault( const ReadOnlyListModelItem & item )
{
	item.textColor = themes::getCurrentPalette().defaultEntryText;
}

void unmarkItemAsDefault( const ReadOnlyListModelItem & item )
{
	item.textColor = themes::getCurrentPalette().color( QPalette::Text );
}


//----------------------------------------------------------------------------------------------------------------------
// PathChecker

void PathChecker::_maybeShowError( bool & errorMessageDisplayed, QWidget * parent, QString title, QString message )
{
	if (!errorMessageDisplayed)
	{
		reportUserError( parent, title, message );
		errorMessageDisplayed = true;  // don't spam too many errors when something goes wrong
	}
}

bool PathChecker::_checkPath(
	const QString & path, EntryType expectedType, bool & errorMessageDisplayed,
	QWidget * parent, QString subjectName, QString errorPostscript
){
	if (path.isEmpty())
	{
		_maybeShowError( errorMessageDisplayed, parent, "Path is empty",
			"Path of "%subjectName%" is empty. "%errorPostscript );
		return false;
	}

	return _checkNonEmptyPath( path, expectedType, errorMessageDisplayed, parent, subjectName, errorPostscript );
}

bool PathChecker::_checkNonEmptyPath(
	const QString & path, EntryType expectedType, bool & errorMessageDisplayed,
	QWidget * parent, QString subjectName, QString errorPostscript
){
	if (!fs::exists( path ))
	{
		QString fileOrDir = correspondingValue( expectedType,
			corresponds( EntryType::File, "File" ),
			corresponds( EntryType::Dir,  "Directory" ),
			corresponds( EntryType::Both, "File or directory" )
		);
		_maybeShowError( errorMessageDisplayed, parent, fileOrDir%" no longer exists",
			capitalize(subjectName)%" ("%path%") no longer exists. "%errorPostscript );
		return false;
	}

	return _checkCollision( path, expectedType, errorMessageDisplayed, parent, subjectName, errorPostscript );
}

bool PathChecker::_checkCollision(
	const QString & path, EntryType expectedType, bool & errorMessageDisplayed,
	QWidget * parent, QString subjectName, QString errorPostscript
){
	QFileInfo entry( path );
	if (expectedType == EntryType::File && !entry.isFile())
	{
		_maybeShowError( errorMessageDisplayed, parent, "Path is a directory",
			capitalize(subjectName)%" ("%path%") is a directory, but it should be a file. "%errorPostscript );
		return false;
	}
	if (expectedType == EntryType::Dir && !entry.isDir())
	{
		_maybeShowError( errorMessageDisplayed, parent, "Path is a file",
			capitalize(subjectName)%" ("%path%") is a file, but it should be a directory. "%errorPostscript );
		return false;
	}
	return true;
}


//----------------------------------------------------------------------------------------------------------------------
// other

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
	for (int currentPos = 0; currentPos < argsStr.size(); ++currentPos)
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
