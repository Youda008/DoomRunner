//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: implementation of dark theme and possibly other themes
//======================================================================================================================

#ifndef THEMES_INCLUDED
#define THEMES_INCLUDED


#include "Common.hpp"

#include <QThread>
#include <QStringList>

class QWidget;


//======================================================================================================================

enum class ColorScheme
{
	SystemDefault,
	Dark,
	// for explicit White scheme we would need a custom palette for Linux

	_EnumEnd
};
const char * schemeToString( ColorScheme scheme );
ColorScheme schemeFromString( const QString & schemeStr );


//======================================================================================================================

namespace themes {

/// Must be called at the start of the program, before setColorScheme() is called.
void init();

/// Sets a color scheme for the whole application.
/** init() must be called before calling this function. */
void setAppColorScheme( ColorScheme scheme );

/// Sets a visual style for the whole application.
/** If the style name is null string - QString(), system default is selected.
  * Passing in a non-existing style name will result in an error message box.
  * init() must be called before calling this function. */
void setAppStyle( const QString & styleName );

/// Returns possible values to the setAppStyle() function.
/** Determined by operating system, graphical environment and installed plugins. */
QStringList getAvailableAppStyles();

// On Windows this needs to be called everytime a new window (dialog) is created,
// because the new title bar and window borders are not automatically changed by Qt.
void updateWindowBorder( QWidget * window );

// On Windows, palette.setColor( QPalette::Link, ... ) does not work, so we have to change the hyperlink color
// by manually editing each widget's rich text.
QString updateHyperlinkColor( const QString & richText );

} // namespace themes


//======================================================================================================================
/// Monitors changes to the theme settings of the operating system.
/** Qt on Windows does not automatically follow OS preferences, so the OS theme settings have to be manually monitored
  * in a background thread, and our theme manually updated whenever it changes.
  * Construct this object in a main thread and call start(), that will ensure the theme update is performed in
  * the main thread even though the monitoring will be done in a background thread.
  */
class SystemThemeWatcher : public QThread {  // TODO

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
	void updateScheme( bool darkModeEnabled );

};


#endif // THEMES_INCLUDED
