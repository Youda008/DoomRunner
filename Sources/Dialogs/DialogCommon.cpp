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

QString DialogWithPaths::browseFile( QWidget * parent, const QString & fileDesc, QString startingDir, const QString & filter )
{
	QString path = OwnFileDialog::getOpenFileName(
		parent, "Locate the "+fileDesc, !startingDir.isEmpty() ? startingDir : lastUsedDir, filter
	);
	if (path.isEmpty())  // user probably clicked cancel
		return {};

	// the path comming out of the file dialog is always absolute
	if (pathConvertor.usingRelativePaths())
		path = pathConvertor.getRelativePath( path );

	// next time use this dir as the starting dir of the file dialog for convenience
	lastUsedDir = fs::getDirOfFile( path );

	return path;
}

QString DialogWithPaths::browseDir( QWidget * parent, const QString & dirDesc, QString startingDir )
{
	QString path = OwnFileDialog::getExistingDirectory(
		parent, "Locate the directory "+dirDesc, !startingDir.isEmpty() ? startingDir : lastUsedDir
	);
	if (path.isEmpty())  // user probably clicked cancel
		return {};

	// the path comming out of the file dialog is always absolute
	if (pathConvertor.usingRelativePaths())
		path = pathConvertor.getRelativePath( path );

	// next time use this dir as the starting dir of the file dialog for convenience
	lastUsedDir = path;

	return path;
}

void DialogWithPaths::browseFile( QWidget * parent, const QString & fileDesc, QLineEdit * targetLine, const QString & filter )
{
	QString path = browseFile( parent, fileDesc, targetLine->text(), filter );
	if (!path.isEmpty())
	{
		targetLine->setText( path );
	}
}

void DialogWithPaths::browseDir( QWidget * parent, const QString & dirDesc, QLineEdit * targetLine )
{
	QString path = browseDir( parent, dirDesc, targetLine->text() );
	if (!path.isEmpty())
	{
		targetLine->setText( path );
	}
}
