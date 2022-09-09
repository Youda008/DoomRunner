//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: window that shows a piped standard output from a started processs
//======================================================================================================================

#include "ProcessOutputWindow.hpp"
#include "ui_ProcessOutputWindow.h"

#include "FileSystemUtils.hpp"

#include <QTextEdit>
#include <QFontDatabase>
#include <QPushButton>
#include <QMessageBox>
#include <QStringBuilder>

#include <QDebug>


//======================================================================================================================

ProcessOutputWindow::ProcessOutputWindow( QWidget * parent )
:
	QDialog( parent ),
	windowIsClosing( false ),
	errorOccured( false ),
	terminated( false )
{
	ui = new Ui::ProcessOutputWindow;
	ui->setupUi( this );
	terminateBtn = ui->buttonBox->button( QDialogButtonBox::StandardButton::Abort );

	QFont font = QFontDatabase::systemFont( QFontDatabase::FixedFont );
	font.setPointSize( 10 );
	ui->textEdit->setFont( font );
	ui->textEdit->clear();

	terminateBtn->setText("Terminate");
	connect( terminateBtn, &QPushButton::clicked, this, &thisClass::terminateClicked );

	connect( this, &QDialog::finished, this, &thisClass::onDialogClose );
}

ProcessOutputWindow::~ProcessOutputWindow()
{
	delete ui;
}

static void setArguments( QProcess & process, const QStringList & arguments )
{
	// Retarded Windows implementation of Qt surrounds all arguments with additional quotes, which is unwanted
	// because we already have them quoted, but it can't be turned off. So we must work around this
	// by setting the command line manually.
 #ifdef _WIN32
	process.setNativeArguments( arguments.join(' ') );
 #else
	process.setArguments( arguments );
 #endif
}

void ProcessOutputWindow::runProcess( const QString & executable, const QStringList & arguments )
{
	executableName = getFileNameFromPath( executable );

	this->setWindowTitle( executableName % " " % this->windowTitle() );

	process.setProgram( executable );
	setArguments( process, arguments );
	process.setProcessChannelMode( QProcess::MergedChannels );  // merge stdout and stderr

	connect( &process, &QProcess::readyReadStandardOutput, this, &thisClass::readProcessOutput );
	connect( &process, QOverload< int, QProcess::ExitStatus >::of( &QProcess::finished ), this, &thisClass::onProcessFinished );
	connect( &process, &QProcess::errorOccurred, this, &thisClass::onErrorOccurred );

	// start asynchronously and wait for signals
	process.start();

	// When the error occurs early and the signal is sent from within process.start(), the accept() call does not
	// initiate closing the dialog because "fuck yea Qt".
	// So we have to manually return here, otherwise the dialog would never quit.
	if (errorOccured)
	{
		return;
	}

	this->exec();
}

void ProcessOutputWindow::readProcessOutput()
{
	QByteArray output = process.readAllStandardOutput();
	QString outputStr( output );
	outputStr.remove('\r');
	// ffs Qt!!! ui->textEdit->append() appends the text with additional newline and it cannot be prevented
	ui->textEdit->moveCursor( QTextCursor::End );
	ui->textEdit->insertPlainText( outputStr );
}

void ProcessOutputWindow::onErrorOccurred( QProcess::ProcessError error )
{
	errorOccured = true;

	// Qt calls this even after the closeEvent when the QProcess is destructed, which is undesirable.
	if (windowIsClosing)
		return;

	switch (error)
	{
		case QProcess::FailedToStart:
			QMessageBox::warning( this, "Process start error", "Failed to start " % executableName % "." );
			closeDialog( ProcessStatus::FailedToStart );
			break;
		case QProcess::Crashed:
			QMessageBox::warning( this, "Program crashed", executableName % " has crashed." );
			closeDialog( ProcessStatus::Crashed );
			break;
		case QProcess::Timedout:
			QMessageBox::warning( this, "Process start timeout", executableName % " process has timed out while starting." );
			closeDialog( ProcessStatus::FailedToStart );
			break;
		case QProcess::ReadError:
			QMessageBox::warning( this, "Cannot read process output", "Failed to read output of the process." );
			process.terminate();  // wait for the process to quit, then close dialog
			break;
		case QProcess::WriteError:
			QMessageBox::warning( this, "Cannot write to process input", "Failed to write to the process input." );
			process.terminate();  // wait for the process to quit, then close dialog
			break;
		default:
			QMessageBox::warning( this, "Unknown error", "Unknown error occured while executing command." );
			process.terminate();  // wait for the process to quit, then close dialog
			break;
	}
}

void ProcessOutputWindow::onProcessFinished( int exitCode, QProcess::ExitStatus exitStatus )
{
	// Qt calls this even after the closeEvent when the QProcess is destructed, which is undesirable.
	if (windowIsClosing)
		return;

	if (terminated)  // process was terminated by user via Terminate button
	{
		closeDialog( ProcessStatus::Terminated );
	}
	if (errorOccured)  // process was terminated due to unexpected error
	{
		closeDialog( ProcessStatus::UnknownError );
	}
	else if (exitStatus == QProcess::CrashExit)
	{
		QMessageBox::warning( this, "Program crashed", executableName % " has crashed." );
		closeDialog( ProcessStatus::Crashed );
	}
	else if (exitCode != 0)
	{
		QMessageBox::warning( this, "Program exited with error",
			executableName % " has exited with error code " % QString::number( exitCode ) % "." );
		closeDialog( ProcessStatus::ExitedWithError );
	}

	closeDialog( ProcessStatus::Success );
}

void ProcessOutputWindow::terminateClicked( bool )
{
	if (process.state() != QProcess::NotRunning)
	{
		// Attempt to quit the process in a polite way (give it a chance to save data, release resources, ...).
		// This should lead to processFinished() being called soon. If it doesn't and user gets impatient,
		// he can still click the X button in the corner, which will kill the process the hard way.
		terminated = true;
		process.terminate();
	}
	else
	{
		closeDialog( ProcessStatus::Terminated );
	}
}

void ProcessOutputWindow::closeDialog( int resultCode )
{
	this->done( resultCode );
}

void ProcessOutputWindow::onDialogClose( int )
{
	windowIsClosing = true;

	if (process.state() != QProcess::NotRunning)
	{
		// last resort, window is quiting, we cannot let the process continue
		process.kill();
	}
}
