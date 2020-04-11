//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  14.5.2019
// Description: data structures and functionality that is used accross multiple windows/dialogs
//======================================================================================================================

#include "SharedData.hpp"

#include "FileSystemUtils.hpp"
#include "DoomUtils.hpp"

#include <QHash>
#include <QFileInfo>

#include <QDebug>


//======================================================================================================================

static const QVector< QString > iwadSuffixes = {"wad", "iwad", "pk3", "ipk3", "pk7", "ipk7"};
static const QVector< QString > mapSuffixes = {"wad", "pk3", "pk7", "zip", "7z"};

bool isIWAD( const QFileInfo & file )
{
	return iwadSuffixes.contains( file.suffix().toLower() );
}

bool isMapPack( const QFileInfo & file )
{
	return mapSuffixes.contains( file.suffix().toLower() );
}

IWAD IWADfromFileMaker::operator()( const QFileInfo & file )
{
	return { file.fileName(), pathHelper.convertPath( file.filePath() ) };
}

