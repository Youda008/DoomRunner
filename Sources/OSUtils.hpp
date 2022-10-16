//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: OS-specific utils
//======================================================================================================================

#ifndef OS_UTILS_INCLUDED
#define OS_UTILS_INCLUDED


#include "Common.hpp"

#include <QString>
#include <QVector>


//======================================================================================================================

inline bool isWindows()
{
 #ifdef _WIN32
	return true;
 #else
	return false;
 #endif
}

const QString & getLinuxDesktopEnv();

struct MonitorInfo
{
	QString name;
	int width;
	int height;
	bool isPrimary;
};
QVector< MonitorInfo > listMonitors();

/// Returns directory for the application to save its data into.
QString getAppDataDir();

/// Returns whether an executable is inside one of directories where the system will find it.
/** If true it means the executable can be started directly by using only its name without its path. */
bool isInSearchPath( const QString & filePath );

/// Opens a directory of a file in a new File Explorer window.
bool openFileLocation( const QString & filePath );

#ifdef _WIN32
bool createWindowsShortcut(
	QString shortcutFile, QString targetFile, QStringList targetArgs, QString workingDir = "", QString description = ""
);
#endif // _WIN32


#endif // OS_UTILS_INCLUDED
