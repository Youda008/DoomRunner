//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: OS-specific utils
//======================================================================================================================

#include "OSUtils.hpp"

#include "FileSystemUtils.hpp"
#include "ExeReader.hpp"
#include "ErrorHandling.hpp"

#include <QStandardPaths>
#include <QApplication>
#include <QGuiApplication>
#include <QDesktopServices>  // fallback for openFileLocation
#include <QUrl>
#include <QStringBuilder>
#include <QRegularExpression>
#include <QScreen>
#include <QProcess>

#if IS_WINDOWS
	#include <windows.h>
	#include <shlobj.h>
#endif

#include <memory>


namespace os {


//======================================================================================================================
// file types

#if IS_WINDOWS
	const QString scriptFileSuffix = "*.bat";
	const QString shortcutFileSuffix = "*.lnk";
#else
	const QString scriptFileSuffix = "*.sh";
#endif


//======================================================================================================================
// standard directories

#if IS_FLATPAK_BUILD && IS_WINDOWS
	#error "Flatpak build on Windows is not supported"
#elif IS_FLATPAK_BUILD && IS_MACOS
	#error "Flatpak build on MacOS is not supported"
#endif

namespace impl {

static QString getUserName()
{
	// There is no other way: https://stackoverflow.com/questions/26552517/get-system-username-in-qt/49215652#49215652
	if constexpr (IS_WINDOWS)
		return qEnvironmentVariable("USERNAME");
	else
		return qEnvironmentVariable("USER");
}

// Here is where QStandardPaths point to on different platforms:
// https://docs.google.com/spreadsheets/d/11UJYZAUbFi-B7oIQ9egNYbgC8hsS9PTyyckT5g0uh08/edit?usp=sharing

static QString getCurrentHomeDir()
{
	return QStandardPaths::writableLocation( QStandardPaths::HomeLocation );
	// result:
	// Windows - system-wide:  C:/Users/Youda                                                    - %UserProfile%
	// Linux - system-wide:    /home/youda                                                       \
	// Linux - Flatpak:        /home/youda/.var/app/io.github.Youda008.DoomRunner                - $HOME
	// Linux - Snap:           /home/youda/snap/gzdoom/current                                   /
	// Mac - system-wide:      /Users/Youda
}

[[maybe_unused]] static QString getMainHomeDir()
{
	if constexpr (IS_FLATPAK_BUILD)  // this is a Flatpak installation of this launcher
	{
		// Inside Flatpak environment the QStandardPaths point into the Flatpak sandbox of this application.
		// But we need the system-wide home dir, and that's not available via Qt, so we must do this hack.
		return "/home/"%getUserName();
	}
	else
	{
		return getCurrentHomeDir();
	}
}

static QString getCurrentAppConfigDir()
{
	if constexpr (IS_WINDOWS)
	{
		// Qt thinks that GenericConfigLocation on Windows belongs to AppData\Local, but imo it belongs to AppData\Roaming.
		// Unfortunatelly there is no GenericRoamingDataLocation, and the only way to extract that roaming parent directory
		// is to take the parent directory of AppDataLocation, which already includes this application name.
		return fs::getParentDir( QStandardPaths::writableLocation( QStandardPaths::AppDataLocation ) );
	}
	else // Linux and Mac
	{
		return QStandardPaths::writableLocation( QStandardPaths::GenericConfigLocation );
	}
	// result:
	// Windows - system-wide:  C:/Users/Youda/AppData/Roaming                                    - %AppData%
	// Linux - system-wide:    /home/youda/.config                                               \
	// Linux - Flatpak:        /home/youda/.var/app/io.github.Youda008.DoomRunner/.config        - $XDG_CONFIG_HOME
	// Linux - Snap:           /home/youda/snap/gzdoom/current/.config                           /
	// Mac - system-wide:      /Users/Youda/Library/Preferences
}

static QString getAppConfigDirRelativeToHome()
{
	// Takes current home dir and "subtracts" it from current config dir.
	// e.g.: "/home/youda/snap/gzdoom/current/.config" - "/home/youda/snap/gzdoom/current" -> ".config"
	return QDir( getCurrentHomeDir() ).relativeFilePath( getCurrentAppConfigDir() );
}

[[maybe_unused]] static QString getMainAppConfigDir()
{
	if constexpr (IS_FLATPAK_BUILD)  // this is a Flatpak installation of this launcher
	{
		// Inside Flatpak environment the QStandardPaths point into the Flatpak sandbox of this application.
		// But we need the system-wide config dir, and that's not available via Qt, so we must do this hack.
		return getMainHomeDir()%"/"%getAppConfigDirRelativeToHome();
	}
	else
	{
		return getCurrentAppConfigDir();
	}
}

static QString getCurrentAppDataDir()
{
	if constexpr (IS_WINDOWS)
	{
		// Qt thinks that GenericDataLocation on Windows belongs to AppData\Local, but imo it belongs to AppData\Roaming.
		// Unfortunatelly there is no GenericRoamingDataLocation, and the only way to extract that roaming parent directory
		// is to take the parent directory of AppDataLocation, which already includes this application name.
		return fs::getParentDir( QStandardPaths::writableLocation( QStandardPaths::AppDataLocation ) );
	}
	else // Linux and Mac
	{
		return QStandardPaths::writableLocation( QStandardPaths::GenericDataLocation );
	}
	// result:
	// Windows - system-wide:  C:/Users/Youda/AppData/Roaming                                    - %AppData%
	// Linux - system-wide:    /home/youda/.local/share                                          \
	// Linux - Flatpak:        /home/youda/.var/app/io.github.Youda008.DoomRunner/.local/share   - $XDG_DATA_HOME
	// Linux - Snap:           /home/youda/snap/gzdoom/current/.local/share                      /
	// Mac - system-wide:      /Users/Youda/Library/Application Support
}

static QString getAppDataDirRelativeToHome()
{
	// Takes current home dir and "subtracts" it from current data dir.
	// e.g.: "/home/youda/snap/gzdoom/current/.local/share" - "/home/youda/snap/gzdoom/current" -> ".local/share"
	return QDir( getCurrentHomeDir() ).relativeFilePath( getCurrentAppDataDir() );
}

[[maybe_unused]] static QString getMainAppDataDir()
{
	if constexpr (IS_FLATPAK_BUILD)  // this is a Flatpak installation of this launcher
	{
		// Inside Flatpak environment the QStandardPaths point into the Flatpak sandbox of this application.
		// But we need the system-wide data dir, and that's not available via Qt, so we must do this hack.
		return getMainHomeDir()%"/"%getAppDataDirRelativeToHome();
	}
	else
	{
		return getCurrentAppDataDir();
	}
}

static QString getCurrentLocalAppDataDir()
{
	return QStandardPaths::writableLocation( QStandardPaths::GenericDataLocation );
	// result:
	// Windows - system-wide:  C:/Users/Youda/AppData/Local                                      - %LocalAppData%
	// Linux - system-wide:    /home/youda/.local/share                                          \
	// Linux - Flatpak:        /home/youda/.var/app/io.github.Youda008.DoomRunner/.local/share   - $XDG_DATA_HOME
	// Linux - Snap:           /home/youda/snap/gzdoom/current/.local/share                      /
	// Mac - system-wide:      /Users/Youda/Library/Application Support
}

static QString getLocalAppDataDirRelativeToHome()
{
	// Takes current home dir and "subtracts" it from current data dir.
	// e.g.: "/home/youda/snap/gzdoom/current/.local/share" - "/home/youda/snap/gzdoom/current" -> ".local/share"
	return QDir( getCurrentHomeDir() ).relativeFilePath( getCurrentLocalAppDataDir() );
}

[[maybe_unused]] static QString getMainLocalAppDataDir()
{
	if constexpr (IS_FLATPAK_BUILD)  // this is a Flatpak installation of this launcher
	{
		// Inside Flatpak environment the QStandardPaths point into the Flatpak sandbox of this application.
		// But we need the system-wide data dir, and that's not available via Qt, so we must do this hack.
		return getMainHomeDir()%"/"%getLocalAppDataDirRelativeToHome();
	}
	else
	{
		return getCurrentLocalAppDataDir();
	}
}

#if IS_WINDOWS

static QString getDocumentsDir()
{
	return QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation );
	// result:
	// Windows - system-wide:  C:/Users/Youda/Documents/ㅤ
}

static QString getPicturesDir()
{
	return QStandardPaths::writableLocation( QStandardPaths::PicturesLocation );
	// result:
	// Windows - system-wide:  C:/Users/Youda/Pictures/ㅤ
}

static QString getSavedGamesDir()
{
	PWSTR pszPath = nullptr;
	HRESULT hr = SHGetKnownFolderPath( FOLDERID_SavedGames, KF_FLAG_DONT_UNEXPAND, nullptr, &pszPath );
	if (FAILED(hr) || !pszPath)
	{
		auto lastError = GetLastError();
		logRuntimeError() << "Cannot get Saved Games location, SHGetKnownFolderPath() failed with error "<<lastError;
		return {};
	}
	auto dir = QString::fromWCharArray( pszPath );
	CoTaskMemFree( pszPath );
	dir.replace('\\', '/');  // Qt internally uses '/' for all platforms
	return dir;
	// result:
	// Windows - system-wide:  C:/Users/Youda/Saved Gamesㅤ
}

#endif // IS_WINDOWS

QString getThisLauncherDataDir()
{
	// mimic ZDoom behaviour - save to application's binary dir on Windows, but to standard data directory on Linux

	QString appDataDir = QStandardPaths::writableLocation( QStandardPaths::AppDataLocation );

	if constexpr (IS_WINDOWS)
	{
		QString thisExeDir = QApplication::applicationDirPath();
		if (fs::isDirectoryWritable( thisExeDir ))
		{
			printInfo() << "Saving data (options, cache, errors) into the install directory ("<<thisExeDir<<")";
			return thisExeDir;
		}
		else  // if we cannot write to the directory where the exe is extracted (e.g. Program Files), fallback to %AppData%\Roaming
		{
			printInfo() << "The install directory ("<<thisExeDir<<") is not writable.";
			printInfo() << "Saving data (options, cache, errors) into the system standard directory ("<<appDataDir<<")";
			return appDataDir;
		}
	}
	else
	{
		printInfo() << "Saving data (options, cache, errors) into the system standard directory ("<<appDataDir<<")";
		return appDataDir;
	}

	// result:
	// Windows - Program Files:  C:/Users/Youda/AppData/Roaming/DoomRunner                                    - %AppData%
	// Windows - custom dir:     E:/Youda/Games/Doom/DoomRunner
	// Linux - system-wide:      /home/youda/.local/share/DoomRunner                                          \
	// Linux - Flatpak:          /home/youda/.var/app/io.github.Youda008.DoomRunner/.local/share/DoomRunner   - $XDG_DATA_HOME
	// Linux - Snap:             /home/youda/snap/gzdoom/current/.local/share/DoomRunner                      /
	// Mac - system-wide:        /Users/Youda/Library/Application Support/DoomRunner
}

} // namespace impl


