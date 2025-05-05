//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: utilities related to file/directory path verification
//======================================================================================================================

#ifndef PATH_CHECK_UTILS_INCLUDED
#define PATH_CHECK_UTILS_INCLUDED


#include "Essential.hpp"

#include "DataModels/AModelItem.hpp"

class QString;
class QWidget;
class QLineEdit;


//----------------------------------------------------------------------------------------------------------------------
// path highlighting

/// Highlights a path in a QLineEdit as being invalid.
void highlightPathLineAsInvalid( QLineEdit * lineEdit );

/// Highlights a path in a QLineEdit as non-existing path that can be automatically created.
void highlightPathLineAsToBeCreated( QLineEdit * lineEdit );

/// Removes the highlighting of a path in a QLineEdit.
void unhighlightPathLine( QLineEdit * lineEdit );

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
void highlightListItemAsInvalid( const AModelItem & item );

/// Removes the highlighting of this item in its views.
void unhighlightListItem( const AModelItem & item );

/// Marks this item as the default one.
void markItemAsDefault( const AModelItem & item );

/// Removes the default item marking.
void unmarkItemAsDefault( const AModelItem & item );


//----------------------------------------------------------------------------------------------------------------------
// PathChecker

class PathChecker {

	QWidget * parent;
	bool verificationRequired;
	bool errorMessageDisplayed = false;

	enum class EntryType
	{
		File,
		Dir,
		Both
	};

	using cStrRef = const QString &;  // for shorter function signatures

 public: // context-sensitive (depend on settings from constructor)

	PathChecker( QWidget * parent, bool verificationRequired )
		: parent( parent ), verificationRequired( verificationRequired ) {}

	bool gotSomeInvalidPaths() const
	{
		return errorMessageDisplayed;
	}

	bool checkAnyPath( cStrRef path, cStrRef subjectName, cStrRef errorPostscript )
	{
		return m_maybeCheckPath( path, EntryType::Both, subjectName, errorPostscript );
	}
	bool checkFilePath( cStrRef path, cStrRef subjectName, cStrRef errorPostscript )
	{
		return m_maybeCheckPath( path, EntryType::File, subjectName, errorPostscript );
	}
	bool checkDirPath( cStrRef path, cStrRef subjectName, cStrRef errorPostscript )
	{
		return m_maybeCheckPath( path, EntryType::Dir, subjectName, errorPostscript );
	}

	bool checkNotAFile( cStrRef path, cStrRef subjectName, cStrRef errorPostscript )
	{
		return m_maybeCheckCollision( path, EntryType::Dir, subjectName, errorPostscript );
	}
	bool checkNotADir( cStrRef path, cStrRef subjectName, cStrRef errorPostscript )
	{
		return m_maybeCheckCollision( path, EntryType::File, subjectName, errorPostscript );
	}

	bool checkOverwrite( cStrRef path, cStrRef subjectName, cStrRef errorPostscript )
	{
		return m_maybeCheckOverwrite( path, subjectName, errorPostscript );
	}

	bool checkLineAnyPath( cStrRef path, QLineEdit * line, cStrRef subjectName, cStrRef errorPostscript )
	{
		return m_maybeCheckLinePath( path, line, EntryType::Both, subjectName, errorPostscript );
	}
	bool checkLineFilePath( cStrRef path, QLineEdit * line, cStrRef subjectName, cStrRef errorPostscript )
	{
		return m_maybeCheckLinePath( path, line, EntryType::File, subjectName, errorPostscript );
	}
	bool checkLineDirPath( cStrRef path, QLineEdit * line, cStrRef subjectName, cStrRef errorPostscript )
	{
		return m_maybeCheckLinePath( path, line, EntryType::Dir, subjectName, errorPostscript );
	}

	bool checkLineNotAFile( cStrRef path, QLineEdit * line, cStrRef subjectName, cStrRef errorPostscript )
	{
		return m_maybeCheckLineCollision( path, line, EntryType::Dir, subjectName, errorPostscript );
	}
	bool checkLineNotADir( cStrRef path, QLineEdit * line, cStrRef subjectName, cStrRef errorPostscript )
	{
		return m_maybeCheckLineCollision( path, line, EntryType::File, subjectName, errorPostscript );
	}

