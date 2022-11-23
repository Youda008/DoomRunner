//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: miscellaneous utilities that are needed in multiple places but don't belong anywhere else
//======================================================================================================================

#ifndef MISC_UTILS_INCLUDED
#define MISC_UTILS_INCLUDED


#include "Common.hpp"

#include "Widgets/ListModel.hpp"  // ReadOnlyListModelItem

#include <QString>
#include <QVector>
#include <QColor>

class QLineEdit;


//----------------------------------------------------------------------------------------------------------------------
//  path verification

/// Highlights a directory path in a QLineEdit if such directory doesn't exist.
/** Returns true if the text was highlighted. */
bool highlightDirPathIfInvalid( QLineEdit * lineEdit, const QString & path );

/// Highlights a file path in a QLineEdit if such file doesn't exist.
/** Returns true if the text was highlighted. */
bool highlightFilePathIfInvalid( QLineEdit * lineEdit, const QString & path );

/// Highlights a path that leads to a file instead of directory.
/** Returns true if the text was highlighted. */
bool highlightDirPathIfFile( QLineEdit * lineEdit, const QString & path );

/// Makes this item highlighted in its views.
void highlightInvalidListItem( ReadOnlyListModelItem & item );

/// Removed the highlighting of this item in its views.
void unhighlightListItem( ReadOnlyListModelItem & item );

bool checkPath( const QString & path, const QString & errorMessage );
bool checkNonEmptyPath( const QString & path, const QString & errorMessage );

class PathChecker {

	QWidget * parent;
	bool verificationRequired;
	bool errorMessageDisplayed = false;

	bool _checkPath( const QString & path, const QString & errorMessage );

 public:

	PathChecker( QWidget * parent, bool verificationRequired )
		: parent( parent ), verificationRequired( verificationRequired ) {}

	bool checkPath( const QString & path, const QString & errorMessage )
	{
		if (!verificationRequired)
			return true;

		return _checkPath( path, errorMessage );
	}

	template< typename ListItem >
	bool checkItemPath( ListItem & item, const QString & errorMessage )
	{
		if (!verificationRequired)
			return true;

		bool verified = _checkPath( item.getFilePath(), errorMessage );
		if (!verified)
			highlightInvalidListItem( item );
		else
			unhighlightListItem( item );
		return verified;
	}

	bool gotSomeInvalidPaths() const
	{
		return errorMessageDisplayed;
	}

};


//----------------------------------------------------------------------------------------------------------------------
//  other

/// Replaces everything between startingChar and endingChar with replaceWith
QString replaceStringBetween( QString source, char startingChar, char endingChar, const QString & replaceWith );

/// Creates a file filter for the QFileDialog::getOpenFileNames.
QString makeFileFilter( const char * filterName, const QVector< QString > & suffixes );

/// Makes a component by component mix of the input colors that corresponds to expression:
/// color1 * weight1 + color2 * weight2 + addition
QColor mixColors( QColor color1, int weight1, QColor color2, int weight2, QColor addition );


#endif // MISC_UTILS_INCLUDED
