//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description:
//======================================================================================================================

#include "Common.hpp"

#include "MainWindow.hpp"
#include "Themes.hpp"

#include <QApplication>
#include <QDir>


//======================================================================================================================

int main( int argc, char * argv [] )
{
	QApplication a( argc, argv );

	// current dir needs to be set to the application's dir so that the app finds the user files with relative paths
	QDir::setCurrent( QApplication::applicationDirPath() );

	themes::init();

	MainWindow w;
	w.show();
	int exitCode = a.exec();

	return exitCode;
}
