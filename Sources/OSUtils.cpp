//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  17.1.2021
// Description: OS-specific utils
//======================================================================================================================

#include "OSUtils.hpp"

#include <QStandardPaths>
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>

#include <QDebug>

#ifdef _WIN32
//#include <windows.h>
#else
//#include <X11/Xlib.h>
//#include <X11/extensions/Xrandr.h>
//#include <SDL2/SDL.h>
//#include <SDL2/SDL_vulkan.h>
#endif // _WIN32


//======================================================================================================================

#ifdef _WIN32
//static BOOL CALLBACK enumMonitorCallback( HMONITOR hMonitor, HDC /*hdc*/, LPRECT /*lprcClip*/, LPARAM userData )
/*{
	QVector< MonitorInfo > * monitors = (QVector< MonitorInfo > *)userData;

	MONITORINFOEXA theirInfo;
	theirInfo.cbSize = sizeof( theirInfo );
	if (GetMonitorInfoA( hMonitor, &theirInfo ))
	{
		MonitorInfo myInfo;
		myInfo.name = theirInfo.szDevice;
		myInfo.width = theirInfo.rcMonitor.right - theirInfo.rcMonitor.left;
		myInfo.height = theirInfo.rcMonitor.bottom - theirInfo.rcMonitor.top;
		myInfo.isPrimary = theirInfo.dwFlags & MONITORINFOF_PRIMARY;
		monitors->append( myInfo );
	}

	return TRUE;
};*/
#endif // _WIN32

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

#ifdef _WIN32

/* WinAPI
	EnumDisplayMonitors( nullptr, nullptr, (MONITORENUMPROC)enumMonitorCallback, (LONG_PTR)&monitors );
*/

#else

/* libXrandr
	Display * display = XOpenDisplay( nullptr );
	if (!display)
	{
		qWarning() << "cannot open X Display";
		return monitors;
	}

	int dummy1, dummy2;
	int major, minor;
	if (!XRRQueryExtension( display, &dummy1, &dummy2 )
	 || !XRRQueryVersion( display, &major, &minor ))
	{
		qWarning() << "failed to retrieve XRandR version";
		XCloseDisplay( display );
		return monitors;
	}

	if (major <= 1 && minor < 5)
	{
		qWarning() << "RandR version at least 1.5 is required, available is only " << major << '.' << minor;
		XCloseDisplay( display );
		return monitors;
	}

	Window rootWindow = RootWindow( display, DefaultScreen( display ) );

	Bool onlyActive = false;
	int monitorCnt;
	XRRMonitorInfo * xMonitors = XRRGetMonitors( display, rootWindow, onlyActive, &monitorCnt );
	if (!xMonitors)
	{
		qDebug() << "failed to retrieve monitor info";
		XCloseDisplay( display );
		return monitors;
	}

	for (int monitorIdx = 0; monitorIdx < monitorCnt; monitorIdx++)
	{
		MonitorInfo myInfo;
		myInfo.name = XGetAtomName( display, xMonitors[ monitorIdx ].name );
		myInfo.width = xMonitors[ monitorIdx ].width;
		myInfo.height = xMonitors[ monitorIdx ].height;
		myInfo.isPrimary = xMonitors[ monitorIdx ].primary != 0;
		monitors.push_back( myInfo );
	}

	XFree( xMonitors );
	XCloseDisplay( display );
*/

/* SDL2
	SDL_Init( SDL_INIT_VIDEO );

	int monitorCnt = SDL_GetNumVideoDisplays();
	for (int monitorIdx = 0; monitorIdx < monitorCnt; monitorIdx++)
	{
		MonitorInfo myInfo;
		SDL_DisplayMode mode;
		SDL_GetCurrentDisplayMode( monitorIdx, &mode );
		myInfo.name = SDL_GetDisplayName( monitorIdx );
		myInfo.width = mode.w;
		myInfo.height = mode.h;
		myInfo.isPrimary = monitorIdx == 0;
		monitors.push_back( myInfo );
	}

	SDL_Quit();
*/

#endif // _WIN32

	return monitors;
}

QString getAppDataDir()
{
	// mimic ZDoom behaviour - save to application's binary dir in Windows, but to /home/user/.config/DoomRunner in Linux
#ifdef _WIN32
	return QApplication::applicationDirPath();
#else
	return QStandardPaths::writableLocation( QStandardPaths::AppConfigLocation );
#endif
}
