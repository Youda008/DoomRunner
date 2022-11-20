//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: common base for windows/dialogs dealing with user-defined directories
//======================================================================================================================

#include "DialogCommon.hpp"

#include "Themes.hpp"  // updateWindowBorder
#include "OwnFileDialog.hpp"

#include <QLineEdit>


//======================================================================================================================

DialogCommon::DialogCommon( QWidget * thisWidget )
{
	// On Windows we need to manually make title bar of every new window dark, if dark theme is used.
	themes::updateWindowBorder( thisWidget );
}

QString DialogWithBrowseDir::lineEditOrLastDir( QLineEdit * line )
{
	QString lineText = line->text();
	return !lineText.isEmpty() ? lineText : lastUsedDir;
}

void DialogWithBrowseDir::browseDir( QWidget * parent, const QString & dirPurpose, QLineEdit * targetLine )
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
