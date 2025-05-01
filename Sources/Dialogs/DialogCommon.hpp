//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: common base for windows/dialogs dealing with user-defined directories
//======================================================================================================================

#ifndef DIALOG_COMMON_INCLUDED
#define DIALOG_COMMON_INCLUDED


#include "Essential.hpp"

#include "Utils/FileSystemUtils.hpp"  // PathConvertor
#include "Utils/ErrorHandling.hpp"  // ErrorReportingComponent

#include <QString>
#include <QStringView>

class QWidget;
class QLineEdit;


//======================================================================================================================

/// Functionality common for all dialogs and windows.
class DialogCommon : public ErrorReportingComponent {

protected:

	DialogCommon( QWidget * self, QStringView dialogName );

};

/// Base for dialogs and windows dealing with user-defined directories.
class DialogWithPaths : public DialogCommon {

 protected:

	DialogWithPaths( QWidget * self, QStringView dialogName, PathConvertor pathConvertor )
		: DialogCommon( self, dialogName ), pathConvertor( std::move(pathConvertor) ) {}

	/// Runs a file-system dialog to let the user select a file and stores its directory for the next call.
	QString selectFile( QWidget * parent, const QString & fileDesc, QString startingDir, const QString & filter );

	/// Runs a file-system dialog to let the user select multiple files and stores their directory for the next call.
	QStringList selectFiles( QWidget * parent, const QString & fileDesc, QString startingDir, const QString & filter );

	/// Runs a file-system dialog to let the user select a directory and stores it for the next call.
	QString selectDir( QWidget * parent, const QString & dirDesc, QString startingDir = QString() );

	/// Convenience wrapper that also stores the result into a text line.
	/** Returns true if the dialog was confirmed or false if it was cancelled. */
	bool selectFile( QWidget * parent, const QString & fileDesc, QLineEdit * targetLine, const QString & filter );

	/// Convenience wrapper that also stores the result into a text line.
	/** Returns true if the dialog was confirmed or false if it was cancelled. */
	bool selectDir( QWidget * parent, const QString & dirDesc, QLineEdit * targetLine );

 public:

	/// Configures the provided QLineEdit to accept only valid file-system paths.
	static void setPathValidator( QLineEdit * pathLine );

	/// Takes a path entered by the user, cleans it from disallowed characters and converts it to the internal Qt form.
	static QString sanitizeInputPath( const QString & path );

	const QString & getLastUsedDir() const   { return lastUsedDir; }
	QString takeLastUsedDir() const          { return std::move( lastUsedDir ); }

 protected:

	PathConvertor pathConvertor;  ///< stores path settings and automatically converts paths to relative or absolute
	QString lastUsedDir;  ///< the last directory the user selected via QFileDialog

};


#endif // DIALOG_COMMON_INCLUDED
