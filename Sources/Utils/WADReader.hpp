//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: WAD file parsing and information extraction
//======================================================================================================================

#ifndef WAD_READER_INCLUDED
#define WAD_READER_INCLUDED


#include "Common.hpp"

#include <QString>
#include <QStringList>


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
	QStringList mapNames;
};

/// Reads required data from a wad file and stores it into a cache.
/** If the file was already read earlier, it returns the cached info. */
const WadInfo & getCachedWadInfo( const QString & filePath );


#endif // WAD_READER_INCLUDED
