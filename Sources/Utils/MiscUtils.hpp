//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: miscellaneous utilities that are needed in multiple places but don't belong anywhere else
//======================================================================================================================

#ifndef MISC_UTILS_INCLUDED
#define MISC_UTILS_INCLUDED


#include "Common.hpp"

#include <QString>
#include <QVector>

class QLineEdit;


//----------------------------------------------------------------------------------------------------------------------
//  path verification

bool checkPath_MsgBox( const QString & path, const QString & errorMessage );

class FileOrDirNotFound
{
	public: QString path;
};
void checkPath_exception( const QString & path );

void assertValidPath( bool verificationRequired, const QString & path, const QString & errorMessage );


//----------------------------------------------------------------------------------------------------------------------
//  other

/// Replaces everything between startingChar and endingChar with replaceWith
QString replaceStringBetween( QString source, char startingChar, char endingChar, const QString & replaceWith );

/// Creates a file filter for the QFileDialog::getOpenFileNames.
QString makeFileFilter( const char * filterName, const QVector< QString > & suffixes );

/// Highlights a non-existing file path by a different text color
void highlightInvalidDir( QLineEdit * lineEdit, const QString & newPath );
/// Highlights a non-existing dir path by a different text color
void highlightInvalidFile( QLineEdit * lineEdit, const QString & newPath );


#endif // MISC_UTILS_INCLUDED
