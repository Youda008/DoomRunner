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
/// this actually opens and reads the file, so don't call it very often, instead cache the results
WadType recognizeWadTypeByHeader( const QString & filePath );




#endif // DOOM_UTILS_INCLUDED
