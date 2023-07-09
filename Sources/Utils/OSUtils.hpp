//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: OS-specific utils
//======================================================================================================================

#ifndef OS_UTILS_INCLUDED
#define OS_UTILS_INCLUDED


#include "Essential.hpp"

#include <QString>
#include <QList>
#include <QVector>

class PathContext;


//======================================================================================================================

namespace os {


//-- standard directories and installation properties ----------------------------------------------

QString getHomeDir();

/// Returns directory for this application to save its config into.
QString getThisAppConfigDir();

/// Returns directory for this application to save its data into. This may be the same as the config dir.
QString getThisAppDataDir();

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

struct ShellCommand
{
	QString executable;
	QStringList arguments;
	QStringList extraPermissions;  // extra sandbox permissions needed to run this command
};
/// Returns a shell command needed to run a specified executable without parameters.
/** The result may be different based on operating system and where the executable is installed.
  * \param base path options with base directory for relative paths and whether paths should be quoted
  * \param dirsToBeAccessed Directories to which the executable will need a read access.
  *                         Required to setup permissions for a sandbox environment. */
ShellCommand getRunCommand(
	const QString & executablePath, const PathContext & base, const QStringList & dirsToBeAccessed = {}
);


//-- graphical environment -------------------------------------------------------------------------

#if !IS_WINDOWS
const QString & getLinuxDesktopEnv();
#endif

struct MonitorInfo
{
	QString name;
	int width;
	int height;
	bool isPrimary;
};
QVector< MonitorInfo > listMonitors();


//-- miscellaneous ---------------------------------------------------------------------------------

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


} // namespace os


#endif // OS_UTILS_INCLUDED
