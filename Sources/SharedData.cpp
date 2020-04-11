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

// Because IWADs are distinguished from PWADs by reading the file header, we cache the results here,
// so that we don't open and read the files on every update.
// The cache is global for the whole process because why not.
static QHash< QString, WadType > g_cachedWadTypes;

static WadType getCachedWadType( const QFileInfo & file )
{
	QString path = file.filePath();
	auto pos = g_cachedWadTypes.find( path );
	if (pos == g_cachedWadTypes.end()) {
		WadType type = recognizeWadTypeByHeader( path );
		if (type == WadType::CANT_READ) {
			qWarning() << "failed to read from " << pos.key();
			return WadType::CANT_READ;
		}
		pos = g_cachedWadTypes.insert( path, type );
	}
	return pos.value();
}

bool isIWAD( const QFileInfo & file )
{
	return iwadSuffixes.contains( file.suffix().toLower() ) && getCachedWadType( file ) == WadType::IWAD;
}

bool isMapPack( const QFileInfo & file )
{
	return mapSuffixes.contains( file.suffix().toLower() ) && getCachedWadType( file ) != WadType::IWAD;
}

IWAD IWADfromFileMaker::operator()( const QFileInfo & file )
{
	return { file.fileName(), pathHelper.convertPath( file.filePath() ) };
}

