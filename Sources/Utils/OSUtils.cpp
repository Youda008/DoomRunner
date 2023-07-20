//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: OS-specific utils
//======================================================================================================================

#include "OSUtils.hpp"

#include "FileSystemUtils.hpp"

#include <QStandardPaths>
#include <QApplication>
#include <QGuiApplication>
#include <QDesktopServices>  // fallback for openFileLocation
#include <QUrl>
#include <QRegularExpression>
#include <QScreen>
#include <QProcess>

#include <QDebug>

#if IS_WINDOWS
	#include <windows.h>
	#include <shlobj.h>
#endif


//======================================================================================================================

namespace os {


//----------------------------------------------------------------------------------------------------------------------
//  standard directories and installation properties

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
		qDebug().nospace() << "Cannot get Saved Games location, SHGetKnownFolderPath() failed with error "<<lastError;
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

bool isInSearchPath( const QString & filePath )
{
	return QStandardPaths::findExecutable( fs::getFileNameFromPath( filePath ) ) == filePath;
}

QString getSandboxName( Sandbox sandbox )
{
	switch (sandbox)
	{
		case Sandbox::Snap:    return "Snap";
		case Sandbox::Flatpak: return "Flatpak";
		default:               return "<invalid>";
	}
}

static const QRegularExpression snapPathRegex("^/snap/");
static const QRegularExpression flatpakPathRegex("^/var/lib/flatpak/app/([^/]+)/");

SandboxInfo getSandboxInfo( const QString & executablePath )
{
	SandboxInfo sandbox;

	QRegularExpressionMatch match;
	if ((match = snapPathRegex.match( executablePath )).hasMatch())
	{
		sandbox.type = Sandbox::Snap;
		sandbox.appName = fs::getFileBasenameFromPath( executablePath );
	}
	else if ((match = flatpakPathRegex.match( executablePath )).hasMatch())
	{
		sandbox.type = Sandbox::Flatpak;
		sandbox.appName = match.captured(1);
	}
	else
	{
		sandbox.type = Sandbox::None;
	}

	return sandbox;
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
	const QString & executablePath, const PathRebaser & currentDirToNewBaseDir, const QStringVec & dirsToBeAccessed
){
	ShellCommand cmd;
	QStringVec cmdParts;

	SandboxInfo traits = getSandboxInfo( executablePath );

	// different installations require different ways to launch the engine executable
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
	if (traits.type == Sandbox::Snap)
	{
		cmdParts << "snap";
		cmdParts << "run";
		// TODO: permissions
		cmdParts << traits.appName;
	}
	else if (traits.type == Sandbox::Flatpak)
	{
		cmdParts << "flatpak";
		cmdParts << "run";
		for (const QString & dir : dirsToBeAccessed)
		{
			QString fileSystemPermission = "--filesystem=" + fs::getAbsolutePath( dir );
			cmdParts << currentDirToNewBaseDir.maybeQuoted( fileSystemPermission );
			cmd.extraPermissions << std::move(fileSystemPermission);
		}
		cmdParts << traits.appName;
	}
	else if (isInSearchPath( executablePath ))
	{
		// If it's in a search path (C:\Windows\System32, /usr/bin, ...)
		// it should be (and sometimes must be) started directly by using only its name.
		cmdParts << fs::getFileNameFromPath( executablePath );
	}
	else
	{
		cmdParts << currentDirToNewBaseDir.maybeQuoted( fixExePath( currentDirToNewBaseDir.rebasePath( executablePath ) ) );
	}

	cmd.executable = cmdParts.takeFirst();
	cmd.arguments = std::move( cmdParts );
	return cmd;
}


//----------------------------------------------------------------------------------------------------------------------
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


//----------------------------------------------------------------------------------------------------------------------
//  miscellaneous

bool openFileLocation( const QString & filePath )
{
	// based on answers at https://stackoverflow.com/questions/3490336/how-to-reveal-in-finder-or-show-in-explorer-with-qt

	const QFileInfo fileInfo( filePath );

 #if defined(Q_OS_WIN)

	QStringList args;
	if (!fileInfo.isDir())
		args += "/select,";
	args += QDir::toNativeSeparators( fileInfo.canonicalFilePath() );
	return QProcess::startDetached( "explorer.exe", args );

 #elif defined(Q_OS_MAC)

	QStringList args;
	args << "-e";
	args << "tell application \"Finder\"";
	args << "-e";
	args << "activate";
	args << "-e";
	args << "select POSIX file \"" + fileInfo.canonicalFilePath() + "\"";
	args << "-e";
	args << "end tell";
	args << "-e";
	args << "return";
	return QProcess::execute( "/usr/bin/osascript", args );

 #else

	// We cannot select a file here, because no file browser really supports it.
	return QDesktopServices::openUrl( QUrl::fromLocalFile( fileInfo.isDir()? fileInfo.filePath() : fileInfo.path() ) );

 #endif
}

#if IS_WINDOWS
bool createWindowsShortcut( QString shortcutFile, QString targetFile, QStringVec targetArgs, QString workingDir, QString description )
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
		qDebug().nospace() << "Cannot create shortcut "<<shortcutFile<<", CoCreateInstance() failed with error "<<lastError;
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
		qDebug().nospace() << "Cannot create shortcut "<<shortcutFile<<", IShellLink::QueryInterface() failed with error "<<lastError;
		return false;
	}

	hRes = pPersistFile->Save( pszLinkfile, TRUE );
	if (!SUCCEEDED( hRes ))
	{
		auto lastError = GetLastError();
		qDebug().nospace() << "Cannot create shortcut "<<shortcutFile<<", IPersistFile::Save() failed with error "<<lastError;
		return false;
	}

	pPersistFile->Release();
	pShellLink->Release();
	CoUninitialize();

	return true;
}
#endif // IS_WINDOWS


} // namespace os