//-- result caching --------------------------------------------------------------------------------
// These directories are unlikely to change, so we init them once and then re-use the result.
// We don't use local static variables, because those use a mutex to prevent initialization by multiple threads.
// These functions will however always be used from the main thread only, so mutex is not needed.

struct SystemDirectories
{
	QString userName;

	QString currentHomeDir;
	QString currentAppConfigDir;
	QString currentAppDataDir;
	QString currentLocalAppDataDir;
 #if IS_FLATPAK_BUILD
	QString mainHomeDir;
	QString mainAppConfigDir;
	QString mainAppDataDir;
	QString mainLocalAppDataDir;
 #endif
	QString appConfigDirRelativeToHome;
	QString appDataDirRelativeToHome;
	QString localAppDataDirRelativeToHome;
 #if IS_WINDOWS
	QString documentsDir;
	QString picturesDir;
	QString savedGamesDir;
 #endif
	QString thisLauncherDataDir;
};

static std::unique_ptr< SystemDirectories > g_cachedDirs;

static std::unique_ptr< SystemDirectories > getSystemDirectories()
{
	auto dirs = std::make_unique< SystemDirectories >();

	dirs->userName                = impl::getUserName();

	dirs->currentHomeDir          = impl::getCurrentHomeDir();
	dirs->currentAppConfigDir     = impl::getCurrentAppConfigDir();
	dirs->currentAppDataDir       = impl::getCurrentAppDataDir();
	dirs->currentLocalAppDataDir  = impl::getCurrentLocalAppDataDir();
 #if IS_FLATPAK_BUILD
	dirs->mainHomeDir             = impl::getMainHomeDir();
	dirs->mainAppConfigDir        = impl::getMainAppConfigDir();
	dirs->mainAppDataDir          = impl::getMainAppDataDir();
	dirs->mainLocalAppDataDir     = impl::getMainLocalAppDataDir();
 #endif
	dirs->appConfigDirRelativeToHome    = impl::getAppConfigDirRelativeToHome();
	dirs->appDataDirRelativeToHome      = impl::getAppDataDirRelativeToHome();
	dirs->localAppDataDirRelativeToHome = impl::getLocalAppDataDirRelativeToHome();
 #if IS_WINDOWS
	dirs->documentsDir            = impl::getDocumentsDir();
	dirs->picturesDir             = impl::getPicturesDir();
	dirs->savedGamesDir           = impl::getSavedGamesDir();
 #endif
	dirs->thisLauncherDataDir     = impl::getThisLauncherDataDir();

	return dirs;
}

