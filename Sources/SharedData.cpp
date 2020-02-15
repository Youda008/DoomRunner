//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  14.5.2019
// Description: declaration of shared data structs used accross multiple windows
//======================================================================================================================

#include "SharedData.hpp"

#include "WidgetUtils.hpp"  // updateListFromDir
#include "Utils.hpp"        // PathHelper

#include <QStringBuilder>


//======================================================================================================================

// useful for debug purposes, easier to set breakpoint inside than in lambdas
QString makeEngineDispStrFromName( const Engine & engine )
{
	return engine.name;
}
QString makeEngineDispStrWithPath( const Engine & engine )
{
	return engine.name % "  [" % engine.path % "]";
}
QString makeIwadDispStrFromName( const IWAD & iwad )
{
	return iwad.name;
}
QString makeIwadDispStrWithPath( const IWAD & iwad )
{
	return iwad.name % "  [" % iwad.path % "]";
}

//----------------------------------------------------------------------------------------------------------------------

const QVector<QString> iwadSuffixes = {"wad", "iwad", "pk3", "ipk3", "pk7", "ipk7"};
const QVector<QString> mapSuffixes = {"wad", "pk3", "pk7", "zip", "7z"};
