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


class DialogWithBrowseDir {

 protected:

	PathContext pathContext;  ///< stores path settings and automatically converts paths to relative or absolute
	QString lastUsedDir;  ///< the last directory the user selected via QFileDialog::getExistingDirectory()

	DialogWithBrowseDir( PathContext pathContext )
		: pathContext( std::move(pathContext) ) {}

	QString lineEditOrLastDir( QLineEdit * line );
	void browseDir( QWidget * parent, const QString & dirPurpose, QLineEdit * targetLine );

};


#endif // DIALOG_COMMON_INCLUDED