const QString & getUserName()
{
	if (!g_cachedDirs)
		g_cachedDirs = getSystemDirectories();
	return g_cachedDirs->userName;
}

const QString & getCurrentHomeDir()
{
	if (!g_cachedDirs)
		g_cachedDirs = getSystemDirectories();
	return g_cachedDirs->currentHomeDir;
}
const QString & getCurrentAppConfigDir()
{
	if (!g_cachedDirs)
		g_cachedDirs = getSystemDirectories();
	return g_cachedDirs->currentAppConfigDir;
}
const QString & getCurrentAppDataDir()
{
	if (!g_cachedDirs)
		g_cachedDirs = getSystemDirectories();
	return g_cachedDirs->currentAppDataDir;
}
const QString & getCurrentLocalAppDataDir()
{
	if (!g_cachedDirs)
		g_cachedDirs = getSystemDirectories();
	return g_cachedDirs->currentLocalAppDataDir;
}

const QString & getMainHomeDir()
{
	if (!g_cachedDirs)
		g_cachedDirs = getSystemDirectories();
 #if IS_FLATPAK_BUILD
	return g_cachedDirs->mainHomeDir;
 #else
	return g_cachedDirs->currentHomeDir;
 #endif
}
const QString & getMainAppConfigDir()
{
	if (!g_cachedDirs)
		g_cachedDirs = getSystemDirectories();
 #if IS_FLATPAK_BUILD
	return g_cachedDirs->mainAppConfigDir;
 #else
	return g_cachedDirs->currentAppConfigDir;
 #endif
}
const QString & getMainAppDataDir()
{
	if (!g_cachedDirs)
		g_cachedDirs = getSystemDirectories();
 #if IS_FLATPAK_BUILD
	return g_cachedDirs->mainAppDataDir;
 #else
	return g_cachedDirs->currentAppDataDir;
 #endif
}
const QString & getMainLocalAppDataDir()
{
	if (!g_cachedDirs)
		g_cachedDirs = getSystemDirectories();
 #if IS_FLATPAK_BUILD
	return g_cachedDirs->mainLocalAppDataDir;
 #else
	return g_cachedDirs->currentLocalAppDataDir;
 #endif
}

