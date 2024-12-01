//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: OS-specific utils
//======================================================================================================================

#include "OSUtils.hpp"

#include "FileSystemUtils.hpp"
#include "ErrorHandling.hpp"

#include <QStandardPaths>
#include <QApplication>
#include <QGuiApplication>
#include <QDesktopServices>  // fallback for openFileLocation
#include <QUrl>
#include <QRegularExpression>
#include <QScreen>
#include <QProcess>

#if IS_WINDOWS
	#include <windows.h>
	#include <shlobj.h>
#endif


namespace os {


//======================================================================================================================
//  standard directories

QString getHomeDir()
{
	return QStandardPaths::writableLocation( QStandardPaths::HomeLocation );
}

QString getDocumentsDir()
{
	return QStandardPaths::writableLocation( QStandardPaths::DocumentsLocation );
}

#if IS_WINDOWS
QString getSavedGamesDir()
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
	dir.replace('\\', '/');
	return dir;
}
#endif

QString getAppConfigDir()
{
 #if !IS_WINDOWS && defined(FLATPAK_BUILD)  // the launcher is a Flatpak installation on Linux
	// Inside Flatpak environment the GenericConfigLocation points into the Flatpak sandbox of this application.
	// But we need the system-wide config dir, and that's not available via Qt, so we must do this guessing hack.
	return getHomeDir()%"/.config";
 #else
	return QStandardPaths::writableLocation( QStandardPaths::GenericConfigLocation );
 #endif
}

QString getAppDataDir()
{
 #if !IS_WINDOWS && defined(FLATPAK_BUILD)  // the launcher is a Flatpak installation on Linux
	// Inside Flatpak environment the GenericDataLocation points into the Flatpak sandbox of this application.
	// But we need the system-wide data dir, and that's not available via Qt, so we must do this guessing hack.
	return getHomeDir()%"/.local/share";
 #else
	return QStandardPaths::writableLocation( QStandardPaths::GenericDataLocation );
 #endif
}

QString getConfigDirForApp( const QString & executablePath )
{
	QString genericConfigDir = getAppConfigDir();
	QString appName = fs::getFileBasenameFromPath( executablePath );
	return fs::getPathFromFileName( genericConfigDir, appName );  // -> /home/youda/.config/zdoom
}

QString getDataDirForApp( const QString & executablePath )
{
	QString genericDataDir = getAppDataDir();
	QString appName = fs::getFileBasenameFromPath( executablePath );
	return fs::getPathFromFileName( genericDataDir, appName );  // -> /home/youda/.local/share/zdoom
}

QString getThisAppConfigDir()
{
	// mimic ZDoom behaviour - save to application's binary dir on Windows, but to /home/user/.config/DoomRunner on Linux
 #if IS_WINDOWS
	QString thisExeDir = QApplication::applicationDirPath();
	if (fs::isDirectoryWritable( thisExeDir ))
		return thisExeDir;
	else  // if we cannot write to the directory where the exe is extracted (e.g. Program Files), fallback to %AppData%/Local
		return QStandardPaths::writableLocation( QStandardPaths::AppConfigLocation );
 #else
	return QStandardPaths::writableLocation( QStandardPaths::AppConfigLocation );
 #endif
}

QString getThisAppDataDir()
{
	// mimic ZDoom behaviour - save to application's binary dir on Windows, but to /home/user/.local/share/DoomRunner on Linux
 #if IS_WINDOWS
	QString thisExeDir = QApplication::applicationDirPath();
	if (fs::isDirectoryWritable( thisExeDir ))
		return thisExeDir;
	else  // if we cannot write to the directory where the exe is extracted (e.g. Program Files), fallback to %AppData%/Roaming
		return QStandardPaths::writableLocation( QStandardPaths::AppDataLocation );
 #else
	return QStandardPaths::writableLocation( QStandardPaths::AppDataLocation );
 #endif
}


//-- cached variants -------------------------------------------------------------------------------
// We don't use local static variables, because those use a mutex to prevent initialization by multiple threads.
// These functions will however always be used from the main thread only, so mutex is not needed.

static std::optional< QString > g_homeDir;
const QString & getCachedHomeDir()
{
	if (!g_homeDir)
		g_homeDir = getHomeDir();
	return *g_homeDir;
}

static std::optional< QString > g_documentsDir;
const QString & getCachedDocumentsDir()
{
	if (!g_documentsDir)
		g_documentsDir = getDocumentsDir();
	return *g_documentsDir;
}

#if IS_WINDOWS
static std::optional< QString > g_savedGamesDir;
const QString & getCachedSavedGamesDir()
{
	if (!g_savedGamesDir)
		g_savedGamesDir = getSavedGamesDir();
	return *g_savedGamesDir;
}
#endif

static std::optional< QString > g_appConfigDir;
const QString & getCachedAppConfigDir()
{
	if (!g_appConfigDir)
		g_appConfigDir = getAppConfigDir();
	return *g_appConfigDir;
}

