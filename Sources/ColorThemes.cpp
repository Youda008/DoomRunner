//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: implementation of dark theme and possibly other themes
//======================================================================================================================

#include "ColorThemes.hpp"

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
//  theme definitions

static const char * const themeStrings [] =
{
	"default",
	"dark",
};
static_assert( size_t(Theme::_EnumEnd) == std::size(themeStrings), "Please update this table" );

struct ThemeDef
{
	QString styleName;
	QPalette palette;
};

static ThemeDef themeDefs [ std::size(themeStrings) ];

// This cannot be done in a static intializer, because it depends on qApp being already initialized.
static void defineThemes()
{
	{
		ThemeDef & systemTheme = themeDefs[ size_t(Theme::SystemDefault) ];

		systemTheme.styleName = qApp->style()->objectName();
		systemTheme.palette = qApp->palette();
	}

	{
		ThemeDef & darkTheme = themeDefs[ size_t(Theme::Dark) ];

		// https://forum.qt.io/topic/101391/windows-10-dark-theme/4

		darkTheme.styleName = "Fusion";

		QColor darkColor = QColor(45,45,45);
		QColor disabledColor = QColor(127,127,127);
		darkTheme.palette.setColor( QPalette::Window, darkColor );
		darkTheme.palette.setColor( QPalette::WindowText, Qt::white );
		darkTheme.palette.setColor( QPalette::Base, QColor(18,18,18) );
		darkTheme.palette.setColor( QPalette::AlternateBase, darkColor );
		darkTheme.palette.setColor( QPalette::Text, Qt::white );
		darkTheme.palette.setColor( QPalette::Disabled, QPalette::Text, disabledColor );
		darkTheme.palette.setColor( QPalette::Button, darkColor );
		darkTheme.palette.setColor( QPalette::ButtonText, Qt::white );
		darkTheme.palette.setColor( QPalette::Disabled, QPalette::ButtonText, disabledColor );
		darkTheme.palette.setColor( QPalette::BrightText, Qt::red );
		darkTheme.palette.setColor( QPalette::Link, QColor(42,130,218) );
		darkTheme.palette.setColor( QPalette::Highlight, QColor(42,130,218) );
		darkTheme.palette.setColor( QPalette::HighlightedText, Qt::black );
		darkTheme.palette.setColor( QPalette::Disabled, QPalette::HighlightedText, disabledColor );
	}

	// Define new themes here
}


//======================================================================================================================
//  Theme enum conversion

const char * themeToString( Theme theme )
{
	if (size_t(theme) < std::size(themeStrings))
		return themeStrings[ size_t(theme) ];
	else
		return "<invalid>";
}

Theme themeFromString( const QString & themeStr )
{
	int idx = find( themeStrings, themeStr );
	if (idx >= 0)
		return Theme( idx );
	else
		return Theme::_EnumEnd;
}


//======================================================================================================================
//  Windows utils

#if IS_WINDOWS

std::optional< DWORD > readRegistryDWORD( HKEY keyHandle, const char * subkeyPath, const char * valueName )
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

bool isSystemDarkModeEnabled()
{
	// based on https://stackoverflow.com/questions/51334674/how-to-detect-windows-10-light-dark-mode-in-win32-application
	auto optAppsUseLightTheme = readRegistryDWORD( darkModeRootKey, darkModeSubkeyPath, darkModeValueName );
	return optAppsUseLightTheme.has_value() && optAppsUseLightTheme.value() == 0;
}

void watchForSystemDarkModeChanges( std::function< void ( bool darkModeEnabled ) > onChange )
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

void toggleDarkTitleBar( HWND hWnd, bool enable )
{
	// based on https://stackoverflow.com/a/70693198/3575426
	DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 20;  // until Windows SDK 10.0.22000.0 (first Windows 11 SDK) this value is not defined
	BOOL useDarkMode = BOOL(enable);
	DwmSetWindowAttribute( hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode) );
}

void toggleDarkTitleBars( bool enable )
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

static std::atomic< Theme > g_currentThemeID = Theme::SystemDefault;

void setQtTheme( Theme themeID )
{
	const ThemeDef & selectedTheme = themeDefs[ size_t(themeID) ];

	qApp->setStyle( QStyleFactory::create( selectedTheme.styleName ) );
	qApp->setPalette( selectedTheme.palette );
}

void initThemes()
{
	// initialize theme definitions
	defineThemes();

 #if IS_WINDOWS
	// Qt on Windows does not automatically follow OS preferences, so when the application starts
	// we have to check the OS settings and manually override the default theme with our dark one in case it's enabled.
	// Later options.json may change this, but let's first open the app with the correct system theme.
	if (isSystemDarkModeEnabled())
	{
		setQtTheme( Theme::Dark );
	}
 #endif
}

void setColorTheme( Theme themeID )
{
	if (themeID == g_currentThemeID)
	{
		return;
	}
	g_currentThemeID = themeID;

 #if IS_WINDOWS
	// Qt on Windows does not automatically follow OS preferences, so we have to check the OS settings
	// and manually override the default theme with our dark one in case it's enabled.
	const bool systemDarkModeEnabled = isSystemDarkModeEnabled();
	if (themeID == Theme::SystemDefault && systemDarkModeEnabled)
	{
		themeID = Theme::Dark;
	}
 #endif

	setQtTheme( themeID );

 #if IS_WINDOWS
	// On Windows the title bar follows the system preferences and isn't controlled by Qt,
	// so in case the user requests explicit dark theme we use this hack to make it dark too.
	toggleDarkTitleBars( themeID == Theme::Dark && !systemDarkModeEnabled );
 #endif
}

void updateWindowBorder( [[maybe_unused]] QWidget * window )
{
 #if IS_WINDOWS
	if (g_currentThemeID == Theme::Dark)
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
 #if IS_WINDOWS
	QString htmlColor = themeDefs[ size_t( g_currentThemeID.load() ) ].palette.color( QPalette::Link ).name();
	QString newText( richText );
	static QRegularExpression regex("color:#[0-9a-fA-F]{6}");
	newText.replace( regex, "color:"+htmlColor );
	qDebug() << richText;
	qDebug() << newText;
	return newText;
 #else
	return richText;
 #endif
}


//======================================================================================================================
//  SystemThemeWatcher

SystemThemeWatcher::SystemThemeWatcher()
{
 #if IS_WINDOWS
	// If this object is constructed in the main thread, this will make the updateTheme be called in the main thread.
	connect( this, &SystemThemeWatcher::darkModeToggled, this, &SystemThemeWatcher::updateTheme );
 #endif
}

void SystemThemeWatcher::run()
{
 #if IS_WINDOWS
	watchForSystemDarkModeChanges( [this]( bool darkModeEnabled )
	{
		emit darkModeToggled( darkModeEnabled );
	});
 #endif
}

void SystemThemeWatcher::updateTheme( [[maybe_unused]] bool darkModeEnabled )
{
 #if IS_WINDOWS
	if (g_currentThemeID == Theme::SystemDefault)
	{
		setQtTheme( darkModeEnabled ? Theme::Dark : Theme::SystemDefault );
	}
 #endif
}