static const QString & getAppConfigDirRelativeToHome()
{
	if (!g_cachedDirs)
		g_cachedDirs = getSystemDirectories();
	return g_cachedDirs->appConfigDirRelativeToHome;
}
static const QString & getAppDataDirRelativeToHome()
{
	if (!g_cachedDirs)
		g_cachedDirs = getSystemDirectories();
	return g_cachedDirs->appDataDirRelativeToHome;
}

#if IS_WINDOWS

const QString & getDocumentsDir()
{
	if (!g_cachedDirs)
		g_cachedDirs = getSystemDirectories();
	return g_cachedDirs->documentsDir;
}
const QString & getPicturesDir()
{
	if (!g_cachedDirs)
		g_cachedDirs = getSystemDirectories();
	return g_cachedDirs->picturesDir;
}
const QString & getSavedGamesDir()
{
	if (!g_cachedDirs)
		g_cachedDirs = getSystemDirectories();
	return g_cachedDirs->savedGamesDir;
}

#endif // IS_WINDOWS

const QString & getThisLauncherDataDir()
{
	if (!g_cachedDirs)
		g_cachedDirs = getSystemDirectories();
	return g_cachedDirs->thisLauncherDataDir;
}

static SandboxEnvInfo getSandboxEnvInfo( const QString & executablePath );

QString getHomeDirForApp( const QString & executablePath )
{
	QString homeDir;
	SandboxEnvInfo sandboxEnv = getSandboxEnvInfo( executablePath );
	if (sandboxEnv.type != SandboxType::None)
	{
		homeDir = sandboxEnv.homeDir;
	}
	else
	{
		homeDir = getMainHomeDir();
	}
	return homeDir;
	// result:
	// Windows - system-wide:  C:/Users/Youda
	// Linux - system-wide:    /home/youda
	// Linux - Flatpak:        /home/youda/.var/app/org.zdoom.GZDoom
	// Linux - Snap:           /home/youda/snap/gzdoom/current
	// Mac - system-wide:      /Users/Youda
}

