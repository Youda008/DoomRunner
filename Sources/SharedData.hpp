//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  14.5.2019
// Description: declaration of shared data structs used accross multiple windows
//======================================================================================================================

#ifndef SHARED_DATA_INCLUDED
#define SHARED_DATA_INCLUDED


#include "Common.hpp"

#include <QString>
#include <QStringBuilder>


//======================================================================================================================
//  this needs to be in a separate header, so it can be included from multiple windows

struct Engine {
	QString name;
	QString path;
};

struct IWAD {
	QString name;
	QString path;
};

struct MapPack {
	QString name;
};

// useful for debugging purposes, easier to set breakpoint in than in lambdas
QString makeEngineDispStrFromName( const Engine & engine );
QString makeEngineDispStrWithPath( const Engine & engine );
QString makeIwadDispStrFromName( const IWAD & iwad );
QString makeIwadDispStrWithPath( const IWAD & iwad );
QString makeMapPackDispStr( const MapPack & pack );


struct GameplayOptions {
	int32_t flags1 = 0;
	int32_t flags2 = 0;
};

struct CompatibilityOptions {
	int32_t flags1 = 0;
	int32_t flags2 = 0;
};


#endif // SHARED_DATA_INCLUDED
