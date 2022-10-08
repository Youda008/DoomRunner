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
#include <QScreen>

#ifdef _WIN32
	#include <windows.h>
	#include <shlobj.h>
	//#include <winnls.h>
	//#include <shobjidl.h>
	//#include <objbase.h>
	//#include <objidl.h>
	//#include <shlguid.h>
#endif // _WIN32


//======================================================================================================================

QVector< MonitorInfo > listMonitors()
{
	QVector< MonitorInfo > monitors;

	// in the end this work well for both platforms, just ZDoom indexes the monitors from 1 while GZDoom from 0
	QList<QScreen *> screens = QGuiApplication::screens();
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

QString getAppDataDir()
{
	// mimic ZDoom behaviour - save to application's binary dir in Windows, but to /home/user/.config/DoomRunner in Linux
 #ifdef _WIN32
	// TODO: check if writable
	return QApplication::applicationDirPath();
 #else
	return QStandardPaths::writableLocation( QStandardPaths::AppConfigLocation );
 #endif
}

bool isInSearchPath( const QString & filePath )
{
	// this should also handle the snap installations, since directory of snap executables is inside PATH
	return !QStandardPaths::findExecutable( getFileNameFromPath( filePath ) ).isEmpty();
}

#ifdef _WIN32
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
#endif // _WIN32
