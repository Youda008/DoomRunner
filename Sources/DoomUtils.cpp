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

bool isDoom1( const QString & iwadName )
{
	return iwadName.compare( "doom.wad", Qt::CaseInsensitive ) == 0
	    || iwadName.startsWith( "doom1" , Qt::CaseInsensitive );
}

WadType recognizeWadTypeByHeader( const QString & filePath )
{
	char wadType [5];

	QFile file( filePath );
	if (!file.open( QIODevice::ReadOnly ))
		return WadType::CANT_READ;

	if (file.read( wadType, 4 ) < 4)
		return WadType::CANT_READ;
	wadType[4] = '\0';

	if (equal( wadType, "IWAD" ))
		return WadType::IWAD;
	else if (equal( wadType, "PWAD" ))
		return WadType::PWAD;
	else
		return WadType::NEITHER;
}
