//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description:
//======================================================================================================================

#include "MainWindow.hpp"
#include "MainWindowPtr.hpp"
#include "Themes.hpp"
#include "DoomFiles.hpp"
#include "Utils/StandardOutput.hpp"

#include <QApplication>
#include <QDir>


QMainWindow * qMainWindow = nullptr;


int main( int argc, char * argv [] )
{
	QApplication a( argc, argv );

	// All stored relative paths are relative to the directory of this application,
	// launching it from a different current working directory would break it.
	QDir::setCurrent( QApplication::applicationDirPath() );

	initStdStreams();

	themes::init();

	doom::initFileNameSuffixes();

	auto mainWindow = std::make_unique< MainWindow >();  // allocate dynamically to not consume too much stack
	qMainWindow = mainWindow.get();

	mainWindow->show();
	int exitCode = a.exec();

	return exitCode;
}
