//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: miscellaneous utilities that are needed in multiple places but don't belong anywhere else
//======================================================================================================================

#include "MiscUtils.hpp"

#include "FileSystemUtils.hpp"  // isValidDir, isValidFile
#include "WidgetUtils.hpp"  // setTextColor

#include <QFileInfo>
#include <QTextStream>
#include <QLineEdit>
#include <QMessageBox>


//----------------------------------------------------------------------------------------------------------------------
//  path verification

static const QColor highlightColor = Qt::red;

bool highlightDirPathIfInvalid( QLineEdit * lineEdit, const QString & path )
{
	if (isInvalidDir( path ))
	{
		setTextColor( lineEdit, QColor( highlightColor ) );
		return true;
	}
	else
	{
		restoreColors( lineEdit );
		return false;
	}
}

bool highlightFilePathIfInvalid( QLineEdit * lineEdit, const QString & path )
{
	if (isInvalidFile( path ))
	{
		setTextColor( lineEdit, QColor( highlightColor ) );
		return true;
	}
	else
	{
		restoreColors( lineEdit );
		return false;
	}
}

bool highlightDirPathIfFile( QLineEdit * lineEdit, const QString & path )
{
	if (isValidFile( path ))
	{
		setTextColor( lineEdit, QColor( highlightColor ) );
		return true;
	}
	else
	{
		restoreColors( lineEdit );
		return false;
	}
}

void highlightInvalidListItem( ReadOnlyListModelItem & item )
{
	item.foregroundColor = highlightColor;
}

void unhighlightListItem( ReadOnlyListModelItem & item )
{
	item.foregroundColor.reset();
}

bool checkPath( const QString & path, const QString & errorMessage )
{
	if (path.isEmpty())
	{
		QMessageBox::warning( nullptr, "Path is empty", errorMessage.arg( path ) );
		return false;
	}
	if (!QFileInfo::exists( path ))
	{
		QMessageBox::warning( nullptr, "File or directory no longer exists", errorMessage.arg( path ) );
		return false;
	}
	return true;
}

bool checkNonEmptyPath( const QString & path, const QString & errorMessage )
{
	if (!path.isEmpty() && !QFileInfo::exists( path ))
	{
		QMessageBox::warning( nullptr, "File or directory no longer exists", errorMessage.arg( path ) );
		return false;
	}
	return true;
}

bool PathChecker::_checkPath( const QString & path, const QString & errorMessage )
{
	if (path.isEmpty())
	{
		if (!errorMessageDisplayed)
		{
			QMessageBox::warning( parent, "Path is empty", errorMessage.arg( path ) );
			errorMessageDisplayed = true;  // don't spam too many errors when something goes wrong
		}
		return false;
	}
	if (!QFileInfo::exists( path ))
	{
		if (!errorMessageDisplayed)
		{
			QMessageBox::warning( parent, "File or directory no longer exists", errorMessage.arg( path ) );
			errorMessageDisplayed = true;  // don't spam too many errors when something goes wrong
		}
		return false;
	}
	return true;
}


//----------------------------------------------------------------------------------------------------------------------
//  other

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

QString makeFileFilter( const char * filterName, const QVector< QString > & suffixes )
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

QColor mixColors( QColor color1, int weight1, QColor color2, int weight2, QColor addition )
{
	int weightSum = weight1 + weight2;
	return QColor(
		(color1.red()   * weight1 + color2.red()   * weight2) / weightSum + addition.red(),
		(color1.green() * weight1 + color2.green() * weight2) / weightSum + addition.green(),
		(color1.blue()  * weight1 + color2.blue()  * weight2) / weightSum + addition.blue()
	);
}