static std::optional< QString > g_appDataDir;
const QString & getCachedAppDataDir()
{
	if (!g_appDataDir)
		g_appDataDir = getAppDataDir();
	return *g_appDataDir;
}

QString getCachedConfigDirForApp( const QString & executablePath )
{
	const QString & genericConfigDir = getCachedAppConfigDir();
	QString appName = fs::getFileBasenameFromPath( executablePath );
	return fs::getPathFromFileName( genericConfigDir, appName );  // -> /home/youda/.config/zdoom
}

QString getCachedDataDirForApp( const QString & executablePath )
{
	const QString & genericDataDir = getCachedAppDataDir();
	QString appName = fs::getFileBasenameFromPath( executablePath );
	return fs::getPathFromFileName( genericDataDir, appName );  // -> /home/youda/.local/share/zdoom
}

static std::optional< QString > g_thisAppConfigDir;
const QString & getCachedThisAppConfigDir()
{
	if (!g_thisAppConfigDir)
		g_thisAppConfigDir = getThisAppConfigDir();
	return *g_thisAppConfigDir;
}

static std::optional< QString > g_thisAppDataDir;
const QString & getCachedThisAppDataDir()
{
	if (!g_thisAppDataDir)
		g_thisAppDataDir = getThisAppDataDir();
	return *g_thisAppDataDir;
}


//-- misc ------------------------------------------------------------------------------------------

bool isInSearchPath( const QString & filePath )
{
	return QStandardPaths::findExecutable( fs::getFileNameFromPath( filePath ) ) == filePath;
}


//-- installation properties -----------------------------------------------------------------------

QString getSandboxName( SandboxEnv sandbox )
{
	switch (sandbox)
	{
		case SandboxEnv::Snap:    return "Snap";
		case SandboxEnv::Flatpak: return "Flatpak";
		default:               return "<invalid>";
	}
}

static const QRegularExpression snapPathRegex("^/snap/");
static const QRegularExpression flatpakPathRegex("^/var/lib/flatpak/app/([^/]+)/");

SandboxEnvInfo getSandboxEnvInfo( const QString & executablePath )
{
	SandboxEnvInfo sandboxEnv;

	QRegularExpressionMatch match;
	if ((match = snapPathRegex.match( executablePath )).hasMatch())
	{
		sandboxEnv.type = SandboxEnv::Snap;
		sandboxEnv.appName = fs::getFileBasenameFromPath( executablePath );
	}
	else if ((match = flatpakPathRegex.match( executablePath )).hasMatch())
	{
		sandboxEnv.type = SandboxEnv::Flatpak;
		sandboxEnv.appName = match.captured(1);
	}
	else
	{
		sandboxEnv.type = SandboxEnv::None;
		sandboxEnv.appName = "";  // make QString::isNull() false to indicate the sandbox info has been initialized
	}

	return sandboxEnv;
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
	const QString & executablePath, const PathRebaser & currentDirToNewWorkingDir, const QStringVec & dirsToBeAccessed
){
	ShellCommand cmd;
	QStringVec cmdParts;

	SandboxEnvInfo sandboxEnv = getSandboxEnvInfo( executablePath );

	// different installations require different ways to launch the program executable
 #ifdef FLATPAK_BUILD
	if (fs::getAbsoluteDirOfFile( executablePath ) == QApplication::applicationDirPath())
	{
		// We are inside a Flatpak package but launching an app inside the same Flatpak package,
		// no special command or permissions needed.
		cmd.executable = fs::getFileNameFromPath( executablePath );
		return cmd;  // this is all we need, skip the rest
	}
	else
	{
		// We are inside a Flatpak package and launching an app outside of this Flatpak package,
		// need to launch it in a special mode granting it special permissions.
		cmdParts << "flatpak-spawn" << "--host";
		// prefix added, continue with the rest
	}
 #endif
	if (sandboxEnv.type == SandboxEnv::Snap)
	{
		cmdParts << "snap";
		cmdParts << "run";
		// TODO: permissions
		cmdParts << sandboxEnv.appName;
	}
	else if (sandboxEnv.type == SandboxEnv::Flatpak)
	{
		cmdParts << "flatpak";
		cmdParts << "run";
		for (const QString & dir : dirsToBeAccessed)
		{
			QString fileSystemPermission = "--filesystem=" + fs::getAbsolutePath( dir );
			cmdParts << currentDirToNewWorkingDir.maybeQuoted( fileSystemPermission );
			cmd.extraPermissions << std::move(fileSystemPermission);
		}
		cmdParts << sandboxEnv.appName;
	}
	else if (isInSearchPath( executablePath ))
	{
		// If it's in a search path (C:\Windows\System32, /usr/bin, ...)
		// it should be (and sometimes must be) started directly by using only its name.
		cmdParts << fs::getFileNameFromPath( executablePath );
	}
	else
	{
		QString rebasedExePath = fixExePath( currentDirToNewWorkingDir.rebasePath( executablePath ) );
		cmdParts << currentDirToNewWorkingDir.maybeQuoted( rebasedExePath );
	}

	cmd.executable = cmdParts.takeFirst();
	cmd.arguments = std::move( cmdParts );
	return cmd;
}