	template< typename ListItem >
	bool checkItemAnyPath( ListItem & item, cStrRef subjectName, cStrRef errorPostscript )
	{
		return m_maybeCheckItemPath( item, EntryType::Both, subjectName, errorPostscript );
	}
	template< typename ListItem >
	bool checkItemFilePath( ListItem & item, cStrRef subjectName, cStrRef errorPostscript )
	{
		return m_maybeCheckItemPath( item, EntryType::File, subjectName, errorPostscript );
	}
	template< typename ListItem >
	bool checkItemDirPath( ListItem & item, cStrRef subjectName, cStrRef errorPostscript )
	{
		return m_maybeCheckItemPath( item, EntryType::Dir, subjectName, errorPostscript );
	}

 public: // context-free

	static bool checkAnyPath( cStrRef path, bool showError, cStrRef subjectName, cStrRef errorPostscript )
	{
		return s_checkPath( path, EntryType::Both, showError, subjectName, errorPostscript );
	}
	static bool checkFilePath( cStrRef path, bool showError, cStrRef subjectName, cStrRef errorPostscript )
	{
		return s_checkPath( path, EntryType::File, showError, subjectName, errorPostscript );
	}
	static bool checkDirPath( cStrRef path, bool showError, cStrRef subjectName, cStrRef errorPostscript )
	{
		return s_checkPath( path, EntryType::Dir, showError, subjectName, errorPostscript );
	}

	static bool checkOnlyNonEmptyAnyPath( cStrRef path, bool showError, cStrRef subjectName, cStrRef errorPostscript )
	{
		return s_checkOnlyNonEmptyAnyPath( path, EntryType::Both, showError, subjectName, errorPostscript );
	}
	static bool checkOnlyNonEmptyFilePath( cStrRef path, bool showError, cStrRef subjectName, cStrRef errorPostscript )
	{
		return s_checkOnlyNonEmptyAnyPath( path, EntryType::File, showError, subjectName, errorPostscript );
	}
	static bool checkOnlyNonEmptyDirPath( cStrRef path, bool showError, cStrRef subjectName, cStrRef errorPostscript )
	{
		return s_checkOnlyNonEmptyAnyPath( path, EntryType::Dir, showError, subjectName, errorPostscript );
	}

	template< typename ListItem >
	static bool checkItemAnyPath( ListItem & item, bool showError, cStrRef subjectName, cStrRef errorPostscript )
	{
		return s_checkItemPath( item, EntryType::Both, showError, subjectName, errorPostscript );
	}
	template< typename ListItem >
	static bool checkItemFilePath( ListItem & item, bool showError, cStrRef subjectName, cStrRef errorPostscript )
	{
		return s_checkItemPath( item, EntryType::File, showError, subjectName, errorPostscript );
	}
	template< typename ListItem >
	static bool checkItemDirPath( ListItem & item, bool showError, cStrRef subjectName, cStrRef errorPostscript )
	{
		return s_checkItemPath( item, EntryType::Dir, showError, subjectName, errorPostscript );
	}

 private: // wrappers using verificationRequired

	bool m_maybeCheckPath( cStrRef path, EntryType expectedType, cStrRef subjectName, cStrRef errorPostscript )
	{
		if (!verificationRequired)
			return true;

		return s_checkPath( path, expectedType, errorMessageDisplayed, parent, subjectName, errorPostscript );
	}

	bool m_maybeCheckNonEmptyPath( cStrRef path, EntryType expectedType, cStrRef subjectName, cStrRef errorPostscript )
	{
		if (!verificationRequired)
			return true;

		return s_checkNonEmptyPath( path, expectedType, errorMessageDisplayed, parent, subjectName, errorPostscript );
	}

	bool m_maybeCheckCollision( cStrRef path, EntryType expectedType, cStrRef subjectName, cStrRef errorPostscript )
	{
		if (!verificationRequired)
			return true;

		return s_checkCollision( path, expectedType, errorMessageDisplayed, parent, subjectName, errorPostscript );
	}

	bool m_maybeCheckOverwrite( cStrRef path, cStrRef subjectName, cStrRef errorPostscript )
	{
		if (!verificationRequired)
			return true;

		return s_checkOverwrite( path, errorMessageDisplayed, parent, subjectName, errorPostscript );
	}

	bool m_maybeCheckLinePath(
		cStrRef path, QLineEdit * line, EntryType expectedType, cStrRef subjectName, cStrRef errorPostscript
	){
		if (!verificationRequired)
			return true;

		return s_checkLinePath( path, line, expectedType, errorMessageDisplayed, parent, subjectName, errorPostscript );
	}

