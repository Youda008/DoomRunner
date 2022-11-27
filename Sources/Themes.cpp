//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: implementation of dark theme and possibly other themes
//======================================================================================================================

#include "Themes.hpp"

#include "Utils/LangUtils.hpp"  // find, atScopeEndDo
#include "Utils/OSUtils.hpp"    // IS_WINDOWS

#include <QApplication>
#include <QGuiApplication>
#include <QWidget>
#include <QWindow>
#include <QStyle>
#include <QStyleFactory>
#include <QPalette>
#include <QColor>
#include <QRegularExpression>
#include <QStringBuilder>
#include <QMessageBox>
#include <QDebug>

#if IS_WINDOWS
	#include <windows.h>
	#include <dwmapi.h>
#endif

#include <algorithm>
#include <functional>
#include <optional>
#include <thread>
#include <atomic>


// Unfortunatelly, behaviour of Qt on Windows is a lot different than on Linux, so there's gonna be a lot of ifdefs.
//
// While Qt on Linux behaves as one would expect - adapts to the system settings (when you change colors or theme
// of the system, all Qt applications change with it),
// on Windows the only thing that changes is the title bar, the rest of the window stays white.
//
// To unify this behaviour, we try to detect the Windows theme via registry values and manually override the colors of
// our app when the Windows theme is set to dark.


//======================================================================================================================
//  utils

/*
static void printColorForRole( QDebug d, const QPalette & palette, QPalette::ColorRole role )
{
	d << QString("%1").arg( QVariant::fromValue(role).toString(), -15 )
	  << "   " << palette.color( QPalette::ColorGroup::Active, role ).name()
	  << "   " << palette.color( QPalette::ColorGroup::Inactive, role ).name()
	  << "   " << palette.color( QPalette::ColorGroup::Disabled, role ).name()
	  << '\n';
}

// for debugging
static void printPalette( QString name, const QPalette & palette )
{
	auto d = qDebug();
	d.noquote();
	d.nospace();

	d << '\n';
	d << name << '\n';
	d << "------------------active----inactive--disabled\n";

	printColorForRole( d, palette, QPalette::ColorRole::WindowText );
	printColorForRole( d, palette, QPalette::ColorRole::Button );
	printColorForRole( d, palette, QPalette::ColorRole::Light );
	printColorForRole( d, palette, QPalette::ColorRole::Midlight );
	printColorForRole( d, palette, QPalette::ColorRole::Dark );
	printColorForRole( d, palette, QPalette::ColorRole::Mid );
	printColorForRole( d, palette, QPalette::ColorRole::Text );
	printColorForRole( d, palette, QPalette::ColorRole::BrightText );
	printColorForRole( d, palette, QPalette::ColorRole::ButtonText );
	printColorForRole( d, palette, QPalette::ColorRole::Base );
	printColorForRole( d, palette, QPalette::ColorRole::Window );
	printColorForRole( d, palette, QPalette::ColorRole::Shadow );
	printColorForRole( d, palette, QPalette::ColorRole::Highlight );
	printColorForRole( d, palette, QPalette::ColorRole::HighlightedText );
	printColorForRole( d, palette, QPalette::ColorRole::Link );
	printColorForRole( d, palette, QPalette::ColorRole::LinkVisited );
	printColorForRole( d, palette, QPalette::ColorRole::AlternateBase );
	printColorForRole( d, palette, QPalette::ColorRole::ToolTipBase );
	printColorForRole( d, palette, QPalette::ColorRole::ToolTipText );
	printColorForRole( d, palette, QPalette::ColorRole::PlaceholderText );
}
*/

static void setColorsForRole( QPalette & palette, QPalette::ColorRole role, QColor active, QColor inactive, QColor disabled )
{
	palette.setColor( QPalette::ColorGroup::Active, role, active );
	palette.setColor( QPalette::ColorGroup::Inactive, role, inactive );
	palette.setColor( QPalette::ColorGroup::Disabled, role, disabled );
}


//======================================================================================================================
//  color schemes

static const char * const schemeStrings [] =
{
	"default",
	"dark",
	"light",
};
static_assert( size_t(ColorScheme::_EnumEnd) == std::size(schemeStrings), "Please update this table" );

static QPalette palettes [ std::size(schemeStrings) ];