QString getConfigDirForApp( const QString & executablePath )
{
	QString exeName = fs::getFileBasenameFromPath( executablePath );
	QString configDir;
	SandboxEnvInfo sandboxEnv = getSandboxEnvInfo( executablePath );
	if (sandboxEnv.type != SandboxType::None)
	{
		configDir = sandboxEnv.homeDir%"/"%getAppConfigDirRelativeToHome();
	}
	else
	{
		configDir = getMainAppConfigDir();
	}
	return configDir%"/"%exeName;
	// result:
	// Windows - system-wide:  C:/Users/Youda/AppData/Roaming/gzdoom
	// Linux - system-wide:    /home/youda/.config/gzdoom
	// Linux - Flatpak:        /home/youda/.var/app/org.zdoom.GZDoom/.config/gzdoom
	// Linux - Snap:           /home/youda/snap/gzdoom/current/.config/gzdoom
	// Mac - system-wide:      /Users/Youda/Library/Preferences/gzdoom
}

QString getDataDirForApp( const QString & executablePath )
{
	QString exeName = fs::getFileBasenameFromPath( executablePath );
	QString dataDir;
	SandboxEnvInfo sandboxEnv = getSandboxEnvInfo( executablePath );
	if (sandboxEnv.type != SandboxType::None)
	{
		dataDir = sandboxEnv.homeDir%"/"%getAppDataDirRelativeToHome();
	}
	else
	{
		dataDir = getMainAppDataDir();
	}
	return dataDir%"/"%exeName;
	// result:
	// Windows - system-wide:  C:/Users/Youda/AppData/Roaming/gzdoom
	// Linux - system-wide:    /home/youda/.local/share/gzdoom
	// Linux - Flatpak:        /home/youda/.var/app/org.zdoom.GZDoom/.local/share/gzdoom
	// Linux - Snap:           /home/youda/snap/gzdoom/current/.local/share/gzdoom
	// Mac - system-wide:      /Users/Youda/Library/Application Support/gzdoom
}


//-- misc ------------------------------------------------------------------------------------------

bool isInSearchPath( const QString & filePath )
{
	return QStandardPaths::findExecutable( fs::getFileNameFromPath( filePath ) ) == filePath;
}


//-- installation properties -----------------------------------------------------------------------

static const QRegularExpression snapPathRegex("^/snap/([^/]+)/");
static const QRegularExpression flatpakPathRegex("^/var/lib/flatpak/app/([^/]+)/");

static SandboxEnvInfo getSandboxEnvInfo( const QString & executablePath )
{
	SandboxEnvInfo sandboxEnv;

	QString absoluteExePath = fs::getAbsolutePath( executablePath );

	QRegularExpressionMatch match;
	if ((match = snapPathRegex.match( absoluteExePath )).hasMatch())
	{
		sandboxEnv.type = SandboxType::Snap;
		sandboxEnv.appName = match.captured(1);
		sandboxEnv.homeDir = getMainHomeDir()%"/snap/"%sandboxEnv.appName%"/current";
	}
	else if ((match = flatpakPathRegex.match( absoluteExePath )).hasMatch())
	{
		sandboxEnv.type = SandboxType::Flatpak;
		sandboxEnv.appName = match.captured(1);
		sandboxEnv.homeDir = getMainHomeDir()%"/.var/app/"%sandboxEnv.appName;
	}
	else
	{
		sandboxEnv.type = SandboxType::None;
	}

	return sandboxEnv;
}

static QString getDisplayName( const AppInfo & info )
{
	if constexpr (IS_WINDOWS)
	{
		// On Windows we can use the metadata built into the executable, or the name of its directory.
		if (!info.versionInfo.appName.isEmpty())
			return info.versionInfo.appName;  // exe metadata should be most reliable source
		else
			return fs::getParentDirName( info.exePath );
	}
	else
	{
		// On Linux we have to fallback to the binary name (or use the Flatpak name if there is one).
		if (info.sandboxEnv.type != SandboxType::None)
			return info.sandboxEnv.appName;
		else
			return info.exeBaseName;
	}
}

static QString getNormalizedName( const AppInfo & info )
{
	//if (!info.versionInfo.appName.isEmpty())
	//	return info.versionInfo.appName.toLower();   // "crispy doom" breaks this
	//else
		return info.exeBaseName.toLower();
}

AppInfo getAppInfo( const QString & executablePath )
{
	AppInfo info;

	QString absoluteExePath = fs::getAbsolutePath( executablePath );

	info.exePath = executablePath;
	info.exeBaseName = fs::getFileBasenameFromPath( absoluteExePath );

	info.sandboxEnv = getSandboxEnvInfo( absoluteExePath );

	// Sometimes opening an executable file takes incredibly long (even > 1 second) for unknown reason (antivirus maybe?).
	// So we cache the results here so that at least the subsequent calls are fast.
	if (fs::isValidFile( absoluteExePath ))
		info.versionInfo = g_cachedExeInfo.getFileInfo( absoluteExePath );
	else
		info.versionInfo.status = ReadStatus::CantOpen;

	info.displayName = getDisplayName( info );
	info.normalizedName = getNormalizedName( info );

	return info;
}

