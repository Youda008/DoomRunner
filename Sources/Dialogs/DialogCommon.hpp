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

#include <QString>

class QWidget;
class QLineEdit;


//======================================================================================================================

/// Functionality common for all dialogs and windows.
class DialogCommon {

protected:

	DialogCommon( QWidget * thisWidget );

};

/// Base for dialogs and windows dealing with user-defined directories.
class DialogWithPaths : public DialogCommon {

 protected:

	DialogWithPaths( QWidget * thisWidget, PathConvertor pathConvertor )
		: DialogCommon( thisWidget ), pathConvertor( std::move(pathConvertor) ) {}

	/// Runs a file-system dialog to let the user select a file and stores the its directory for the next call.
	QString browseFile( QWidget * parent, const QString & fileDesc, QString startingDir, const QString & filter );

	/// Runs a file-system dialog to let the user select a directory and stores it for the next call.
	QString browseDir( QWidget * parent, const QString & dirDesc, QString startingDir = QString() );

	/// Convenience wrapper that also stores the result into a text line.
	void browseFile( QWidget * parent, const QString & fileDesc, QLineEdit * targetLine, const QString & filter );

	/// Convenience wrapper that also stores the result into a text line.
	void browseDir( QWidget * parent, const QString & dirDesc, QLineEdit * targetLine );

 public:

	const QString & getLastUsedDir() const   { return lastUsedDir; }
	QString takeLastUsedDir() const          { return std::move(lastUsedDir); }

 protected:

	PathConvertor pathConvertor;  ///< stores path settings and automatically converts paths to relative or absolute
	QString lastUsedDir;  ///< the last directory the user selected via QFileDialog

};


#endif // DIALOG_COMMON_INCLUDED
