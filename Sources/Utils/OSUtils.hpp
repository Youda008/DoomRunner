//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: OS-specific utils
//======================================================================================================================

#ifndef OS_UTILS_INCLUDED
#define OS_UTILS_INCLUDED


#include "Essential.hpp"
#include "CommonTypes.hpp"

#include <QString>
#include <QList>
#include <QVector>

class PathRebaser;


namespace os {


//======================================================================================================================
//  standard directories and installation properties

/// Returns home directory for the current user.
QString getHomeDir();

/// Returns directory for document files of the current user.
QString getDocumentsDir();

#if IS_WINDOWS
/// Returns directory for game saves of the current user.
QString getSavedGamesDir();
#endif

/// Returns parent directory where applications should store their config files.
QString getAppConfigDir();

/// Returns parent directory where applications should store their data files.
QString getAppDataDir();

/// Returns directory where selected application should store its config files.
QString getConfigDirForApp( const QString & executablePath );

/// Returns directory where selected application should store its data files.
QString getDataDirForApp( const QString & executablePath );

/// Returns directory where this application should save its config files.
QString getThisAppConfigDir();

/// Returns directory where this application should save its data files. This may be the same as the config dir.
QString getThisAppDataDir();


// cached variants of the functions above for standard directories that might be expensive to get

const QString & getCachedHomeDir();
const QString & getCachedDocumentsDir();
#if IS_WINDOWS
const QString & getCachedSavedGamesDir();
#endif
const QString & getCachedAppConfigDir();
const QString & getCachedAppDataDir();
QString getCachedConfigDirForApp( const QString & executablePath );
QString getCachedDataDirForApp( const QString & executablePath );
const QString & getCachedThisAppConfigDir();
const QString & getCachedThisAppDataDir();


// other

/// Returns whether an executable is inside one of directories where the system will find it.
/** If true it means the executable can be started directly by using only its name without its path. */
bool isInSearchPath( const QString & filePath );


// installation properties

/// Type of sandbox environment an application might be installed in
enum class SandboxEnv
{
	None,
	Snap,
	Flatpak,
};
QString getSandboxName( SandboxEnv sandbox );

struct SandboxEnvInfo
{
	SandboxEnv type;   ///< sandbox environment type determined from path
	QString appName;   ///< name which the sandbox uses to identify the application
};
SandboxEnvInfo getSandboxEnvInfo( const QString & executablePath );

struct ShellCommand
{
	QString executable;
	QStringVec arguments;
	QStringVec extraPermissions;  ///< extra sandbox environment permissions needed to run this command
};
/// Returns a shell command needed to run a specified executable without parameters.
/** The result may be different based on operating system and where the executable is installed.
  * \param executablePath path to the executable that's either absolute or relative to the current working dir
  * \param rebaser path convertor set up to rebase relative paths from current working dir to a working dir
  *                from which the process will be started
  * \param dirsToBeAccessed Directories to which the executable will need a read access.
  *                         Required to setup permissions for a sandbox environment. */
ShellCommand getRunCommand(
	const QString & executablePath, const PathRebaser & currentDirToNewWorkingDir, const QStringVec & dirsToBeAccessed = {}
);


//======================================================================================================================
//  graphical environment

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


//======================================================================================================================
//  miscellaneous

/// Opens a selected directory in a new File Explorer window.
bool openDirectoryWindow( const QString & dirPath );

/// Opens a directory of a file in a new File Explorer window.
bool openFileLocation( const QString & filePath );

struct EnvVar
{
	QString name;
	QString value;
};


} // namespace os


//======================================================================================================================
//  Windows-specific

#if IS_WINDOWS
namespace win {

/// Creates a Windows shortcut to an executable with arguments.
/** \param shortcutFile Path to the shortcut file to be created.
  * \param targetFile Path to the file the shortcut will point to.
  *                   Must be either absolute or relative to the current working directory of this running application.
  * \param targetArgs Command-line arguments for the targetFile, if it's an executable.
  *                   If the arguments contain file path, they must be relative to the workingDir. */
bool createShortcut(
	QString shortcutFile, QString targetFile, QStringVec targetArgs, QString workingDir = {}, QString description = {}
);

} // namespace win
#endif // IS_WINDOWS


#endif // OS_UTILS_INCLUDED