// On Unix, to run an executable file inside current working directory, the relative path needs to be prepended by "./"
// On Windows this must be prefixed too! Otherwise Windows will prefer executable in the same directory as DoomRunner
// over executable in the current working directory
// https://superuser.com/questions/897644/how-does-windows-decide-which-executable-to-run/1683394#1683394
inline static QString fixExePath( QString exePath )
{
	if (!exePath.contains("/"))  // the file is in the current working directory
	{
		return "./" + exePath;
	}
	return exePath;
}

ShellCommand getRunCommand(
	const QString & executablePath, const PathRebaser & runnersDirRebaser, bool forceExeName,
	const QStringList & dirsToBeAccessed
){
	QStringList cmdParts, extraPermissions;

	SandboxEnvInfo sandboxEnv = getSandboxEnvInfo( executablePath );
	QDir sandboxAppDir( sandboxEnv.homeDir );

	// different installations require different ways to launch the program executable
 #if IS_FLATPAK_BUILD
	if (fs::getAbsoluteParentDir( executablePath ) == QApplication::applicationDirPath())
	{
		// We are inside a Flatpak package but launching an app inside the same Flatpak package,
		// no special command or permissions needed.
		QString exeFileName = fs::getFileNameFromPath( executablePath );
		return { .executable = exeFileName, .arguments = {}, .extraPermissions = {} };  // this is all we need, skip the rest
	}
	else
	{
		// We are inside a Flatpak package and launching an app outside of this Flatpak package,
		// need to launch it in a special mode granting it special permissions.
		cmdParts << "flatpak-spawn" << "--host";
		// prefix added, continue with the rest
	}
 #endif
	if (sandboxEnv.type == SandboxType::Snap)
	{
		cmdParts << "snap";
		cmdParts << "run";
		// TODO: permissions
		cmdParts << sandboxEnv.appName;
	}
	else if (sandboxEnv.type == SandboxType::Flatpak)
	{
		cmdParts << "flatpak";
		cmdParts << "run";
		for (const QString & dir : dirsToBeAccessed)
		{
			if (!fs::isInsideDir( sandboxAppDir, dir ))
			{
				QString fileSystemPermission = "--filesystem=" + runnersDirRebaser.makeQuotedCmdPath( dir );
				cmdParts << fileSystemPermission;  // add it to the command
				extraPermissions << std::move( fileSystemPermission );  // add it to a list to be shown to the user
			}
		}
		cmdParts << sandboxEnv.appName;
	}
	else if (forceExeName || isInSearchPath( executablePath ))
	{
		// If it's in a search path (C:\Windows\System32, /usr/bin, ...)
		// it should be (and sometimes must be) started directly by using only its name.
		cmdParts << fs::getFileNameFromPath( executablePath );
	}
	else
	{
		QString rebasedExePath = runnersDirRebaser.rebaseAndConvert( executablePath );  // respect configured path style
		cmdParts << runnersDirRebaser.makeCmdPath( fixExePath( rebasedExePath ) );
	}

	ShellCommand cmd;
	cmd.executable = cmdParts.takeFirst();
	cmd.arguments = std::move( cmdParts );
	cmd.extraPermissions = std::move( extraPermissions );
	return cmd;
}


//======================================================================================================================
// graphical environment

const QString & getLinuxDesktopEnv()
{
	static const QString desktopEnv = qEnvironmentVariable("XDG_CURRENT_DESKTOP");  // only need to read this once
	return desktopEnv;
}

QList< MonitorInfo > listMonitors()
{
	QList< MonitorInfo > monitors;

	// in the end this work well for both platforms, just ZDoom indexes the monitors from 1 while GZDoom from 0
	const QList< QScreen * > screens = QGuiApplication::screens();
	monitors.reserve( screens.size() );
	for (qsize_t monitorIdx = 0; monitorIdx < screens.size(); monitorIdx++)
	{
		MonitorInfo myInfo;
		myInfo.name = screens[ monitorIdx ]->name();
		myInfo.width = screens[ monitorIdx ]->size().width();
		myInfo.height = screens[ monitorIdx ]->size().height();
		myInfo.isPrimary = monitorIdx == 0;
		monitors.append( myInfo );
	}

	return monitors;
}


//======================================================================================================================
// miscellaneous

inline constexpr bool OpenTargetDirectory = false;  ///< open directly the selected entry (the entry must be a directory)
inline constexpr bool OpenParentAndSelect = true;   ///< open the parent directory of the entry and highlight the entry