// This cannot be done in a static intializer, because it depends on qApp being already initialized.
static void initColorPalettes()
{
	{
		QPalette & systemPalette = palettes[ size_t(ColorScheme::SystemDefault) ];

		systemPalette = qApp->palette();
	}

	{
		QPalette & darkPalette = palettes[ size_t(ColorScheme::Dark) ];

		// https://forum.qt.io/topic/101391/windows-10-dark-theme/4

		QColor darkColor = QColor(0x2D,0x2D,0x2D);
		QColor disabledColor = QColor(0x7F,0x7F,0x7F);
		darkPalette.setColor( QPalette::All,      QPalette::Window, darkColor );
		darkPalette.setColor( QPalette::All,      QPalette::WindowText, Qt::white );
		darkPalette.setColor( QPalette::Disabled, QPalette::WindowText, disabledColor );
		darkPalette.setColor( QPalette::All,      QPalette::Base, QColor(0x12,0x12,0x12) );
		darkPalette.setColor( QPalette::All,      QPalette::AlternateBase, darkColor );
		darkPalette.setColor( QPalette::All,      QPalette::Text, Qt::white );
		darkPalette.setColor( QPalette::Disabled, QPalette::Text, disabledColor );
		darkPalette.setColor( QPalette::All,      QPalette::Button, darkColor );
		darkPalette.setColor( QPalette::All,      QPalette::ButtonText, Qt::white );
		darkPalette.setColor( QPalette::Disabled, QPalette::ButtonText, disabledColor );
		darkPalette.setColor( QPalette::All,      QPalette::BrightText, Qt::red );
		darkPalette.setColor( QPalette::All,      QPalette::Link, QColor(0x1D,0x99,0xF3) );
		darkPalette.setColor( QPalette::All,      QPalette::Highlight, QColor(0x2A,0x82,0xDA) );
		darkPalette.setColor( QPalette::All,      QPalette::HighlightedText, Qt::black );
		darkPalette.setColor( QPalette::Disabled, QPalette::HighlightedText, disabledColor );
	}

	{

		QPalette & lightPalette = palettes[ size_t(ColorScheme::Light) ];

		// based on "Breeze Light" in KDE

		setColorsForRole( lightPalette, QPalette::WindowText,      QColor(0x232629), QColor(0x232629), QColor(0xa0a1a3) );
		setColorsForRole( lightPalette, QPalette::Button,          QColor(0xfcfcfc), QColor(0xfcfcfc), QColor(0xf0f0f0) );
		setColorsForRole( lightPalette, QPalette::Light,           QColor(0xffffff), QColor(0xffffff), QColor(0xffffff) );
		setColorsForRole( lightPalette, QPalette::Midlight,        QColor(0xf6f7f7), QColor(0xf6f7f7), QColor(0xebedee) );
		setColorsForRole( lightPalette, QPalette::Dark,            QColor(0x888e93), QColor(0x888e93), QColor(0x82878c) );
		setColorsForRole( lightPalette, QPalette::Mid,             QColor(0xc4c8cc), QColor(0xc4c8cc), QColor(0xbbc0c5) );
		setColorsForRole( lightPalette, QPalette::Text,            QColor(0x232629), QColor(0x232629), QColor(0xaaabac) );
		setColorsForRole( lightPalette, QPalette::BrightText,      QColor(0xffffff), QColor(0xffffff), QColor(0xffffff) );
		setColorsForRole( lightPalette, QPalette::ButtonText,      QColor(0x232629), QColor(0x232629), QColor(0xa8a9aa) );
		setColorsForRole( lightPalette, QPalette::Base,            QColor(0xffffff), QColor(0xffffff), QColor(0xf3f3f3) );
		setColorsForRole( lightPalette, QPalette::Window,          QColor(0xeff0f1), QColor(0xeff0f1), QColor(0xe3e5e7) );
		setColorsForRole( lightPalette, QPalette::Shadow,          QColor(0x474a4c), QColor(0x474a4c), QColor(0x474a4c) );
		setColorsForRole( lightPalette, QPalette::Highlight,       QColor(0x3daee9), QColor(0xc2e0f5), QColor(0xe3e5e7) );
		setColorsForRole( lightPalette, QPalette::HighlightedText, QColor(0xffffff), QColor(0x232629), QColor(0xa0a1a3) );
		setColorsForRole( lightPalette, QPalette::Link,            QColor(0x2980b9), QColor(0x2980b9), QColor(0xa3cae2) );
		setColorsForRole( lightPalette, QPalette::LinkVisited,     QColor(0x9b59b6), QColor(0x9b59b6), QColor(0xd6bae1) );
		setColorsForRole( lightPalette, QPalette::AlternateBase,   QColor(0xf7f7f7), QColor(0xf7f7f7), QColor(0xebebeb) );
		setColorsForRole( lightPalette, QPalette::ToolTipBase,     QColor(0xf7f7f7), QColor(0xf7f7f7), QColor(0xf7f7f7) );
		setColorsForRole( lightPalette, QPalette::ToolTipText,     QColor(0x232629), QColor(0x232629), QColor(0x232629) );
		setColorsForRole( lightPalette, QPalette::PlaceholderText, QColor(0x232629), QColor(0x232629), QColor(0x232629) );
	}

	// Define new palettes here

	/* Full palette dumps for reference

	Windows 10 default
	------------------active----inactive--disabled
	WindowText        #000000   #000000   #787878
	Button            #f0f0f0   #f0f0f0   #f0f0f0
	Light             #ffffff   #ffffff   #ffffff
	Midlight          #e3e3e3   #e3e3e3   #f7f7f7
	Dark              #a0a0a0   #a0a0a0   #a0a0a0
	Mid               #a0a0a0   #a0a0a0   #a0a0a0
	Text              #000000   #000000   #787878
	BrightText        #ffffff   #ffffff   #ffffff
	ButtonText        #000000   #000000   #787878
	Base              #ffffff   #ffffff   #f0f0f0
	Window            #f0f0f0   #f0f0f0   #f0f0f0
	Shadow            #696969   #696969   #000000
	Highlight         #0078d7   #f0f0f0   #0078d7
	HighlightedText   #ffffff   #000000   #ffffff
	Link              #0000ff   #0000ff   #0000ff
	LinkVisited       #ff00ff   #ff00ff   #ff00ff
	AlternateBase     #f5f5f5   #f5f5f5   #f5f5f5
	ToolTipBase       #ffffdc   #ffffdc   #ffffdc
	ToolTipText       #000000   #000000   #000000
	PlaceholderText   #000000   #000000   #000000

	KDE - Breeze Light
	------------------active----inactive--disabled
	WindowText        #232629   #232629   #a0a1a3
	Button            #fcfcfc   #fcfcfc   #f0f0f0
	Light             #ffffff   #ffffff   #ffffff
	Midlight          #f6f7f7   #f6f7f7   #ebedee
	Dark              #888e93   #888e93   #82878c
	Mid               #c4c8cc   #c4c8cc   #bbc0c5
	Text              #232629   #232629   #aaabac
	BrightText        #ffffff   #ffffff   #ffffff
	ButtonText        #232629   #232629   #a8a9aa
	Base              #ffffff   #ffffff   #f3f3f3
	Window            #eff0f1   #eff0f1   #e3e5e7
	Shadow            #474a4c   #474a4c   #474a4c
	Highlight         #3daee9   #c2e0f5   #e3e5e7
	HighlightedText   #ffffff   #232629   #a0a1a3
	Link              #2980b9   #2980b9   #a3cae2
	LinkVisited       #9b59b6   #9b59b6   #d6bae1
	AlternateBase     #f7f7f7   #f7f7f7   #ebebeb
	ToolTipBase       #f7f7f7   #f7f7f7   #f7f7f7
	ToolTipText       #232629   #232629   #232629
	PlaceholderText   #232629   #232629   #232629

	KDE - Breeze Dark
	------------------active----inactive--disabled
	WindowText        #fcfcfc   #fcfcfc   #6e7173
	Button            #31363b   #31363b   #2f3338
	Light             #40464c   #40464c   #3e444a
	Midlight          #363b40   #363b40   #353a3f
	Dark              #191b1d   #191b1d   #181a1c
	Mid               #25292c   #25292c   #23272a
	Text              #fcfcfc   #fcfcfc   #656768
	BrightText        #ffffff   #ffffff   #ffffff
	ButtonText        #fcfcfc   #fcfcfc   #727679
	Base              #1b1e20   #1b1e20   #1a1d1f
	Window            #2a2e32   #2a2e32   #282c30
	Shadow            #121415   #121415   #111314
	Highlight         #3daee9   #1f485e   #282c30
	HighlightedText   #fcfcfc   #fcfcfc   #6e7173
	Link              #1d99f3   #1d99f3   #1a4665
	LinkVisited       #9b59b6   #9b59b6   #443051
	AlternateBase     #232629   #232629   #212427
	ToolTipBase       #31363b   #31363b   #31363b
	ToolTipText       #fcfcfc   #fcfcfc   #fcfcfc
	PlaceholderText   #fcfcfc   #fcfcfc   #fcfcfc

	Dark override
	------------------active----inactive--disabled
	WindowText        #ffffff   #ffffff   #7f7f7f
	Button            #2d2d2d   #2d2d2d   #2d2d2d
	Light             #000000   #000000   #000000
	Midlight          #000000   #000000   #000000
	Dark              #000000   #000000   #000000
	Mid               #000000   #000000   #000000
	Text              #ffffff   #ffffff   #7f7f7f
	BrightText        #ff0000   #ff0000   #ff0000
	ButtonText        #ffffff   #ffffff   #7f7f7f
	Base              #121212   #121212   #121212
	Window            #2d2d2d   #2d2d2d   #2d2d2d
	Shadow            #000000   #000000   #000000
	Highlight         #2a82da   #2a82da   #2a82da
	HighlightedText   #000000   #000000   #7f7f7f
	Link              #2a82da   #2a82da   #2a82da
	LinkVisited       #ff00ff   #ff00ff   #ff00ff
	AlternateBase     #2d2d2d   #2d2d2d   #2d2d2d
	ToolTipBase       #ffffdc   #ffffdc   #ffffdc
	ToolTipText       #000000   #000000   #000000
	PlaceholderText   #ffffff   #ffffff   #ffffff

	*/
}

