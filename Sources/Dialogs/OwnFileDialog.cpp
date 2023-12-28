//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: custom QFileDialog wrapper
//======================================================================================================================

#include "OwnFileDialog.hpp"

#include "CommonTypes.hpp"  // emptyString
#include "Utils/OSUtils.hpp"  // getLinuxDesktopEnv(), getThisAppDataDir()


//======================================================================================================================
//  static file dialog wrappers

// workaround for some issues with less common Linux graphical environments
static const QFileDialog::Options disableNativeDialogOnLinux( QFileDialog::Options options )
{
 #if IS_WINDOWS
	return options;
 #else
	// initialize once on first call and then re-use.
	static const QFileDialog::Options DontUseNativeDialogOnLinux =
		os::getLinuxDesktopEnv() == "KDE" ? QFileDialog::Options() : QFileDialog::Option::DontUseNativeDialog;
	return options | DontUseNativeDialogOnLinux;
 #endif
}

static const QString & getDefaultStartingDir()
{
	// initialize once on first call and then re-use.
 #ifdef FLATPAK_BUILD
	return os::getCachedThisAppDataDir();  // should return $XDG_DATA_HOME
 #else
	return emptyString;  // let the OS choose one
 #endif
}

QString OwnFileDialog::getOpenFileName(
	QWidget * parent, const QString & caption, const QString & dir,
	const QString & filter, QString * selectedFilter, Options options
){
	const QString & startingDir = !dir.isEmpty() ? dir : getDefaultStartingDir();
	return QFileDialog::getOpenFileName( parent, caption, startingDir, filter, selectedFilter, disableNativeDialogOnLinux( options ) );
}

QStringList OwnFileDialog::getOpenFileNames(
	QWidget * parent, const QString & caption, const QString & dir,
	const QString & filter, QString * selectedFilter, Options options
){
	const QString & startingDir = !dir.isEmpty() ? dir : getDefaultStartingDir();
	return QFileDialog::getOpenFileNames( parent, caption, startingDir, filter, selectedFilter, disableNativeDialogOnLinux( options ) );
}

QString OwnFileDialog::getSaveFileName(
	QWidget * parent, const QString & caption, const QString & dir,
	const QString & filter, QString * selectedFilter, Options options
){
	const QString & startingDir = !dir.isEmpty() ? dir : getDefaultStartingDir();
	return QFileDialog::getSaveFileName( parent, caption, startingDir, filter, selectedFilter, disableNativeDialogOnLinux( options ) );
}

QString OwnFileDialog::getExistingDirectory(
	QWidget * parent, const QString & caption, const QString & dir, Options options
){
	const QString & startingDir = !dir.isEmpty() ? dir : getDefaultStartingDir();
	return QFileDialog::getExistingDirectory( parent, caption, startingDir, disableNativeDialogOnLinux( options ) );
}
