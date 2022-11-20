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

#ifdef _WIN32
	#define IS_WINDOWS true
#else
	#define IS_WINDOWS false
#endif

inline constexpr bool isWindows()
{
 #if IS_WINDOWS
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

/// Returns directory for this application to save its data into.
QString getThisAppDataDir();

/// Returns directory for any application to save its data into.
QString getAppDataDir( const QString & executablePath );

/// Returns whether an executable is inside one of directories where the system will find it.
/** If true it means the executable can be started directly by using only its name without its path. */
bool isInSearchPath( const QString & filePath );

/// Opens a directory of a file in a new File Explorer window.
bool openFileLocation( const QString & filePath );

#if IS_WINDOWS
bool createWindowsShortcut(
	QString shortcutFile, QString targetFile, QStringList targetArgs, QString workingDir = "", QString description = ""
);
#endif // IS_WINDOWS


#endif // OS_UTILS_INCLUDED
