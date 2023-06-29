//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: custom QFileDialog wrapper, workaround for some issues with less common Linux graphical environments
//======================================================================================================================

#include "OwnFileDialog.hpp"

#include "Utils/OSUtils.hpp"  // isWindows


//======================================================================================================================
//  static file dialog wrappers

static const QFileDialog::Options disableNativeDialogOnLinux( QFileDialog::Options options )
{
	// initialize once on first call and then re-use.
	static const QFileDialog::Options DontUseNativeDialogOnLinux =
		(isWindows() || getLinuxDesktopEnv() == "KDE") ? QFileDialog::Options() : QFileDialog::Option::DontUseNativeDialog;
	return options | DontUseNativeDialogOnLinux;
}

QString OwnFileDialog::getOpenFileName(
	QWidget * parent, const QString & caption, const QString & dir,
	const QString & filter, QString * selectedFilter, Options options
){
	return QFileDialog::getOpenFileName( parent, caption, dir, filter, selectedFilter, disableNativeDialogOnLinux( options ) );
}

QStringList OwnFileDialog::getOpenFileNames(
	QWidget * parent, const QString & caption, const QString & dir,
	const QString & filter, QString * selectedFilter, Options options
){
	return QFileDialog::getOpenFileNames( parent, caption, dir, filter, selectedFilter, disableNativeDialogOnLinux( options ) );
}

QString OwnFileDialog::getSaveFileName(
	QWidget * parent, const QString & caption, const QString & dir,
	const QString & filter, QString * selectedFilter, Options options
){
	return QFileDialog::getSaveFileName( parent, caption, dir, filter, selectedFilter, disableNativeDialogOnLinux( options ) );
}

QString OwnFileDialog::getExistingDirectory(
	QWidget * parent, const QString & caption, const QString & dir, Options options
){
	return QFileDialog::getExistingDirectory( parent, caption, dir, disableNativeDialogOnLinux( options ) );
}