const char * schemeToString( ColorScheme scheme )
{
	if (size_t(scheme) < std::size(schemeStrings))
		return schemeStrings[ size_t(scheme) ];
	else
		return "<invalid>";
}

ColorScheme schemeFromString( const QString & schemeStr )
{
	int idx = find( schemeStrings, schemeStr );
	if (idx >= 0)
		return ColorScheme( idx );
	else
		return ColorScheme::_EnumEnd;
}

static void setQtColorScheme( ColorScheme schemeID )
{
	const QPalette & selectedPalette = palettes[ size_t(schemeID) ];

	qApp->setPalette( selectedPalette );
}


//======================================================================================================================
//  App styles

static QString defaultStyle;  ///< style active when application starts, depends on system settings
static QStringList availableStyles;  ///< styles available on this operating system and graphical environment

static void initStyles()
{
	defaultStyle = qApp->style()->objectName();
	availableStyles = QStyleFactory::keys();
}

static void setQtStyle( const QString & styleName )
{
	if (styleName.isNull())
	{
		qApp->setStyle( QStyleFactory::create( defaultStyle ) );
	}
	else if (availableStyles.contains( styleName ))
	{
		qApp->setStyle( QStyleFactory::create( styleName ) );
	}
	else
	{
		QMessageBox::warning( nullptr, "Unknown style name",
			"Unable to set application style to \""%styleName%"\". Such style doesn't exist."
		);
	}
}


