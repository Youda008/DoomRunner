//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  17.1.2021
// Description: OS-specific utils
//======================================================================================================================

#include "OSUtils.hpp"

#include <QDebug>

#ifdef _WIN32
#include <windows.h>
#else
#include <X11/Xlib.h>
//#include <X11/extensions/Xinerama.h>
#include <X11/extensions/Xrandr.h>
#endif // _WIN32


//======================================================================================================================

std::vector< MonitorInfo > listMonitors()
{
	std::vector< MonitorInfo > monitors;

#ifdef _WIN32

	auto enumMonitorCallback = []( HMONITOR hMonitor, HDC /*hdc*/, LPRECT /*lprcClip*/, LPARAM userData ) -> BOOL
	{
		vector< MonitorInfo > * monitors = (QVector< MonitorInfo > *)userData;

		MONITORINFOEXA theirInfo;
		theirInfo.cbSize = sizeof( theirInfo );
		if (GetMonitorInfoA( hMonitor, &theirInfo ))
		{
			MonitorInfo myInfo;
			myInfo.name = theirInfo.szDevice;
			myInfo.width = theirInfo.rcMonitor.right - theirInfo.rcMonitor.left;
			myInfo.height = theirInfo.rcMonitor.bottom - theirInfo.rcMonitor.top;
			myInfo.isPrimary = theirInfo.dwFlags & MONITORINFOF_PRIMARY;
			monitors->push_back( myInfo );
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
