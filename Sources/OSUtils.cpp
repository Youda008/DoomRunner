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
#include <QDebug>

#ifdef _WIN32
#include <windows.h>
#else
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#endif // _WIN32


//======================================================================================================================

QVector< MonitorInfo > listMonitors()
{
	QVector< MonitorInfo > monitors;

	// We can't use Qt for that because Qt reorders the monitors so that the primary one is always first,
	// but we need them in the original order given by system.

#ifdef _WIN32

	auto enumMonitorCallback = []( HMONITOR hMonitor, HDC /*hdc*/, LPRECT /*lprcClip*/, LPARAM userData ) -> BOOL
	{
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
	};

	EnumDisplayMonitors( nullptr, nullptr, (MONITORENUMPROC)enumMonitorCallback, (LONG_PTR)&monitors );

#else

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
