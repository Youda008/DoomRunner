//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: common base for windows/dialogs dealing with user-defined directories
//======================================================================================================================

#ifndef DIALOG_COMMON_INCLUDED
#define DIALOG_COMMON_INCLUDED


#include "Common.hpp"

#include "Utils/FileSystemUtils.hpp"  // PathContext

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
class DialogWithBrowseDir : public DialogCommon {

 protected:

	PathContext pathContext;  ///< stores path settings and automatically converts paths to relative or absolute
	QString lastUsedDir;  ///< the last directory the user selected via QFileDialog::getExistingDirectory()

	DialogWithBrowseDir( QWidget * thisWidget, PathContext pathContext )
		: DialogCommon( thisWidget ), pathContext( std::move(pathContext) ) {}

	void browseDir( QWidget * parent, const QString & dirPurpose, QLineEdit * targetLine );

 private:

	QString lineEditOrLastDir( QLineEdit * line );

};


#endif // DIALOG_COMMON_INCLUDED
