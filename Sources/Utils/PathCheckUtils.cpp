//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: utilities related to file/directory path verification
//======================================================================================================================

#include "PathCheckUtils.hpp"

#include "Utils/LangUtils.hpp"       // correspondingValue
#include "Utils/StringUtils.hpp"     // capitalize
#include "Utils/WidgetUtils.hpp"     // setTextColor, restoreColors
#include "Utils/ErrorHandling.hpp"   // reportUserError
#include "Themes.hpp"                // getCurrentPalette

#include <QString>
#include <QFileInfo>
#include <QLineEdit>
#include <QMessageBox>


//----------------------------------------------------------------------------------------------------------------------
// path highlighting

void highlightPathLineAsInvalid( QLineEdit * lineEdit )
{
	wdg::setTextColor( lineEdit, themes::getCurrentPalette().invalidEntryText );
}

void highlightPathLineAsToBeCreated( QLineEdit * lineEdit )
{
	wdg::setTextColor( lineEdit, themes::getCurrentPalette().toBeCreatedEntryText );
}

void unhighlightPathLine( QLineEdit * lineEdit )
{
	wdg::restoreColors( lineEdit );
}

bool highlightDirPathIfInvalid( QLineEdit * lineEdit, const QString & path )
{
	if (fs::isInvalidDir( path ))
	{
		highlightPathLineAsInvalid( lineEdit );
		return true;
	}
	else
	{
		unhighlightPathLine( lineEdit );
		return false;
	}
}

bool highlightFilePathIfInvalid( QLineEdit * lineEdit, const QString & path )
{
	if (fs::isInvalidFile( path ))
	{
		highlightPathLineAsInvalid( lineEdit );
		return true;
	}
	else
	{
		unhighlightPathLine( lineEdit );
		return false;
	}
}

bool highlightDirPathIfFile( QLineEdit * lineEdit, const QString & path )
{
	if (fs::isValidFile( path ))
	{
		highlightPathLineAsInvalid( lineEdit );
		return true;
	}
	else
	{
		unhighlightPathLine( lineEdit );
		return false;
	}
}

bool highlightFilePathIfDir( QLineEdit * lineEdit, const QString & path )
{
	if (fs::isValidDir( path ))
	{
		highlightPathLineAsInvalid( lineEdit );
		return true;
	}
	else
	{
		unhighlightPathLine( lineEdit );
		return false;
	}
}

bool highlightDirPathIfFileOrCanBeCreated( QLineEdit * lineEdit, const QString & path )
{
	if (path.isEmpty())
	{
		unhighlightPathLine( lineEdit );
		return false;
	}

	QFileInfo dir( path );
	if (!dir.exists())
	{
		highlightPathLineAsToBeCreated( lineEdit );
		return true;
	}
	else if (dir.isFile())
	{
		highlightPathLineAsInvalid( lineEdit );
		return true;
	}
	else
	{
		unhighlightPathLine( lineEdit );
		return false;
	}
}

bool highlightFilePathIfDirOrCanBeCreated( QLineEdit * lineEdit, const QString & path )
{
	if (path.isEmpty())
	{
		unhighlightPathLine( lineEdit );
		return false;
	}

	QFileInfo file( path );
	if (!file.exists())
	{
		highlightPathLineAsToBeCreated( lineEdit );
		return true;
	}
	else if (file.isDir())
	{
		highlightPathLineAsInvalid( lineEdit );
		return true;
	}
	else
	{
		unhighlightPathLine( lineEdit );
		return false;
	}
}

void highlightListItemAsInvalid( const AModelItem & item )
{
	item.textColor = themes::getCurrentPalette().invalidEntryText;
}

void unhighlightListItem( const AModelItem & item )
{
	item.textColor.reset();
}

void markItemAsDefault( const AModelItem & item )
{
	item.textColor = themes::getCurrentPalette().defaultEntryText;
}

void unmarkItemAsDefault( const AModelItem & item )
{
	item.textColor = themes::getCurrentPalette().color( QPalette::Text );
}