namespace ProcessStatus
{
	inline constexpr int FailedToStart = -2;
	inline constexpr int Crashed = -1;
	inline constexpr int Success = 0;
	// any other value is an exit codes from the executed application
}

static int openEntryInFileBrowser( const QString & entryPath, bool openParentAndSelect )
{
	// based on answers at https://stackoverflow.com/questions/3490336/how-to-reveal-in-finder-or-show-in-explorer-with-qt
	//                 and https://stackoverflow.com/questions/11261516/applescript-open-a-folder-in-finder

	QFileInfo entry( entryPath );

	if constexpr (IS_WINDOWS)
	{
		QString program = "explorer.exe";
		QStringList args;
		if (openParentAndSelect)
			args << "/select,";
		args << fs::toNativePath( entry.canonicalFilePath() );
		return QProcess::startDetached( program, args ) ? ProcessStatus::Success : ProcessStatus::FailedToStart;
	}
	else if constexpr (IS_MACOS)
	{
		QString program = "/usr/bin/osascript";
		QString command = openParentAndSelect ? "select" : "open";
		QStringList args;
		args << "-e" << "tell application \"Finder\"";
		args << "-e" <<     "activate";
		args << "-e" <<     command%" (\""%fs::toNativePath( entry.canonicalFilePath() )%"\" as POSIX file)";
		args << "-e" << "end tell";
		// https://doc.qt.io/qt-6/qprocess.html#execute
		return QProcess::execute( program, args );
	}
	else
	{
		// We cannot select the entry here, because no file browser really supports it.
		QString pathToOpen = openParentAndSelect ? entry.canonicalPath() : entry.canonicalFilePath();
		return QDesktopServices::openUrl( QUrl::fromLocalFile( pathToOpen ) ) ? ProcessStatus::Success : ProcessStatus::FailedToStart;
	}
}

bool openDirectoryWindow( const QString & dirPath )
{
	if (dirPath.isEmpty())
	{
		reportLogicError( nullptr, {}, "Cannot open directory window", "The path is empty." );
		return false;
	}
	else if (!fs::exists( dirPath ))
	{
		reportRuntimeError( nullptr, "Cannot open directory window", "\""%dirPath%"\" does not exist." );
		return false;
	}
	else if (!fs::isDirectory( dirPath ))
	{
		reportRuntimeError( nullptr, "Cannot open directory window", "\""%dirPath%"\" is not a directory." );
		return false;
	}

	int status = openEntryInFileBrowser( dirPath, OpenTargetDirectory );

	if (status != ProcessStatus::Success)
	{
		reportRuntimeError( nullptr, "Cannot open directory window",
			"Opening directory window failed (error code: "%QString::number(status)%")."
		);
		return false;
	}

	return true;
}

bool openFileLocation( const QString & filePath )
{
	if (filePath.isEmpty())
	{
		reportLogicError( nullptr, {}, "Cannot open file location", "The path is empty." );
		return false;
	}
	else if (!fs::exists( filePath ))
	{
		reportRuntimeError( nullptr, "Cannot open file location", "\""%filePath%"\" does not exist." );
		return false;
	}
	/*else if (!fs::isFile( filePath ))
	{
		reportRuntimeError( nullptr, "Cannot open file location", "\""%filePath%"\" is not a file." );
		return false;
	}*/

	int status = openEntryInFileBrowser( filePath, OpenParentAndSelect );

	if (status != ProcessStatus::Success)
	{
		reportRuntimeError( nullptr, "Cannot open file location",
			"Opening file location failed (error code: "%QString::number(status)%")."
		);
		return false;
	}

	return true;
}

bool openFileInDefaultApp( const QString & filePath )
{
	return QDesktopServices::openUrl( QUrl::fromLocalFile( filePath ) );
}

