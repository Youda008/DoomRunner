//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: doom-specific utilities
//======================================================================================================================

#ifndef DOOM_UTILS_INCLUDED
#define DOOM_UTILS_INCLUDED


#include "Common.hpp"

#include <QVector>
#include <QStringList>

class QString;
class QFileInfo;


//======================================================================================================================
//  file type recognition

extern const QVector< QString > configFileSuffixes;
extern const QString saveFileSuffix;
extern const QString demoFileSuffix;

extern const QVector< QString > iwadSuffixes;
extern const QVector< QString > pwadSuffixes;
extern const QVector< QString > dukeSuffixes;

// convenience wrappers to be used, where otherwise lambda would have to be written
bool isIWAD( const QFileInfo & file );
bool isMapPack( const QFileInfo & file );

// used to setup file filter in QFileSystemModel
QStringList getModFileSuffixes();


//======================================================================================================================
//  properties and capabilities of different engines

/// Properties of a different engine types such as GZDoom, Zandronum, PrBoom, ...
struct EngineProperties
{
	const char * saveDirParam;
	int firstMonitorIndex;  // some engines index monitors from 1 and others from 0
};

/// Returns properties of an engine based on its executable name, or default properties if it's not recognized.
const EngineProperties & getEngineProperties( const QString & enginePath );


//======================================================================================================================
//  WAD info loading

enum class WadType
{
	IWAD,
	PWAD,
	Neither
};

struct WadInfo
{
	bool successfullyRead;
	WadType type;
	QVector< QString > mapNames;
};

/// Reads required data from a wad file and stores it into a cache.
/** If the file was already read earlier, it returns the cached info. */
const WadInfo & getCachedWadInfo( const QString & filePath );


#endif // DOOM_UTILS_INCLUDED
