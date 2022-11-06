//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: implementation of dark theme and possibly other themes
//======================================================================================================================

#ifndef COLOR_THEMES_INCLUDED
#define COLOR_THEMES_INCLUDED


#include "Common.hpp"

class QString;
class QWidget;


enum class Theme
{
	SystemDefault,
	Dark,

	_EnumEnd
};
const char * themeToString( Theme theme );
Theme themeFromString( const QString & themeStr );

/// Must be called at the start of the program, before setColorTheme() is called.
void initThemes();

/// Sets a color theme for the whole application.
/** Themes must first be initialized by calling initThemes() before calling this function. */
void setColorTheme( Theme theme );

// On Windows this needs to be called everytime a new window (dialog) is created,
// because the new title bar and window borders will always be white by default.
void updateWindowBorder( QWidget * window );


#endif // COLOR_THEMES_INCLUDED