bool openFileInNotepad( const QString & filePath )
{
	QFileInfo fileInfo( filePath );
	QString nativePath = fs::toNativePath( fileInfo.canonicalFilePath() );

	auto startDetachedOrReportError = [ &filePath ]( const QString & program, const QStringList & args )
	{
		bool success = QProcess::startDetached( program, args );
		if (!success)
		{
			QString command = program % ' ' % args.join(' ');
			reportRuntimeError( nullptr, "Cannot open text file",
				"Couldn't open file \""%filePath%"\" in a text editor.\n"
				"Command \""%command%"\" failed."
			);
		}
		return success;
	};

	if constexpr (IS_WINDOWS)
	{
		return startDetachedOrReportError( "notepad", { nativePath } );
	}
	else if constexpr (IS_MACOS)
	{
		return startDetachedOrReportError( "open", { "-t", nativePath } );
	}
	else
	{
		bool success = false;
		QString desktopEnv = getLinuxDesktopEnv();
		if (desktopEnv == "KDE")
			success = QProcess::startDetached( "kate", { nativePath } );
		if (success)
			return true;
		success = QProcess::startDetached( "gnome-text-editor", { nativePath } );
		if (success)
			return true;
		success = QProcess::startDetached( "gedit", { nativePath } );
		if (success)
			return true;
		success = QProcess::startDetached( "sublime-text", { nativePath } );

		if (!success)
		{
			reportRuntimeError( nullptr, "Cannot open text file",
				"Couldn't open file \""%filePath%"\" in a text editor.\n"
				"Neither gnome-text-editor, nor gedit, nor kate, nor sublime-text is installed."
			);
		}

		return success;
	}
}


} // namespace os


//======================================================================================================================
// Windows-specific

#if IS_WINDOWS
namespace win {

bool createShortcut( QString shortcutFile, QString targetFile, QStringList targetArgs, QString workingDir, QString description )
{
	// prepare arguments for WinAPI

	if (!shortcutFile.endsWith(".lnk"))
		shortcutFile.append(".lnk");
	shortcutFile = fs::getAbsolutePath( shortcutFile );
	targetFile = fs::getAbsolutePath( targetFile );
	QString targetArgsStr = targetArgs.join(' ');
	if (workingDir.isEmpty())
		workingDir = fs::getAbsoluteParentDir( targetFile );

	LPCWSTR pszLinkfile = reinterpret_cast< LPCWSTR >( shortcutFile.utf16() );
	LPCWSTR pszTargetfile = reinterpret_cast< LPCWSTR >( targetFile.utf16() );
	LPCWSTR pszTargetargs = reinterpret_cast< LPCWSTR >( targetArgsStr.utf16() );
	LPCWSTR pszCurdir = reinterpret_cast< LPCWSTR >( shortcutFile.utf16() );
	LPCWSTR pszDescription = reinterpret_cast< LPCWSTR >( description.utf16() );

	// https://stackoverflow.com/a/16633100/3575426

	HRESULT       hRes;          /* Returned COM result code */
	IShellLink*   pShellLink;    /* IShellLink object pointer */
	IPersistFile* pPersistFile;  /* IPersistFile object pointer */

	CoInitialize( nullptr );  // initializes the COM library

	hRes = CoCreateInstance(
		CLSID_ShellLink,      /* pre-defined CLSID of the IShellLink object */
		nullptr,              /* pointer to parent interface if part of aggregate */
		CLSCTX_INPROC_SERVER, /* caller and called code are in same	process */
		IID_IShellLink,       /* pre-defined interface of the IShellLink object */
		(LPVOID*)&pShellLink  /* Returns a pointer to the IShellLink object */
	);
	if (!SUCCEEDED( hRes ))
	{
		auto lastError = GetLastError();
		reportRuntimeError( nullptr, "Cannot create shortcut",
			"Cannot create shortcut "%shortcutFile%", CoCreateInstance() failed with error "%QString::number(lastError)
		);
		return false;
	}

	/* Set the fields in the IShellLink object */
	pShellLink->SetPath( pszTargetfile );
	pShellLink->SetArguments( pszTargetargs );
	if (!description.isEmpty())
	{
		pShellLink->SetDescription( pszDescription );
	}
	pShellLink->SetWorkingDirectory( pszCurdir );

	/* Use the IPersistFile object to save the shell link */
	hRes = pShellLink->QueryInterface(
		IID_IPersistFile,       /* pre-defined interface of the IPersistFile object */
		(LPVOID*)&pPersistFile  /* returns a pointer to the IPersistFile object */
	);
	if (!SUCCEEDED( hRes ))
	{
		auto lastError = GetLastError();
		reportRuntimeError( nullptr, "Cannot create shortcut",
			"Cannot create shortcut "%shortcutFile%", IShellLink::QueryInterface() failed with error "%QString::number(lastError)
		);
		return false;
	}

	hRes = pPersistFile->Save( pszLinkfile, TRUE );
	if (!SUCCEEDED( hRes ))
	{
		auto lastError = GetLastError();
		reportRuntimeError( nullptr, "Cannot create shortcut",
			"Cannot create shortcut "%shortcutFile%", IPersistFile::Save() failed with error "%QString::number(lastError)
		);
		return false;
	}

	pPersistFile->Release();
	pShellLink->Release();
	CoUninitialize();

	return true;
}

} // namespace win
#endif // IS_WINDOWS
