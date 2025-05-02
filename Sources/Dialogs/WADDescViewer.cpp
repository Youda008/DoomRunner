//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: primitive viewer of WAD text description
//======================================================================================================================

#include "WADDescViewer.hpp"

#include "Utils/FileSystemUtils.hpp"  // replaceFileSuffix
#include "Utils/StringUtils.hpp"      // capitalize
#include "Utils/ErrorHandling.hpp"

#include <QString>
#include <QDialog>
#include <QFileInfo>
#include <QFile>
#include <QVBoxLayout>
#include <QPlainTextEdit>
#include <QFont>
#include <QFontDatabase>


//----------------------------------------------------------------------------------------------------------------------

void showTxtDescriptionFor( QWidget * parentWindow, const QString & filePath, const QString & contentType )
{
	QFileInfo dataFileInfo( filePath );

	if (!dataFileInfo.isFile())  // user could click on a directory
	{
		return;
	}

	// get the corresponding file with txt suffix
	QFileInfo descFileInfo( fs::replaceFileSuffix( dataFileInfo.filePath(), "txt" ) );
	if (!descFileInfo.isFile())
	{
		// try TXT in case we are in a case-sensitive file-system such as Linux
		descFileInfo = QFileInfo( fs::replaceFileSuffix( dataFileInfo.filePath(), "TXT" ) );
		if (!descFileInfo.isFile())
		{
			reportUserError( parentWindow, "Cannot open "%contentType,
				capitalize( contentType )%" file \""%descFileInfo.fileName()%"\" does not exist" );
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

	QDialog descDialog( parentWindow );
	descDialog.setObjectName( "FileDescription" );
	descDialog.setWindowTitle( descFileInfo.fileName() );
	descDialog.setWindowModality( Qt::WindowModal );

	QVBoxLayout * layout = new QVBoxLayout( &descDialog );

	QPlainTextEdit * textEdit = new QPlainTextEdit( &descDialog );
	textEdit->setReadOnly( true );
	textEdit->setWordWrapMode( QTextOption::NoWrap );
	QFont font = QFontDatabase::systemFont( QFontDatabase::FixedFont );
	font.setPointSize( 10 );
	textEdit->setFont( font );

	textEdit->setPlainText( desc );

	layout->addWidget( textEdit );

	// estimate the optimal window size
	int dialogWidth  = int( 75.0f * float( font.pointSize() ) * 0.84f ) + 30;
	int dialogHeight = int( 40.0f * float( font.pointSize() ) * 1.62f ) + 30;
	descDialog.resize( dialogWidth, dialogHeight );

	// position it to the right of the center of the parent widget
	QRect parentGeometry = parentWindow->geometry();
	int parentCenterX = parentGeometry.x() + (parentGeometry.width() / 2);
	int parentCenterY = parentGeometry.y() + (parentGeometry.height() / 2);
	QPoint globalPos( parentCenterX, parentCenterY );  //parentWindow->mapToGlobal( QPoint( parentCenterX, parentCenterY ) );
	descDialog.move( globalPos.x(), globalPos.y() - (descDialog.height() / 2) );

	descDialog.exec();
}
