//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  14.5.2019
// Description: declaration of shared data structs used accross multiple windows
//======================================================================================================================

#include "SharedData.hpp"

// useful for debug purposes, easier to set breakpoint in than in lambdas
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
QString makeMapPackDispStr( const MapPack & pack )
{
	return pack.name;
}
