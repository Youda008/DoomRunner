//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  30.4.2020
// Description: information about application version
//======================================================================================================================

#ifndef VERSION_INCLUDED
#define VERSION_INCLUDED


#include "Common.hpp"

#include <QString>
#include <QtGlobal>


//======================================================================================================================

constexpr const char appVersion [] =
	#include "../version.txt"
;

constexpr const char qtVersion [] = QT_VERSION_STR;

int compareVersions( const QString & ver1, const QString & ver2 );


#endif // VERSION_INCLUDED
