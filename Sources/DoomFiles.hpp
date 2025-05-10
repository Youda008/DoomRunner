//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: Doom file type recognition and known WAD detection
//======================================================================================================================

#ifndef DOOM_FILE_INFO_INCLUDED
#define DOOM_FILE_INFO_INCLUDED


#include "Essential.hpp"

#include <QString>
#include <QStringList>
class QFileInfo;


namespace doom {


//======================================================================================================================
// file type recognition

extern const QString demoFileSuffix;

extern const QStringList iwadSuffixes;
extern const QStringList pwadSuffixes;
extern const QStringList dukeSuffixes;

// convenience wrappers to be used, where otherwise lambda would have to be written
bool canBeIWAD( const QFileInfo & file );
bool canBeMapPack( const QFileInfo & file );

// used to setup file filter in QFileSystemModel
QStringList getModFileSuffixes();


//======================================================================================================================
// known WAD info

struct GameIdentification
{
	const char * name = nullptr;         ///< human-readable name of the game
	const char * gzdoomID = nullptr;     ///< GZDoom-based game ID used as subdirectory for game data
	const char * chocolateID = nullptr;  ///< ChocolateDoom-based game ID used as subdirectory for game data
};
/// Given a list of lump names found in an IWAD, returns what game it probably belongs to.
GameIdentification identifyGame( const QSet< QString > & lumpNames );

namespace game
{
	extern const GameIdentification Doom2;
}

// fallback in case the map names cannot be read from the WAD
QStringList getStandardMapNames( const QString & iwadFileName );

// Some WADs (map packs) don't start at the first map of the list defined by IWADs (MAP01, E1M1, ...).
/// If it's a known WAD and it's known to start from a non-first map, returns that map, otherwise returns empty string.
QString getStartingMap( const QString & wadFileName );


} // namespace doom


//======================================================================================================================


#endif // DOOM_FILE_INFO_INCLUDED
