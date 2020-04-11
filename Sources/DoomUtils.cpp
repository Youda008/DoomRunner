//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  5.4.2020
// Description: doom specific utilities
//======================================================================================================================

#include "DoomUtils.hpp"

#include "LangUtils.hpp"

#include <QString>
#include <QFile>


//======================================================================================================================

QString getMapNumber( const QString & mapName )
{
	if (mapName.startsWith('E')) {  // E2M7
		return mapName[1]+QString(' ')+mapName[3];
	} else {  // MAP21
		return mapName.mid(3,2);
	}
}

bool isDoom1( const QString & iwadName )
{
	return iwadName.compare( "doom.wad", Qt::CaseInsensitive ) == 0
	    || iwadName.startsWith( "doom1" , Qt::CaseInsensitive );
}
