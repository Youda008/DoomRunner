//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  13.5.2019
// Description: general utilities
//======================================================================================================================

#include "Utils.hpp"


//======================================================================================================================

QString getMapNumber( QString mapName )
{
	if (mapName.startsWith('E')) {  // E2M7
		return mapName[1]+QString(' ')+mapName[3];
	} else {  // MAP21
		return mapName.mid(3,2);
	}
}

bool isDoom1( QString iwadName )
{
	return iwadName.compare( "doom.wad", Qt::CaseInsensitive ) == 0
	    || iwadName.startsWith( "doom1" , Qt::CaseInsensitive );
}
