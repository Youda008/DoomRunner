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

static const QFileDialog::Options DontUseNativeDialogOnLinux
	= (isWindows() || getLinuxDesktopEnv() == "KDE") ? QFileDialog::Options() : QFileDialog::Option::DontUseNativeDialog;

QString OwnFileDialog::getOpenFileName(
	QWidget * parent, const QString & caption, const QString & dir,
	const QString & filter, QString * selectedFilter, Options options
){
	return QFileDialog::getOpenFileName( parent, caption, dir, filter, selectedFilter, options | DontUseNativeDialogOnLinux );
}

QStringList OwnFileDialog::getOpenFileNames(
	QWidget * parent, const QString & caption, const QString & dir,
	const QString & filter, QString * selectedFilter, Options options
){
	return QFileDialog::getOpenFileNames( parent, caption, dir, filter, selectedFilter, options | DontUseNativeDialogOnLinux );
}

QString OwnFileDialog::getSaveFileName(
	QWidget * parent, const QString & caption, const QString & dir,
	const QString & filter, QString * selectedFilter, Options options
){
	return QFileDialog::getSaveFileName( parent, caption, dir, filter, selectedFilter, options | DontUseNativeDialogOnLinux );
}

QString OwnFileDialog::getExistingDirectory(
	QWidget * parent, const QString & caption, const QString & dir, Options options
){
	return QFileDialog::getExistingDirectory( parent, caption, dir, options | DontUseNativeDialogOnLinux );
}
