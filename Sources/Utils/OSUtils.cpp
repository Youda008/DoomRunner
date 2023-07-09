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
#include <QScreen>
#include <QProcess>

#if IS_WINDOWS
	#include <windows.h>
	#include <shlobj.h>
	//#include <winnls.h>
	//#include <shobjidl.h>
	//#include <objbase.h>
	//#include <objidl.h>
	//#include <shlguid.h>
#endif // IS_WINDOWS


//======================================================================================================================

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

QString getThisAppDataDir()
{
	// mimic ZDoom behaviour - save to application's binary dir in Windows, but to /home/user/.config/DoomRunner in Linux
	if (isWindows())
	{
		QString appExeDir = QApplication::applicationDirPath();
		if (isDirectoryWritable( appExeDir ))
			return appExeDir;
		else  // if we cannot write to the directory where the exe is extracted (e.g. Program Files), fallback to %AppData%
			return QStandardPaths::writableLocation( QStandardPaths::AppConfigLocation );
	}
	else
	{
		return QStandardPaths::writableLocation( QStandardPaths::AppConfigLocation );
	}
}

QString getAppDataDir( const QString & executablePath )
{
	// In Windows engines store their config in the directory of its binaries,
	// but in Linux it stores them in standard user's app config dir (usually something like /home/user/.config/)
	if (isWindows())
	{
		return getDirOfFile( executablePath );
	}
	else
	{
		QDir standardConfigDir( QStandardPaths::writableLocation( QStandardPaths::GenericConfigLocation ) );
		QString appName = getFileBasenameFromPath( executablePath );
		return standardConfigDir.filePath( appName );  // -> /home/user/.config/zdoom
	}
}

bool isInSearchPath( const QString & filePath )
{
	// this should also handle the snap installations, since directory of snap executables is inside PATH
	return QStandardPaths::findExecutable( getFileNameFromPath( filePath ) ) == filePath;
}

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
bool createWindowsShortcut( QString shortcutFile, QString targetFile, QStringList targetArgs, QString workingDir, QString description )
{
	// prepare arguments for WinAPI

	if (!shortcutFile.endsWith(".lnk"))
		shortcutFile.append(".lnk");
	shortcutFile = getAbsolutePath( shortcutFile );
	targetFile = getAbsolutePath( targetFile );
	QString targetArgsStr = targetArgs.join(' ');
	if (workingDir.isEmpty())
		workingDir = getAbsoluteDirOfFile( targetFile );

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
		return false;
	}

	hRes = pPersistFile->Save( pszLinkfile, TRUE );
	if (!SUCCEEDED( hRes ))
	{
		return false;
	}

	pPersistFile->Release();
	pShellLink->Release();
	CoUninitialize();

	return true;
}
#endif // IS_WINDOWS
