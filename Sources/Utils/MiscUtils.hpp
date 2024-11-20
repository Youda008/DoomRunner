//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: miscellaneous utilities that are needed in multiple places but don't belong anywhere else
//======================================================================================================================

#ifndef MISC_UTILS_INCLUDED
#define MISC_UTILS_INCLUDED


#include "Essential.hpp"

#include "CommonTypes.hpp"
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

/// Highlights a path in a QLineEdit that leads to a file instead of directory.
/** Returns true if the text was highlighted. */
bool highlightDirPathIfFile( QLineEdit * lineEdit, const QString & path );

/// Highlights a path in a QLineEdit that leads to a directory instead of file.
/** Returns true if the text was highlighted. */
bool highlightFilePathIfDir( QLineEdit * lineEdit, const QString & path );

/// Highlights a path in a QLineEdit that leads to a file instead of directory or it doesn't exist but can be created.
/** Returns true if the text was highlighted. */
bool highlightDirPathIfFileOrCanBeCreated( QLineEdit * lineEdit, const QString & path );

/// Highlights a path in a QLineEdit that leads to a directory instead of file or it doesn't exist but can be created.
/** Returns true if the text was highlighted. */
bool highlightFilePathIfDirOrCanBeCreated( QLineEdit * lineEdit, const QString & path );

/// Makes this item highlighted in its views.
void highlightInvalidListItem( const ReadOnlyListModelItem & item );

/// Removed the highlighting of this item in its views.
void unhighlightListItem( const ReadOnlyListModelItem & item );

/// Marks this item as the default one.
void markItemAsDefault( const ReadOnlyListModelItem & item );

/// Removes the default item marking.
void unmarkItemAsDefault( const ReadOnlyListModelItem & item );


class PathChecker {

	QWidget * parent;
	bool verificationRequired;
	bool errorMessageDisplayed = false;

 private: // internal D.R.Y. helpers

	enum class EntryType
	{
		File,
		Dir,
		Both
	};

	static void _maybeShowError( bool & errorMessageDisplayed, QWidget * parent, QString title, QString message );

	static bool _checkPath( const QString & path, EntryType expectedType, bool & errorMessageDisplayed,
	                        QWidget * parent, QString subjectName, QString errorPostscript );
	static bool _checkNonEmptyPath( const QString & path, EntryType expectedType, bool & errorMessageDisplayed,
	                                QWidget * parent, QString subjectName, QString errorPostscript );
	static bool _checkCollision( const QString & path, EntryType expectedType, bool & errorMessageDisplayed,
	                             QWidget * parent, QString subjectName, QString errorPostscript );

	bool _checkPath( const QString & path, EntryType expectedType, QString subjectName, QString errorPostscript )
	{
		if (!verificationRequired)
			return true;

		return _checkPath( path, expectedType, errorMessageDisplayed, parent, subjectName, errorPostscript );
	}

	template< typename ListItem >
	bool _checkItemPath( ListItem & item, EntryType expectedType, QString subjectName, QString errorPostscript )
	{
		if (!verificationRequired)
			return true;

		bool verified = _checkPath( item.getFilePath(), expectedType, errorMessageDisplayed, parent, subjectName, errorPostscript );
		if (!verified)
			highlightInvalidListItem( item );
		else
			unhighlightListItem( item );
		return verified;
	}

	bool _checkCollision( const QString & path, EntryType expectedType, QString subjectName, QString errorPostscript )
	{
		if (!verificationRequired)
			return true;

		if (path.isEmpty() || !fs::exists( path ))
			return true;

		return _checkCollision( path, expectedType, errorMessageDisplayed, parent, subjectName, errorPostscript );
	}

 public: // context-free

	static bool checkFilePath( const QString & path, bool showError, QString subjectName, QString errorPostscript )
	{
		bool errorMessageDisplayed = !showError;
		return _checkPath( path, EntryType::File, errorMessageDisplayed, nullptr, subjectName, errorPostscript );
	}