//======================================================================================================================
//  Windows utils

#if IS_WINDOWS

static std::optional< DWORD > readRegistryDWORD( HKEY keyHandle, const char * subkeyPath, const char * valueName )
{
	DWORD value;
	DWORD varLength = DWORD( sizeof(value) );
	auto res = RegGetValueA(
		keyHandle, subkeyPath, valueName,
		RRF_RT_REG_DWORD,  // in: value type filter - only accept DWORD
		nullptr,           // out: final value type
		&value,            // out: the requested value
		&varLength         // in/out: size of buffer / number of bytes read
	);

	if (res != ERROR_SUCCESS)
	{
		return std::nullopt;
	}

	return value;
}

static const HKEY darkModeRootKey = HKEY_CURRENT_USER;
static const char * const darkModeSubkeyPath = "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
static const char * const darkModeValueName = "AppsUseLightTheme";

static bool isSystemDarkModeEnabled()
{
	// based on https://stackoverflow.com/questions/51334674/how-to-detect-windows-10-light-dark-mode-in-win32-application
	auto optAppsUseLightTheme = readRegistryDWORD( darkModeRootKey, darkModeSubkeyPath, darkModeValueName );
	return optAppsUseLightTheme.has_value() && optAppsUseLightTheme.value() == 0;
}

