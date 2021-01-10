//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  13.5.2019
// Description:
//======================================================================================================================

#include "Common.hpp"

#include "MainWindow.hpp"

#include <QApplication>
#include <QDir>


//======================================================================================================================

int main( int argc, char * argv [] )
{
	QApplication a( argc, argv );

	// current dir needs to be set to the application's dir so that the app finds its files
	QDir::setCurrent( QApplication::applicationDirPath() );

	MainWindow w;
	w.show();
	return a.exec();
}
