//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: implementation of dark theme and possibly other themes
//======================================================================================================================

#ifndef THEMES_INCLUDED
#define THEMES_INCLUDED


#include "Essential.hpp"

#include "Utils/ErrorHandling.hpp"  // LoggingComponent

#include <QThread>
#include <QStringList>
#include <QPalette>

#include <mutex>

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
	QColor invalidEntryText;       ///< text color for a file/directory that doesn't exist or has a wrong type
	QColor toBeCreatedEntryText;   ///< text color for a file/directory that doesn't exist but can be created
	QColor defaultEntryText;       ///< text color for a file/directory that is set as default
	QColor separatorText;          ///< text color for an entry that represents a visual separator
	QColor separatorBackground;    ///< background color for an entry that represents a visual separator
};


//======================================================================================================================

namespace themes {


/// Must be called at the start of the program, before the following functions are called.
void init();


// color schemes

/// Sets a color scheme for the whole application.
/** init() must be called before calling this function. */
void setAppColorScheme( ColorScheme scheme );

const Palette & getCurrentPalette();

// Sometimes hyperlinks in a widget's text specify color in HTML tag, which overrides palette.setColor( QPalette::Link, ... )
// In such case this needs to be called to update the HTML tag color.
QString updateHyperlinkColor( QString richText );

// On Windows this needs to be called everytime a new window (dialog) is created,
// because the new title bar and window borders are not automatically changed by Qt.
void updateWindowBorder( QWidget * window );


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


} // namespace themes


//======================================================================================================================
// system theme settings monitoring

enum class SystemTheme
{
	Light,
	Dark,
};

/// Monitors changes to the theme settings of the operating system.
/**
  * Qt on Windows does not automatically follow OS preferences, so the OS theme settings have to be manually monitored
  * in a background thread, and our theme manually updated whenever it changes.
  * Construct this object in a main thread and call start(), that will ensure the theme update is performed in
  * the main thread even though the monitoring will be done in a background thread.
  */
class SystemThemeWatcher : public QThread, protected LoggingComponent {

	Q_OBJECT

 public:

	SystemThemeWatcher();
	virtual ~SystemThemeWatcher() override;

	/// Starts a background thread that monitors the system theme settings and automatically updates Qt theme whenever it changes.
	bool start();

	/// Signals the background thread to quit and waits timeout_ms milliseconds for it to exit.
	/** If the thread does not exit in time, false is returned and stop() can be called again.
	  * If the thread is still running when this object is destroyed, the thread is forcefully teminated. */
	bool stop( ulong timeout_ms );

 private:

	virtual void run() override;

 signals:

	/// Emitted whenever dark mode is enabled or disabled in the system settings.
	void systemThemeChanged( SystemTheme newTheme );

 private slots:

	/// Automatically called from the thread that constructed this object, whenever system theme changes.
	void updateQtScheme( SystemTheme systemTheme );

 public:

	/// Interface for OS-specific implementation of the theme settings monitoring
	struct SystemThemeWatcherImpl
	{
		virtual ~SystemThemeWatcherImpl() = default;

		/// Opens the system theme settings and prepares them to be monitored.
		/** Must be called and succeed before calling monitorThemeSettingsChanges(). */
		virtual bool openThemeSettingsMonitoring() = 0;

		/// Closes the system theme settings and aborts any monitoring currently running.
		/** Once a thread enters the monitorThemeSettingsChanges(), this is the only way to make the function return. */
		virtual void closeThemeSettingsMonitoring() = 0;

		/// Whether the system theme settings are still open.
		virtual bool isThemeSettingsMonitoringOpen() const = 0;

		enum class QuitReason
		{
			MonitoringClosed,  ///< function ended due to openThemeSettingsMonitoring() being called from another thread
			ReadingError,      ///< function ended due to error while trying to read the theme settings
			MiscError,
		};

		/// Enters an infinite loop that waits for system theme settings changes and calls a callback when it does.
		/** This is a blocking function that will return only when error occurs or when monitoring is aborted by calling
		  * closeThemeSettingsMonitoring() from another thread. It must be run in a thread dedicated only for this job.
		  * Return value indicates whether the monitoring ended with an error or was externally aborted.
		  * When the function inadvertently returns due to an error, the theme settings must be closed manually
		  * by calling closeThemeSettingsMonitoring(). */
		virtual QuitReason monitorThemeSettingsChanges( std::function< void ( SystemTheme newTheme ) > onThemeChange ) = 0;
	};

 private: // members

	// pimpl idiom
	// Avoids including Windows headers in this header and poluting the global namespace with problematic macros.
	std::unique_ptr< SystemThemeWatcherImpl > _impl;

	bool _started;  ///< indicates only that the start() call suceeded and stop() was not called yet, does NOT indicate that the thread is actually running
	std::mutex _monitoringMtx;  ///< mutex to protect the state of theme settings monitoring

};


#endif // THEMES_INCLUDED