	bool m_maybeCheckLineCollision(
		cStrRef path, QLineEdit * line, EntryType expectedType, cStrRef subjectName, cStrRef errorPostscript
	){
		if (!verificationRequired)
			return true;

		return s_checkLineCollision( path, line, expectedType, errorMessageDisplayed, parent, subjectName, errorPostscript );
	}

	template< typename ListItem >
	bool m_maybeCheckItemPath( ListItem & item, EntryType expectedType, cStrRef subjectName, cStrRef errorPostscript )
	{
		if (!verificationRequired)
			return true;

		return s_checkItemPath( item, expectedType, errorMessageDisplayed, parent, subjectName, errorPostscript );
	}

 private: // code de-duplication helpers

	// the actual checks, with error reporting

	static bool s_checkPath(
		cStrRef path, EntryType expectedType, bool & errorMessageDisplayed, QWidget * parent,
		cStrRef subjectName, cStrRef errorPostscript
	);

	static bool s_checkNonEmptyPath(
		cStrRef path, EntryType expectedType, bool & errorMessageDisplayed, QWidget * parent,
		cStrRef subjectName, cStrRef errorPostscript
	);

	static bool s_checkCollision(
		cStrRef path, EntryType expectedType, bool & errorMessageDisplayed, QWidget * parent,
		cStrRef subjectName, cStrRef errorPostscript
	);

	static bool s_checkExistingPathForCollision(
		cStrRef path, EntryType expectedType, bool & errorMessageDisplayed, QWidget * parent,
		cStrRef subjectName, cStrRef errorPostscript
	);

	static bool s_checkOverwrite(
		cStrRef path, bool & errorMessageDisplayed, QWidget * parent,
		cStrRef subjectName, cStrRef errorPostscript
	);

	// wrappers with invalid path highlighting

	static bool s_checkLinePath(
		cStrRef path, QLineEdit * line, EntryType expectedType, bool & errorMessageDisplayed, QWidget * parent,
		cStrRef subjectName, cStrRef errorPostscript
	){
		bool verified = s_checkPath( path, expectedType, errorMessageDisplayed, parent, subjectName, errorPostscript );
		if (!verified)
			highlightPathLineAsInvalid( line );
		else
			unhighlightPathLine( line );
		return verified;
	}

	static bool s_checkLineCollision(
		cStrRef path, QLineEdit * line, EntryType expectedType, bool & errorMessageDisplayed, QWidget * parent,
		cStrRef subjectName, cStrRef errorPostscript
	){
		bool verified = s_checkCollision( path, expectedType, errorMessageDisplayed, parent, subjectName, errorPostscript );
		if (!verified)
			highlightPathLineAsInvalid( line );
		else
			unhighlightPathLine( line );
		return verified;
	}

	template< typename ListItem >
	static bool s_checkItemPath(
		ListItem & item, EntryType expectedType, bool & errorMessageDisplayed, QWidget * parent,
		cStrRef subjectName, cStrRef errorPostscript
	){
		bool verified = s_checkPath( item.getFilePath(), expectedType, errorMessageDisplayed, parent, subjectName, errorPostscript );
		if (!verified)
			highlightListItemAsInvalid( item );
		else
			unhighlightListItem( item );
		return verified;
	}

	// even more code de-duplication

	static bool s_checkPath( cStrRef path, EntryType expectedType, bool showError, cStrRef subjectName, cStrRef errorPostscript )
	{
		bool errorMessageDisplayed = !showError;
		return s_checkPath( path, expectedType, errorMessageDisplayed, nullptr, subjectName, errorPostscript );
	}

	static bool s_checkOnlyNonEmptyAnyPath( cStrRef path, EntryType expectedType, bool showError, cStrRef subjectName, cStrRef errorPostscript )
	{
		if (path.isEmpty())
			return true;

		bool errorMessageDisplayed = !showError;
		return s_checkNonEmptyPath( path, expectedType, errorMessageDisplayed, nullptr, subjectName, errorPostscript );
	}

	template< typename ListItem >
	static bool s_checkItemPath( ListItem & item, EntryType expectedType, bool showError, cStrRef subjectName, cStrRef errorPostscript )
	{
		bool errorMessageDisplayed = !showError;
		return s_checkItemPath( item, expectedType, errorMessageDisplayed, nullptr, subjectName, errorPostscript );
	}

};


#endif // PATH_CHECK_UTILS_INCLUDED
