//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: OS-specific utils
//======================================================================================================================

#ifndef OS_UTILS_INCLUDED
#define OS_UTILS_INCLUDED


#include "Essential.hpp"

#include "OSUtilsTypes.hpp"
class PathRebaser;

#include <QString>
#include <QList>


namespace os {


//======================================================================================================================
// file types

extern const QString scriptFileSuffix;

#if IS_WINDOWS
extern const QString shortcutFileSuffix;
#endif


//======================================================================================================================
// standard directories and installation properties

/// Returns the name of the OS user who started this process.
const QString & getUserName();

/// Returns home directory for the current process and current user.
/** NOTE: If this launcher is running in a sandbox environment such as Flatpak, this will point inside that sandbox. */
const QString & getCurrentHomeDir();

/// Returns directory where this application should store its config files.
/** NOTE: If this launcher is running in a sandbox environment such as Flatpak, this will point inside that sandbox. */
const QString & getCurrentAppConfigDir();

/// Returns directory where this application should store its internal data files that are portable to other computers.
/** NOTE: If this launcher is running in a sandbox environment such as Flatpak, this will point inside that sandbox. */
const QString & getCurrentAppDataDir();

/// Returns directory where this application should store its internal data files that are specific to this computer.
/** NOTE: If this launcher is running in a sandbox environment such as Flatpak, this will point inside that sandbox. */
const QString & getCurrentLocalAppDataDir();

/// Returns the main home directory for the current current user.
/** NOTE: If this launcher is running in a sandbox environment such as Flatpak, this will point outside of that sandbox. */
const QString & getMainHomeDir();

/// Returns the main directory where applications should store their config files.
/** NOTE: If this launcher is running in a sandbox environment such as Flatpak, this will point outside of that sandbox. */
const QString & getMainAppConfigDir();

/// Returns the main directory where applications should store their internal data files that are portable to other computers.
/** NOTE: If this launcher is running in a sandbox environment such as Flatpak, this will point outside of that sandbox. */
const QString & getMainAppDataDir();

/// Returns the main directory where applications should store their internal data files that are specific to this computer.
/** NOTE: If this launcher is running in a sandbox environment such as Flatpak, this will point outside of that sandbox. */
const QString & getMainLocalAppDataDir();

#if IS_WINDOWS

/// Returns directory for document files of the current user.
const QString & getDocumentsDir();

/// Returns directory for image files of the current user.
const QString & getPicturesDir();

/// Returns directory for game saves of the current user.
const QString & getSavedGamesDir();

#endif

/// Returns directory where a selected application should store its config files.
QString getHomeDirForApp( const QString & executablePath );

/// Returns directory where a selected application should store its config files.
QString getConfigDirForApp( const QString & executablePath );

/// Returns directory where a selected application should store its data files.
QString getDataDirForApp( const QString & executablePath );

/// Returns directory where this launcher should store its data files.
const QString & getThisLauncherDataDir();


// other

/// Returns whether an executable is inside one of the directories where the system will find it.
/** True means the executable can be started directly by using only its name without its path. */
bool isInSearchPath( const QString & filePath );


// installation properties

/// Returns application info that can be deduced from the executable path or extracted from the executable file.
/** This may open and read the executable file, which may be a time-expensive operation. */
AppInfo getAppInfo( const QString & executablePath );

/// Returns a shell command needed to run a specified executable without parameters.
/** The result may be different based on operating system and where the executable is installed.
  * \param executablePath path to the executable that's either absolute or relative to the current working dir
  * \param runnersDirRebaser path rebaser set up to rebase relative paths from current working dir to a working dir
  *                          from which the command will be executed.
  * \param dirsToBeAccessed Directories to which the executable will need a read access.
  *                         Required to setup permissions for a sandbox environment. */
ShellCommand getRunCommand(
	const QString & executablePath, const PathRebaser & runnersDirRebaser, bool forceExeName,
	const QStringList & dirsToBeAccessed = {}
);


//======================================================================================================================
// graphical environment

const QString & getLinuxDesktopEnv();

QList< MonitorInfo > listMonitors();


//======================================================================================================================
// miscellaneous

/// Opens a selected directory in a new File Explorer window.
bool openDirectoryWindow( const QString & dirPath );

/// Opens a directory of a file in a new File Explorer window.
bool openFileLocation( const QString & filePath );

/// Opens a selected file in the application that's assigned for this file type.
bool openFileInDefaultApp( const QString & filePath );

/// Opens a selected file in the system's main notepad.
bool openFileInNotepad( const QString & filePath );


} // namespace os


//======================================================================================================================
// Windows-specific

#if IS_WINDOWS
namespace win {

/// Creates a Windows shortcut to an executable with arguments.
/** \param shortcutFile Path to the shortcut file to be created.
  * \param targetFile Path to the file the shortcut will point to.
  *                   Must be either absolute or relative to the current working directory of this running application.
  * \param targetArgs Command-line arguments for the targetFile, if it's an executable.
  *                   If the arguments contain file path, they must be relative to the workingDir. */
bool createShortcut(
	QString shortcutFile, QString targetFile, QStringList targetArgs, QString workingDir = {}, QString description = {}
);

} // namespace win
#endif // IS_WINDOWS


#endif // OS_UTILS_INCLUDED