static void watchForSystemDarkModeChanges( std::function< void ( bool darkModeEnabled ) > onChange )
{
	HKEY hThemeSettingsKey;
	std::optional< DWORD > optAppsUseLightTheme;

	LONG lErrorCode = RegOpenKeyExA(
		darkModeRootKey, darkModeSubkeyPath,
		0,                             // in: options
		KEY_QUERY_VALUE | KEY_NOTIFY,  // in: requested permissions
		&hThemeSettingsKey             // out: handle to the opened key
	);
	if (lErrorCode != ERROR_SUCCESS)
	{
		qWarning() << "cannot open registry key: HKEY_CURRENT_USER /" << darkModeSubkeyPath;
		return;
	}

	auto guard = atScopeEndDo( [&]() { RegCloseKey( hThemeSettingsKey ); } );

	optAppsUseLightTheme = readRegistryDWORD( hThemeSettingsKey, nullptr, darkModeValueName );
	if (!optAppsUseLightTheme)
	{
		qWarning() << "cannot read registry value" << darkModeValueName << "(error" << GetLastError() << ")";
		return;
	}

	DWORD lastAppsUseLightTheme = *optAppsUseLightTheme;

	while (true)
	{
		lErrorCode = RegNotifyChangeKeyValue(
			hThemeSettingsKey,      // handle to the opened key
			FALSE,     // monitor subtree
			REG_NOTIFY_CHANGE_LAST_SET,  // notification filter
			nullptr,   // handle to event object
			FALSE      // asynchronously
		);
		if (lErrorCode != ERROR_SUCCESS)
		{
			qWarning() << "RegNotifyChangeKeyValue failed";
			Sleep( 1000 );
			continue;
		}

		optAppsUseLightTheme = readRegistryDWORD( hThemeSettingsKey, nullptr, darkModeValueName );
		if (!optAppsUseLightTheme)
		{
			qWarning() << "cannot read registry value" << darkModeValueName << "(error" << GetLastError() << ")";
			break;
		}

		if (*optAppsUseLightTheme != lastAppsUseLightTheme)  // value has changed
		{
			onChange( *optAppsUseLightTheme == 0 );
			lastAppsUseLightTheme = *optAppsUseLightTheme;
		}
	}
}

static void toggleDarkTitleBar( HWND hWnd, bool enable )
{
	// based on https://stackoverflow.com/a/70693198/3575426
	DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 20;  // until Windows SDK 10.0.22000.0 (first Windows 11 SDK) this value is not defined
	BOOL useDarkMode = BOOL(enable);
	DwmSetWindowAttribute( hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode) );
}

static void toggleDarkTitleBars( bool enable )
{
	QWindow * focusWindow = qApp->focusWindow();
	for (QWindow * window : qApp->topLevelWindows())
	{
		HWND hWnd = reinterpret_cast< HWND >( window->winId() );
		toggleDarkTitleBar( hWnd, enable );
		// This is the only way how to force the window title bar to redraw with the new settings.
		SetFocus( hWnd );
	}
	if (focusWindow)
	{
		SetFocus( reinterpret_cast< HWND >( focusWindow->winId() ) );
	}
}

#endif // IS_WINDOWS


//======================================================================================================================
//  main logic

// make sure these variables are only accessed from the main thread, or make them std::atomic
static ColorScheme g_currentUserSchemeID = ColorScheme::SystemDefault;  ///< the scheme user chose via setAppColorScheme()
static ColorScheme g_currentRealSchemeID = ColorScheme::SystemDefault;  ///< the scheme that was really set after examining system settings
static QString g_currentStyle;  ///< the application style the user chose via setAppStyle()

