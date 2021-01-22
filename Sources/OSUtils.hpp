//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  17.1.2021
// Description: OS-specific utils
//======================================================================================================================

#ifndef OS_UTILS_INCLUDED
#define OS_UTILS_INCLUDED


#include "Common.hpp"

#include <QString>
#include <QVector>


//======================================================================================================================

struct MonitorInfo
{
	QString name;
	int width;
	int height;
	bool isPrimary;
};
QVector< MonitorInfo > listMonitors();

/// returns directory for the application to save its data into
QString getAppDataDir();


#endif // OS_UTILS_INCLUDED