//======================================================================================================================
//  graphical environment

const QString & getLinuxDesktopEnv()
{
	static const QString desktopEnv = qEnvironmentVariable("XDG_CURRENT_DESKTOP");  // only need to read this once
	return desktopEnv;
}

QVector< MonitorInfo > listMonitors()
{
	QVector< MonitorInfo > monitors;

	// in the end this work well for both platforms, just ZDoom indexes the monitors from 1 while GZDoom from 0
	QList< QScreen * > screens = QGuiApplication::screens();
	for (int monitorIdx = 0; monitorIdx < screens.count(); monitorIdx++)
	{
		MonitorInfo myInfo;
		myInfo.name = screens[ monitorIdx ]->name();
		myInfo.width = screens[ monitorIdx ]->size().width();
		myInfo.height = screens[ monitorIdx ]->size().height();
		myInfo.isPrimary = monitorIdx == 0;
		monitors.push_back( myInfo );
	}

	return monitors;
}


//======================================================================================================================
//  miscellaneous

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

 #if defined(Q_OS_WIN)

	QStringList args;
	if (openParentAndSelect)
		args << "/select,";
	args << QDir::toNativeSeparators( entry.canonicalFilePath() );
	return QProcess::startDetached( "explorer.exe", args ) ? ProcessStatus::Success : ProcessStatus::FailedToStart;

 #elif defined(Q_OS_MAC)

	QString command = openParentAndSelect ? "select" : "open";
	QStringList args;
	args << "-e" << "tell application \"Finder\"";
	args << "-e" <<     "activate";
	args << "-e" <<     command%" (\""%entry.canonicalFilePath()%"\" as POSIX file)";
	args << "-e" << "end tell";
	// https://doc.qt.io/qt-6/qprocess.html#execute
	return QProcess::execute( "/usr/bin/osascript", args );

 #else

	// We cannot select the entry here, because no file browser really supports it.
	QString pathToOpen = openParentAndSelect ? entry.canonicalPath() : entry.canonicalFilePath();
	return QDesktopServices::openUrl( QUrl::fromLocalFile( pathToOpen ) ) ? ProcessStatus::Success : ProcessStatus::FailedToStart;

 #endif
}

bool openDirectoryWindow( const QString & dirPath )
{
	if (dirPath.isEmpty())
	{
		reportLogicError( nullptr, "Cannot open directory window", "The path is empty." );
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
		reportLogicError( nullptr, "Cannot open file location", "The path is empty." );
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


} // namespace os


//======================================================================================================================
//  Windows-specific

#if IS_WINDOWS
namespace win {

bool createShortcut( QString shortcutFile, QString targetFile, QStringVec targetArgs, QString workingDir, QString description )
{
	// prepare arguments for WinAPI

	if (!shortcutFile.endsWith(".lnk"))
		shortcutFile.append(".lnk");
	shortcutFile = fs::getAbsolutePath( shortcutFile );
	targetFile = fs::getAbsolutePath( targetFile );
	QString targetArgsStr = targetArgs.join(' ');
	if (workingDir.isEmpty())
		workingDir = fs::getAbsoluteDirOfFile( targetFile );

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
		logRuntimeError() << "Cannot create shortcut "<<shortcutFile<<", CoCreateInstance() failed with error "<<lastError;
		return false;
	}

	/* Set the fields in the IShellLink object */
	pShellLink->SetPath( pszTargetfile );
	pShellLink->SetArguments( pszTargetargs );
	if (!description.isEmpty())
	{
		hRes = pShellLink->SetDescription( pszDescription );
	}
	hRes = pShellLink->SetWorkingDirectory( pszCurdir );

	/* Use the IPersistFile object to save the shell link */
	hRes = pShellLink->QueryInterface(
		IID_IPersistFile,       /* pre-defined interface of the IPersistFile object */
		(LPVOID*)&pPersistFile  /* returns a pointer to the IPersistFile object */
	);
	if (!SUCCEEDED( hRes ))
	{
		auto lastError = GetLastError();
		logRuntimeError() << "Cannot create shortcut "<<shortcutFile<<", IShellLink::QueryInterface() failed with error "<<lastError;
		return false;
	}

	hRes = pPersistFile->Save( pszLinkfile, TRUE );
	if (!SUCCEEDED( hRes ))
	{
		auto lastError = GetLastError();
		logRuntimeError() << "Cannot create shortcut "<<shortcutFile<<", IPersistFile::Save() failed with error "<<lastError;
		return false;
	}

	pPersistFile->Release();
	pShellLink->Release();
	CoUninitialize();

	return true;
}

} // namespace win
#endif // IS_WINDOWS
