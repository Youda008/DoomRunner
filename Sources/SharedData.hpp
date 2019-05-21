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
// in main window we want to display the engines as names only and in setup dialog with paths
inline QString makeEngineUITextFromName( const Engine & engine )
{
	return engine.name;
}
inline QString makeEngineUITextWithPath( const Engine & engine )
{
	return engine.name % "  [" % engine.path % "]";
}

struct IWAD {
	QString name;
	QString path;
};
// in main window we want to display the iwads as names only and in setup dialog with paths
inline QString makeIwadUITextFromName( const IWAD & iwad )
{
	return iwad.name;
}
inline QString makeIwadUITextWithPath( const IWAD & iwad )
{
	return iwad.name % "  [" % iwad.path % "]";
}


#endif // SHARED_DATA_INCLUDED
