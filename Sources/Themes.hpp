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
#include <QPalette>

class QWidget;


//======================================================================================================================

enum class ColorScheme
{
	SystemDefault,
	Dark,
	Light,

	_EnumEnd
};
const char * schemeToString( ColorScheme scheme );
ColorScheme schemeFromString( const QString & schemeStr );

/// Our own extended color palette.
struct Palette : public QPalette
{
	QColor defaultEntryText;
	QColor separatorText = Qt::black;
	QColor separatorBackground = QRgb( 0xA0A0A0 );
};


//======================================================================================================================

namespace themes {


/// Must be called at the start of the program, before the following functions are called.
void init();


// app styles

/// Returns possible values to the setAppStyle() function.
/** Determined by operating system, graphical environment and installed plugins. */
QStringList getAvailableAppStyles();

/// Returns which of the available app styles from getAvailableAppStyles() is the default on this operating system.
QString getDefaultAppStyle();

/// Sets a visual style for the whole application.
/** If the style name is null string - QString(), system default is selected.
  * Passing in a non-existing style name will result in an error message box.
  * init() must be called before calling this function. */
void setAppStyle( const QString & styleName );

// On Windows this needs to be called everytime a new window (dialog) is created,
// because the new title bar and window borders are not automatically changed by Qt.
void updateWindowBorder( QWidget * window );


// color schemes

/// Sets a color scheme for the whole application.
/** init() must be called before calling this function. */
void setAppColorScheme( ColorScheme scheme );

const Palette & getCurrentPalette();

// Sometimes hyperlinks in a widget's text specify color in HTML tag, which overrides palette.setColor( QPalette::Link, ... )
// In such case this needs to be called to update the HTML tag color.
QString updateHyperlinkColor( const QString & richText );


} // namespace themes


//======================================================================================================================
/// Monitors changes to the theme settings of the operating system.
/** Qt on Windows does not automatically follow OS preferences, so the OS theme settings have to be manually monitored
  * in a background thread, and our theme manually updated whenever it changes.
  * Construct this object in a main thread and call start(), that will ensure the theme update is performed in
  * the main thread even though the monitoring will be done in a background thread.
  */
#ifdef _WIN32
class WindowsThemeWatcher : public QThread {

	Q_OBJECT

 public:

	WindowsThemeWatcher();

 protected:

	virtual void run() override;

 signals:

	/// Emitted whenever dark mode is enabled or disabled in the system settings.
	void darkModeToggled( bool darkModeEnabled );

 private slots:

	/// Automatically called from the thread that constructed this object, whenever dark mode is toggled in the system settings.
	void updateScheme( bool darkModeEnabled );

};
#endif // _WIN32


#endif // THEMES_INCLUDED