namespace themes {

void init()
{
	initColorPalettes();  // initialize color scheme definitions

	initStyles();  // initialize available style names

 #if IS_WINDOWS
	// Qt on Windows does not automatically follow OS preferences, so when the application starts
	// we have to check the OS settings and manually override the default theme with our dark one in case it's enabled.
	// Later options.json may change this, but let's first open the app with the correct system theme.
	if (isSystemDarkModeEnabled())
	{
		setQtColorScheme( ColorScheme::Dark );
	}
 #endif
}

void setAppColorScheme( ColorScheme userSchemeID )
{
	ColorScheme realSchemeID = userSchemeID;
	g_currentUserSchemeID = userSchemeID;

 #if IS_WINDOWS
	// Qt on Windows does not automatically follow OS preferences, so we have to check the OS settings
	// and manually override the user-selected default theme with our dark one in case it's enabled.
	const bool systemDarkModeEnabled = isSystemDarkModeEnabled();
	if (userSchemeID == ColorScheme::SystemDefault && systemDarkModeEnabled)
	{
		realSchemeID = ColorScheme::Dark;
	}
 #endif

	if (realSchemeID == g_currentRealSchemeID)
	{
		return;  // nothing to be done, this scheme is already active
	}
	g_currentRealSchemeID = realSchemeID;

	setQtColorScheme( realSchemeID );

 #if IS_WINDOWS
	// On Windows the title bar follows the system preferences and isn't controlled by Qt,
	// so in case the user requests explicit dark theme we use this hack to make it dark too.
	toggleDarkTitleBars( realSchemeID == ColorScheme::Dark && !systemDarkModeEnabled );
	// The dark colors don't work well with the default Windows style.
	// We have to use "Fusion", that's the only style where it looks good.
	if (realSchemeID == ColorScheme::Dark)
	{
		setAppStyle( "Fusion" );
	}
 #endif
}

void setAppStyle( const QString & styleName )
{
	if (styleName == g_currentStyle)
	{
		return;  // nothing to be done, this style is already active
	}
	g_currentStyle = styleName;

	setQtStyle( styleName );
}

QStringList getAvailableAppStyles()
{
	return availableStyles;
}

QString getDefaultAppStyle()
{
	return defaultStyle;
}

void updateWindowBorder( [[maybe_unused]] QWidget * window )
{
 #if IS_WINDOWS
	if (g_currentRealSchemeID == ColorScheme::Dark && !isSystemDarkModeEnabled())
	{
		toggleDarkTitleBar( reinterpret_cast< HWND >( window->winId() ), true );
		// This is the only way how to force the window title bar to redraw with the new settings.
		SetFocus( reinterpret_cast< HWND >( static_cast< QWidget * >( window->parent() )->winId() ) );
		SetFocus( reinterpret_cast< HWND >( window->winId() ) );
	}
 #endif
}

QString updateHyperlinkColor( const QString & richText )
{
	QString htmlColor = palettes[ size_t( g_currentRealSchemeID ) ].color( QPalette::Link ).name();
	QString newText( richText );
	static QRegularExpression regex("color:#[0-9a-fA-F]{6}");
	newText.replace( regex, "color:"+htmlColor );
	return newText;
}

} // namespace themes


//======================================================================================================================
//  WindowsThemeWatcher

WindowsThemeWatcher::WindowsThemeWatcher()
{
 #if IS_WINDOWS
	// If this object is constructed in the main thread, this will make the updateTheme be called in the main thread.
	connect( this, &WindowsThemeWatcher::darkModeToggled, this, &WindowsThemeWatcher::updateScheme );
 #endif
}

void WindowsThemeWatcher::run()
{
 #if IS_WINDOWS
	watchForSystemDarkModeChanges( [this]( bool darkModeEnabled )
	{
		emit darkModeToggled( darkModeEnabled );
	});
 #endif
}

void WindowsThemeWatcher::updateScheme( [[maybe_unused]] bool darkModeEnabled )
{
	// This will be executed in the main thread.
 #if IS_WINDOWS
	if (g_currentUserSchemeID == ColorScheme::SystemDefault)
	{
		ColorScheme realSchemeID = darkModeEnabled ? ColorScheme::Dark : ColorScheme::SystemDefault;
		if (realSchemeID == g_currentRealSchemeID)
		{
			return;  // nothing to be done, this scheme is already active
		}
		g_currentRealSchemeID = realSchemeID;

		setQtColorScheme( realSchemeID );
		toggleDarkTitleBars( realSchemeID == ColorScheme::Dark && !darkModeEnabled );
		if (realSchemeID == ColorScheme::Dark)
		{
			themes::setAppStyle( "Fusion" );
		}
	}
 #endif
}