	static bool checkDirPath( const QString & path, bool showError, QString subjectName, QString errorPostscript )
	{
		bool errorMessageDisplayed = !showError;
		return _checkPath( path, EntryType::Dir, errorMessageDisplayed, nullptr, subjectName, errorPostscript );
	}

	static bool checkNonEmptyFilePath( const QString & path, bool showError, QString subjectName, QString errorPostscript )
	{
		if (path.isEmpty())
			return true;

		bool errorMessageDisplayed = !showError;
		return _checkNonEmptyPath( path, EntryType::File, errorMessageDisplayed, nullptr, subjectName, errorPostscript );
	}

	static bool checkNonEmptyDirPath( const QString & path, bool showError, QString subjectName, QString errorPostscript )
	{
		if (path.isEmpty())
			return true;

		bool errorMessageDisplayed = !showError;
		return _checkNonEmptyPath( path, EntryType::Dir, errorMessageDisplayed, nullptr, subjectName, errorPostscript );
	}

 public: // context-sensitive (depend on settings from constructor)

	PathChecker( QWidget * parent, bool verificationRequired )
		: parent( parent ), verificationRequired( verificationRequired ) {}

	bool checkAnyPath( const QString & path, QString subjectName, QString errorPostscript )
	{
		return _checkPath( path, EntryType::Both, subjectName, errorPostscript );
	}

	bool checkFilePath( const QString & path, QString subjectName, QString errorPostscript )
	{
		return _checkPath( path, EntryType::File, subjectName, errorPostscript );
	}

	bool checkDirPath( const QString & path, QString subjectName, QString errorPostscript )
	{
		return _checkPath( path, EntryType::Dir, subjectName, errorPostscript );
	}

	bool checkNotAFile( const QString & path, QString subjectName, QString errorPostscript )
	{
		return _checkCollision( path, EntryType::Dir, subjectName, errorPostscript );
	}

	bool checkNotADir( const QString & path, QString subjectName, QString errorPostscript )
	{
		return _checkCollision( path, EntryType::File, subjectName, errorPostscript );
	}

	template< typename ListItem >
	bool checkItemAnyPath( ListItem & item, QString subjectName, QString errorPostscript )
	{
		return _checkItemPath( item, EntryType::Both, subjectName, errorPostscript );
	}

	template< typename ListItem >
	bool checkItemFilePath( ListItem & item, QString subjectName, QString errorPostscript )
	{
		return _checkItemPath( item, EntryType::File, subjectName, errorPostscript );
	}

	template< typename ListItem >
	bool checkItemDirPath( ListItem & item, QString subjectName, QString errorPostscript )
	{
		return _checkItemPath( item, EntryType::Dir, subjectName, errorPostscript );
	}

	bool gotSomeInvalidPaths() const
	{
		return errorMessageDisplayed;
	}

};


//----------------------------------------------------------------------------------------------------------------------
//  other

/// Makes the first letter of a string capital.
inline QString & capitalize( QString & str )
{
	str[0] = str[0].toUpper();
	return str;
}

inline QString capitalize( const QString & str )
{
	auto strCopy = str;
	return capitalize( strCopy );
}

/// Replaces everything between startingChar and endingChar with replaceWith
QString replaceStringBetween( QString source, char startingChar, char endingChar, const QString & replaceWith );

/// Creates a file filter for the QFileDialog::getOpenFileNames.
QString makeFileFilter( const char * filterName, const QStringVec & suffixes );

struct Argument
{
	QString str;  ///< individual argument trimmed from whitespaces and quotes
	bool wasQuoted;  ///< whether this argument was originally quoted
};
/// Splits a command line string into individual arguments, taking into account quotes.
/** NOTE: This is simplified, it will not handle the full command line syntax, only the basic cases. */
QVector< Argument > splitCommandLineArguments( const QString & argsStr );

bool areScreenCoordinatesValid( int x, int y );


#endif // MISC_UTILS_INCLUDED
