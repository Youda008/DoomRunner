//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: primitive viewer of WAD text description
//======================================================================================================================

#include "WADDescViewer.hpp"
#include "ui_WADDescViewer.h"

#include "MainWindowPtr.hpp"
#include "Utils/FileSystemUtils.hpp"  // replaceFileSuffix
#include "Utils/StringUtils.hpp"      // capitalize
#include "Utils/ErrorHandling.hpp"

#include <QString>
#include <QFileInfo>
#include <QFile>
#include <QFont>
#include <QFontDatabase>


//======================================================================================================================

WADDescViewer::WADDescViewer( QWidget * parent, const QString & fileName, const QString & content, bool wrapLines )
:
	QDialog( parent ),
	DialogCommon( this, u"WADDescViewer" ),
	wrapLines( wrapLines )
{
	ui = new Ui::WADDescViewer;
	ui->setupUi( this );
	setupUi_custom( wrapLines );

	this->setWindowTitle( fileName );
	ui->textEdit->setPlainText( content );

	ui->wrapLinesAction->setChecked( wrapLines );

	connect( ui->closeAction, &QAction::triggered, this, &QDialog::accept );
	connect( ui->wrapLinesAction, &QAction::triggered, this, &ThisClass::toggleLineWrap );
}

void WADDescViewer::setupUi_custom( bool wrapLines )
{
	this->setWindowModality( Qt::WindowModal );

	// setup the text edit area
	ui->textEdit->setReadOnly( true );
	ui->textEdit->setWordWrapMode( wrapLines ? QTextOption::WordWrap : QTextOption::NoWrap );
	QFont font = QFontDatabase::systemFont( QFontDatabase::FixedFont );
	font.setPointSize( 10 );
	ui->textEdit->setFont( font );

	// estimate the optimal window size
	int dialogWidth  = int( 75.0f * float( font.pointSize() ) * 0.84f ) + 30;
	int dialogHeight = int( 40.0f * float( font.pointSize() ) * 1.62f ) + 30;
	this->resize( dialogWidth, dialogHeight );

	// position it to the right of the center of the main window
	QRect parentGeometry = qMainWindow->geometry();
	int parentCenterX = parentGeometry.x() + (parentGeometry.width() / 2);
	int parentCenterY = parentGeometry.y() + (parentGeometry.height() / 2);
	QPoint globalPos( parentCenterX, parentCenterY );  //parentWindow->mapToGlobal( QPoint( parentCenterX, parentCenterY ) );
	this->move( globalPos.x(), globalPos.y() - (this->height() / 2) );
}

WADDescViewer::~WADDescViewer()
{
	delete ui;
}

void WADDescViewer::toggleLineWrap()
{
	auto currentMode = ui->textEdit->wordWrapMode();
	auto newMode = currentMode == QTextOption::NoWrap ? QTextOption::WordWrap : QTextOption::NoWrap;
	wrapLines = newMode != QTextOption::NoWrap;
	ui->textEdit->setWordWrapMode( newMode );
	ui->wrapLinesAction->setChecked( wrapLines );
}


//----------------------------------------------------------------------------------------------------------------------

void showTxtDescriptionFor( QWidget * parentWindow, const QString & filePath, const QString & contentType, bool & wrapLines )
{
	QFileInfo dataFileInfo( filePath );

	if (!dataFileInfo.isFile())  // user could click on a directory
	{
		return;
	}

	// get the corresponding file with txt suffix
	QFileInfo descFileInfo( fs::replaceFileSuffix( filePath, "txt" ) );
	if (!descFileInfo.isFile())
	{
		// try TXT in case we are in a case-sensitive file-system such as Linux
		descFileInfo = QFileInfo( fs::replaceFileSuffix( filePath, "TXT" ) );
		if (!descFileInfo.isFile())
		{
			reportUserError( parentWindow, "Cannot open "%contentType,
				capitalize( contentType )%" file \""%fs::replaceFileSuffix( filePath, "txt" )%"\" does not exist" );
			return;
		}
	}

	QFile descFile( descFileInfo.filePath() );
	if (!descFile.open( QIODevice::Text | QIODevice::ReadOnly ))
	{
		reportRuntimeError( parentWindow, "Cannot open "%contentType,
			"Failed to open map "%contentType%" \""%descFileInfo.fileName()%"\" ("%descFile.errorString()%")" );
		return;
	}

	QByteArray desc = descFile.readAll();

	WADDescViewer descDialog( parentWindow, descFileInfo.fileName(), desc, wrapLines );

	descDialog.exec();

	wrapLines = descDialog.wrapLines;
}
