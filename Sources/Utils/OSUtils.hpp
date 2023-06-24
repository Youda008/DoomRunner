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

QString getHomeDir();

/// Returns directory for this application to save its data into.
QString getThisAppDataDir();

/// Returns directory for any application to save its data into.
QString getAppDataDir( const QString & executablePath );

/// Returns whether an executable is inside one of directories where the system will find it.
/** If true it means the executable can be started directly by using only its name without its path. */
bool isInSearchPath( const QString & filePath );

enum class Sandbox
{
	None,
	Snap,
	Flatpak,
};
QString getSandboxName( Sandbox sandbox );

struct ExecutableTraits
{
	QString executableBaseName;   ///< executable name without file suffix
	Sandbox sandboxEnv;
	QString sandboxAppName;   ///< application name which the sandbox uses to start it
};
ExecutableTraits getExecutableTraits( const QString & executablePath );

/// Opens a directory of a file in a new File Explorer window.
bool openFileLocation( const QString & filePath );

#if IS_WINDOWS
/// Creates a Windows shortcut to an executable with arguments.
/** \param shortcutFile Path to the shortcut file to be created.
  * \param targetFile Path to the file the shortcut will point to.
  *                   Must be either absolute or relative to the current working directory of this running application.
  * \param targetArgs Command-line arguments for the targetFile, if it's an executable.
  *                   If the arguments contain file path, they must be relative to the workingDir. */
bool createWindowsShortcut(
	QString shortcutFile, QString targetFile, QStringList targetArgs, QString workingDir = "", QString description = ""
);
#endif // IS_WINDOWS


#endif // OS_UTILS_INCLUDED
