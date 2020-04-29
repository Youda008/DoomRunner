//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  5.4.2020
// Description: doom specific utilities
//======================================================================================================================

#ifndef DOOM_UTILS_INCLUDED
#define DOOM_UTILS_INCLUDED


#include "Common.hpp"

class QString;
class QFileInfo;


//======================================================================================================================

inline constexpr const char configFileExt [] = "ini";
inline constexpr const char saveFileExt [] = "zds";

bool isDoom1( const QString & iwadName );

enum class WadType {
	CANT_READ,
	IWAD,
	PWAD,
	NEITHER
};

/** this actually opens and reads the file, so don't call it very often, instead used the cached results below */
WadType recognizeWadTypeByHeader( const QString & filePath );

/** returns last known WAD type for this file path */
WadType getCachedWadType( const QFileInfo & file );

// convenience wrappers to be used, where otherwise lambda would have to be written
bool isIWAD( const QFileInfo & file );
bool isMapPack( const QFileInfo & file );


#endif // DOOM_UTILS_INCLUDED
