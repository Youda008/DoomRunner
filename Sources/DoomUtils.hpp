//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  5.4.2020
// Description: doom specific utilities
//======================================================================================================================

#ifndef DOOM_UTILS_INCLUDED
#define DOOM_UTILS_INCLUDED


#include "Common.hpp"

class QString;


//======================================================================================================================

QString getMapNumber( const QString & mapName );

bool isDoom1( const QString & iwadName );


#endif // DOOM_UTILS_INCLUDED
