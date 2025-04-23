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
#include <QRegularExpressionValidator>


//======================================================================================================================

DialogCommon::DialogCommon( QWidget * self, QStringView dialogName )
:
	ErrorReportingComponent( self, dialogName )
{
	// On Windows we need to manually make title bar of every new window dark, if dark theme is used.
	themes::updateWindowBorder( self );
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
	lastUsedDir = fs::getParentDir( path );

	return path;
}

QStringList DialogWithPaths::browseFiles( QWidget * parent, const QString & fileDesc, QString startingDir, const QString & filter )
{
	QStringList paths = OwnFileDialog::getOpenFileNames(
		parent, "Locate the "+fileDesc, !startingDir.isEmpty() ? startingDir : lastUsedDir, filter
	);
	if (paths.isEmpty())  // user probably clicked cancel
		return {};

	// the paths comming out of the file dialog are always absolute
	if (pathConvertor.usingRelativePaths())
		for (QString & path : paths)
			path = pathConvertor.getRelativePath( path );

	// next time use this dir as the starting dir of the file dialog for convenience
	lastUsedDir = fs::getParentDir( paths.first() );

	return paths;
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

bool DialogWithPaths::browseFile( QWidget * parent, const QString & fileDesc, QLineEdit * targetLine, const QString & filter )
{
	QString path = browseFile( parent, fileDesc, targetLine->text(), filter );
	bool confirmed = !path.isEmpty();  // user may have clicked cancel
	if (confirmed)
	{
		targetLine->setText( path );
	}
	return confirmed;
}

bool DialogWithPaths::browseDir( QWidget * parent, const QString & dirDesc, QLineEdit * targetLine )
{
	QString path = browseDir( parent, dirDesc, targetLine->text() );
	bool confirmed = !path.isEmpty();  // user may have clicked cancel
	if (confirmed)
	{
		targetLine->setText( path );
	}
	return confirmed;
}

void DialogWithPaths::setPathValidator( QLineEdit * pathLine )
{
	pathLine->setValidator( new QRegularExpressionValidator( fs::getPathRegex(), pathLine ) );
}

QString DialogWithPaths::sanitizeInputPath( const QString & path )
{
	return fs::fromNativePath( fs::sanitizePath( path ) );
}
