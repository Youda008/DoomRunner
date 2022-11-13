//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: implementation of dark theme and possibly other themes
//======================================================================================================================

#ifndef COLOR_THEMES_INCLUDED
#define COLOR_THEMES_INCLUDED


#include "Common.hpp"

#include <QThread>

class QString;
class QWidget;


//======================================================================================================================

enum class Theme
{
	SystemDefault,
	Dark,
	// for explicit White theme we would need a custom palette for Linux

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
// because the new title bar and window borders are not automatically changed by Qt.
void updateWindowBorder( QWidget * window );


/// Monitors changes to the theme settings of the operating system.
/** Qt on Windows does not automatically follow OS preferences, so the OS theme settings have to be manually monitored
  * in a background thread, and our theme manually updated whenever it changes.
  * Construct this object in a main thread and call start(), that will ensure the theme update is performed in
  * the main thread even though the monitoring will be done in a background thread. */
class SystemThemeWatcher : public QThread {

	Q_OBJECT

 public:

	SystemThemeWatcher();

 protected:

	virtual void run() override;

 signals:

	/// Emitted whenever dark mode is enabled or disabled in the system settings.
	void darkModeToggled( bool darkModeEnabled );

 private slots:

	/// Automatically called from the thread that constructed this object, whenever dark mode is toggled in the system settings.
	void updateTheme( bool darkModeEnabled );

};


#endif // COLOR_THEMES_INCLUDED
