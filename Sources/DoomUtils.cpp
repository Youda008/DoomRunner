//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  5.4.2020
// Description: doom specific utilities
//======================================================================================================================

#include "DoomUtils.hpp"

#include "LangUtils.hpp"

#include <QVector>
#include <QHash>
#include <QString>
#include <QFileInfo>
#include <QFile>
#include <QDebug>


//======================================================================================================================

static const QVector< QString > iwadSuffixes = {"wad", "iwad", "pk3", "ipk3", "pk7", "ipk7"};
static const QVector< QString > mapSuffixes = {"wad", "deh", "pk3", "pk7", "zip", "7z"};
static const QVector< QString > dukeSuffixes = {"grp", "rff"};

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

// Because IWADs are distinguished from PWADs by reading the file header, we cache the results here,
// so that we don't open and read the files on every update.
// The cache is global for the whole process because why not.
static QHash< QString, WadType > g_cachedWadTypes;

WadType getCachedWadType( const QFileInfo & file )
{
	QString path = file.filePath();
	auto pos = g_cachedWadTypes.find( path );
	if (pos == g_cachedWadTypes.end()) {
		WadType type = recognizeWadTypeByHeader( path );
		if (type == WadType::CANT_READ) {
			qWarning() << "failed to read from " << path;
			return WadType::CANT_READ;
		}
		pos = g_cachedWadTypes.insert( path, type );
	}
	return pos.value();
}

bool isIWAD( const QFileInfo & file )
{
	return (iwadSuffixes.contains( file.suffix().toLower() ))
	     || dukeSuffixes.contains( file.suffix().toLower() );  // i did not want this, but the guy was insisting on it
}

bool isMapPack( const QFileInfo & file )
{
	return (mapSuffixes.contains( file.suffix().toLower() ))
	     || dukeSuffixes.contains( file.suffix().toLower() );  // i did not want this, but the guy was insisting on it
}
