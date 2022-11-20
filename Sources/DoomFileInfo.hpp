//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: Doom file type recognition and known WAD detection
//======================================================================================================================

#ifndef DOOM_FILE_INFO_INCLUDED
#define DOOM_FILE_INFO_INCLUDED


#include "Common.hpp"

#include <QVector> // TODO: cleanup
#include <QString>
#include <QStringList>

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
//  known WAD info

QStringList getStandardMapNames( const QString & iwadFileName );

// Some WADs (map packs) don't start at the first map of the list defined by IWADs (MAP01, E1M1, ...).
/// If it's a known WAD and it's known to start from a non-first map, returns that map, otherwise returns empty string.
QString getStartingMap( const QString & wadFileName );


#endif // DOOM_FILE_INFO_INCLUDED
