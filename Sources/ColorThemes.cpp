//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: implementation of dark theme and possibly other themes
//======================================================================================================================

#include "ColorThemes.hpp"

#include "LangUtils.hpp"

#include <QApplication>
#include <QGuiApplication>
#include <QWidget>
#include <QWindow>
#include <QStyle>
#include <QStyleFactory>
#include <QPalette>
#include <QColor>

#ifdef _WIN32
	#include <windows.h>
	#include <dwmapi.h>
#endif // _WIN32

#include <algorithm>



//======================================================================================================================
//  themes

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

static ThemeDef themes [ std::size(themeStrings) ];

void initThemes()
{
	{
		ThemeDef & theme = themes[ size_t(Theme::SystemDefault) ];

		theme.styleName = qApp->style()->objectName();
		theme.palette = qApp->palette();
	}

	{
		ThemeDef & theme = themes[ size_t(Theme::Dark) ];

		// https://forum.qt.io/topic/101391/windows-10-dark-theme/4
		theme.styleName = "Fusion";
		QColor darkColor = QColor(45,45,45);
		QColor disabledColor = QColor(127,127,127);
		theme.palette.setColor( QPalette::Window, darkColor );
		theme.palette.setColor( QPalette::WindowText, Qt::white );
		theme.palette.setColor( QPalette::Base, QColor(18,18,18) );
		theme.palette.setColor( QPalette::AlternateBase, darkColor );
		theme.palette.setColor( QPalette::Text, Qt::white );
		theme.palette.setColor( QPalette::Disabled, QPalette::Text, disabledColor );
		theme.palette.setColor( QPalette::Button, darkColor );
		theme.palette.setColor( QPalette::ButtonText, Qt::white );
		theme.palette.setColor( QPalette::Disabled, QPalette::ButtonText, disabledColor );
		theme.palette.setColor( QPalette::BrightText, Qt::red );
		theme.palette.setColor( QPalette::Link, QColor(42,130,218) );
		theme.palette.setColor( QPalette::Highlight, QColor(42,130,218) );
		theme.palette.setColor( QPalette::HighlightedText, Qt::black );
		theme.palette.setColor( QPalette::Disabled, QPalette::HighlightedText, disabledColor );
	}
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

#ifdef _WIN32

void toggleImmersiveDarkMode( HWND windowHandle, bool enable )
{
	// based on https://stackoverflow.com/a/70693198/3575426
	DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 20;  // until Windows SDK 10.0.22000.0 (first Windows 11 SDK) this value is not defined
	BOOL useDarkMode = enable;
	DwmSetWindowAttribute( windowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode) );
}

HWND toWindowHandle( WId wid )
{
	return reinterpret_cast< HWND >( wid );
}

#endif // _WIN32


//======================================================================================================================

static Theme currentTheme = Theme::SystemDefault;

void setColorTheme( Theme themeID )
{
	currentTheme = themeID;

	const ThemeDef & theme = themes[ size_t(themeID) ];

	qApp->setStyle( QStyleFactory::create( theme.styleName ) );
	qApp->setPalette( theme.palette );

 #ifdef _WIN32
	// On Windows the title bar stays white so we use this hack to make it dark too.
	QWindow * focusWindow = qApp->focusWindow();
	for (QWindow * window : qApp->topLevelWindows())
	{
		toggleImmersiveDarkMode( toWindowHandle( window->winId() ), themeID == Theme::Dark );
		// This is the only way how to force the window title bar to redraw with the new settings.
		SetFocus( toWindowHandle( window->winId() ) );
	}
	if (focusWindow) SetFocus( toWindowHandle( focusWindow->winId() ) );
 #endif // _WIN32
}

void updateWindowBorder( QWidget * window )
{
 #ifdef _WIN32
	if (currentTheme == Theme::Dark)
	{
		toggleImmersiveDarkMode( toWindowHandle( window->winId() ), true );
		SetFocus( toWindowHandle( static_cast< QWidget * >( window->parent() )->winId() ) );
		SetFocus( toWindowHandle( window->winId() ) );
	}
 #endif // _WIN32
}
