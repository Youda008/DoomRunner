//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description:
//======================================================================================================================

#include "MainWindow.hpp"
#include "Themes.hpp"
#include "Utils/StandardOutput.hpp"

#include <QApplication>
#include <QDir>


//======================================================================================================================

int main( int argc, char * argv [] )
{
	QApplication a( argc, argv );

	// All stored relative paths are relative to the directory of this application,
	// launching it from a different current working directory would break it.
	QDir::setCurrent( QApplication::applicationDirPath() );

	initStdStreams();

	themes::init();

	auto w = std::make_unique< MainWindow >();  // don't consume so much stack
	w->show();
	int exitCode = a.exec();

	return exitCode;
}
