//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: miscellaneous utilities that are needed in multiple places but don't belong anywhere else
//======================================================================================================================

#include "MiscUtils.hpp"

#include "OwnFileDialog.hpp"

#include <QFileInfo>
#include <QTextStream>
#include <QMessageBox>
#include <QLineEdit>


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


//----------------------------------------------------------------------------------------------------------------------
//  DialogCommon

QString DialogCommon::lineEditOrLastDir( QLineEdit * line )
{
	QString lineText = line->text();
	return !lineText.isEmpty() ? lineText : lastUsedDir;
}

void DialogCommon::browseDir( QWidget * parent, const QString & dirPurpose, QLineEdit * targetLine )
{
	QString path = OwnFileDialog::getExistingDirectory( parent, "Locate the directory "+dirPurpose, lineEditOrLastDir( targetLine ) );
	if (path.isEmpty())  // user probably clicked cancel
		return;

	// the path comming out of the file dialog is always absolute
	if (pathContext.usingRelativePaths())
		path = pathContext.getRelativePath( path );

	// next time use this dir as the starting dir of the file dialog for convenience
	lastUsedDir = path;

	targetLine->setText( path );
	// the rest of the actions will be performed in the line edit callback,
	// because we want to do the same things when user edits the path manually
}
