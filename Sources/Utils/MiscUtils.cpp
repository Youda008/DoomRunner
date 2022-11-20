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
#include <QMessageBox>


//----------------------------------------------------------------------------------------------------------------------
//  path verification

bool checkPath_MsgBox( const QString & path, const QString & errorMessage )
{
	if (!path.isEmpty() && !QFileInfo::exists( path ))
	{
		QMessageBox::warning( nullptr, "File or directory no longer exists", errorMessage.arg( path ) );
		return false;
	}
	return true;
}

void checkPath_exception( const QString & path )
{
	if (!path.isEmpty() && !QFileInfo::exists( path ))
	{
		throw FileOrDirNotFound{ path };
	}
}

void assertValidPath( bool verificationRequired, const QString & path, const QString & errorMessage )
{
	if (verificationRequired)
		if (!checkPath_MsgBox( path, errorMessage ))
			throw FileOrDirNotFound{ path };
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

void highlightInvalidDir( QLineEdit * lineEdit, const QString & newPath )
{
	if (isInvalidDir( newPath ))
	{
		setTextColor( lineEdit, QColor( Qt::red ) );
	}
	else
	{
		restoreColors( lineEdit );
	}
}

void highlightInvalidFile( QLineEdit * lineEdit, const QString & newPath )
{
	if (isInvalidFile( newPath ))
	{
		setTextColor( lineEdit, QColor( Qt::red ) );
	}
	else
	{
		restoreColors( lineEdit );
	}
}