//----------------------------------------------------------------------------------------------------------------------
// PathChecker

void PathChecker::s_maybeShowError( bool & errorMessageDisplayed, QWidget * parent, cStrRef title, cStrRef message )
{
	if (!errorMessageDisplayed)
	{
		reportUserError( parent, title, message );
		errorMessageDisplayed = true;  // don't spam too many errors when something goes wrong
	}
}

bool PathChecker::s_checkPath(
	cStrRef path, EntryType expectedType, bool & errorMessageDisplayed, QWidget * parent,
	cStrRef subjectName, cStrRef errorPostscript
){
	if (path.isEmpty())
	{
		s_maybeShowError( errorMessageDisplayed, parent, "Path is empty",
			"Path of "%subjectName%" is empty. "%errorPostscript );
		return false;
	}

	return s_checkNonEmptyPath( path, expectedType, errorMessageDisplayed, parent, subjectName, errorPostscript );
}

bool PathChecker::s_checkNonEmptyPath(
	cStrRef path, EntryType expectedType, bool & errorMessageDisplayed, QWidget * parent,
	cStrRef subjectName, cStrRef errorPostscript
){
	if (!fs::exists( path ))
	{
		QString fileOrDir = correspondingValue( expectedType,
			correspondsTo( EntryType::File, "File" ),
			correspondsTo( EntryType::Dir,  "Directory" ),
			correspondsTo( EntryType::Both, "File or directory" )
		);
		s_maybeShowError( errorMessageDisplayed, parent, fileOrDir%" no longer exists",
			capitalize(subjectName)%" ("%path%") no longer exists. "%errorPostscript );
		return false;
	}

	return s_checkExistingPathForCollision( path, expectedType, errorMessageDisplayed, parent, subjectName, errorPostscript );
}

bool PathChecker::s_checkCollision(
	cStrRef path, EntryType expectedType, bool & errorMessageDisplayed, QWidget * parent,
	cStrRef subjectName, cStrRef errorPostscript
){
	if (path.isEmpty() || !fs::exists( path ))
	{
		return true;  // here we only care if the path collides with something, everything else is ok
	}

	return s_checkExistingPathForCollision( path, expectedType, errorMessageDisplayed, parent, subjectName, errorPostscript );
}

bool PathChecker::s_checkExistingPathForCollision(
	cStrRef path, EntryType expectedType, bool & errorMessageDisplayed, QWidget * parent,
	cStrRef subjectName, cStrRef errorPostscript
){
	QFileInfo entry( path );
	if (expectedType == EntryType::File && !entry.isFile())
	{
		s_maybeShowError( errorMessageDisplayed, parent, "Path is a directory",
			capitalize(subjectName)%" ("%path%") is a directory, but a file is expected. "%errorPostscript );
		return false;
	}
	if (expectedType == EntryType::Dir && !entry.isDir())
	{
		s_maybeShowError( errorMessageDisplayed, parent, "Path is a file",
			capitalize(subjectName)%" ("%path%") is a file, but a directory is expected. "%errorPostscript );
		return false;
	}
	return true;
}

bool PathChecker::s_checkOverwrite(
	cStrRef path, bool & errorMessageDisplayed, QWidget * parent,
	cStrRef subjectName, cStrRef errorPostscript
){
	QFileInfo entry( path );
	if (entry.exists( path ))
	{
		if (!entry.isFile())
		{
			s_maybeShowError( errorMessageDisplayed, parent, "Path is a directory",
				capitalize(subjectName)%" ("%path%") is a directory, but a file is expected. "%errorPostscript );
			return false;
		}
		else if (!errorMessageDisplayed)
		{
			auto answer = QMessageBox::question( parent, "Overwrite existing file",
				capitalize(subjectName)%" ("%path%") already exists. Do you want to overwrite it?",
				QMessageBox::Yes | QMessageBox::No
			);
			errorMessageDisplayed = (answer == QMessageBox::No);
			return answer == QMessageBox::Yes;
		}
	}
	return true;
}
