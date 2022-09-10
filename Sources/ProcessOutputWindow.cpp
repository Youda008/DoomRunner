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
#include <QColor>

#include <QDebug>


//======================================================================================================================

static const char * const statusNames [] =
{
	"Not started",
	"Starting",
	"Running",
	"Finished successfully",
	"Exited with error",
	"Failed to start",
	"Crashed",
	"Quitting",
	"Terminated",
	"Unknown error",
};
static_assert( size_t(ProcessStatus::UnknownError) + 1 == std::size(statusNames), "Please update this table" );

const char * toString( ProcessStatus status )
{
	if (size_t(status) < std::size(statusNames))
		return statusNames[ size_t(status) ];
	else
		return "<invalid>";
}


//======================================================================================================================

ProcessOutputWindow::ProcessOutputWindow( QWidget * parent )
:
	QDialog( parent ),
	windowIsClosing( false )
{
	ui = new Ui::ProcessOutputWindow;
	ui->setupUi( this );
	terminateBtn = ui->buttonBox->button( QDialogButtonBox::StandardButton::Abort );
	closeBtn = ui->buttonBox->button( QDialogButtonBox::StandardButton::Close );

	QFont font = QFontDatabase::systemFont( QFontDatabase::FixedFont );
	font.setPointSize( 10 );
	ui->textEdit->setFont( font );
	ui->textEdit->clear();

	terminateBtn->setText("Terminate");
	closeBtn->setText("Close");
	connect( terminateBtn, &QPushButton::clicked, this, &thisClass::terminateClicked );
	connect( closeBtn, &QPushButton::clicked, this, &thisClass::reject );

	// closeEvent() is not called when the dialog is closed, we have to connect this to the finished() signal
	connect( this, &QDialog::finished, this, &thisClass::dialogClosed );

	setStatus( ProcessStatus::NotStarted );
}

ProcessOutputWindow::~ProcessOutputWindow()
{
	delete ui;
}

void ProcessOutputWindow::setStatus( ProcessStatus status, const QString & detail )
{
	this->status = status;

	// set status line text
	QString statusText = toString( status );
	if (!detail.isEmpty())
		statusText += " (" % detail % ")";
	ui->statusLine->setText( statusText );

	// set status line color
	QColor textColor;
	switch (status)
	{
		case ProcessStatus::Running:
		case ProcessStatus::FinishedSuccessfully:
			textColor = QColor::fromHsv( 120, 200, 255 );  // lighter green
			break;
		case ProcessStatus::Quitting:
		case ProcessStatus::Terminated:
			textColor = QColor::fromHsv( 50, 255, 255 );   // darker yellow
			break;
		case ProcessStatus::ExitedWithError:
		case ProcessStatus::FailedToStart:
		case ProcessStatus::Crashed:
			textColor = QColor::fromHsv( 4, 180, 255 );    // lighter red
			break;
		default:
			textColor = Qt::white;
			break;
	}
	QPalette palette = ui->statusLine->palette();
	palette.setColor( QPalette::Text, textColor );
	ui->statusLine->setPalette( palette );

	// toggle buttons
	switch (status)
	{
		case ProcessStatus::NotStarted:
		case ProcessStatus::FinishedSuccessfully:
		case ProcessStatus::ExitedWithError:
		case ProcessStatus::FailedToStart:
		case ProcessStatus::Crashed:
		case ProcessStatus::Terminated:
			terminateBtn->setEnabled( false );
			closeBtn->setEnabled( true );
			break;
		case ProcessStatus::Starting:
			terminateBtn->setEnabled( false );
			closeBtn->setEnabled( true );
			break;
		case ProcessStatus::Running:
			terminateBtn->setEnabled( true );
			closeBtn->setEnabled( false );
			break;
		case ProcessStatus::Quitting:
			terminateBtn->setEnabled( true );
			closeBtn->setEnabled( true );
			break;
		default:
			terminateBtn->setEnabled( false );
			closeBtn->setEnabled( true );
			break;
	}
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

ProcessStatus ProcessOutputWindow::runProcess( const QString & executable, const QStringList & arguments )
{
	qDebug() << "runProcess:" << executable;

	executableName = getFileNameFromPath( executable );
	this->setWindowTitle( executableName % " output" );

	process.setProgram( executable );
	setArguments( process, arguments );
	process.setProcessChannelMode( QProcess::MergedChannels );  // merge stdout and stderr

	connect( &process, &QProcess::started, this, &thisClass::processStarted );
	connect( &process, &QProcess::readyReadStandardOutput, this, &thisClass::readProcessOutput );
	connect( &process, QOverload< int, QProcess::ExitStatus >::of( &QProcess::finished ), this, &thisClass::processFinished );
	connect( &process, &QProcess::errorOccurred, this, &thisClass::errorOccurred );

	setStatus( ProcessStatus::Starting );

	// start asynchronously and wait for signals
	process.start();

	// When the error occurs early and the signal is sent from within process.start(),
	// the accept()/reject()/done() call does not initiate closing the dialog because "fuck yea Qt".
	// So we have to manually return here, otherwise the dialog would never quit.
	if (status != ProcessStatus::Starting && status != ProcessStatus::Running)
	{
		return status;
	}

	// start dialog event loop and wait for the process to finish or for the user to close it
	this->exec();

	return status;
}

void ProcessOutputWindow::processStarted()
{
	qDebug() << "processStarted";

	setStatus( ProcessStatus::Running );
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

void ProcessOutputWindow::processFinished( int exitCode, QProcess::ExitStatus exitStatus )
{
	qDebug() << "processFinished:" << exitCode << "," << exitStatus;

	// Qt calls this even after onDialogClose() when the QProcess is killed, which is undesirable.
	if (windowIsClosing)
		return;

	if (status == ProcessStatus::Quitting)  // user requested to terminate and now it happened
	{
		setStatus( ProcessStatus::Terminated );
	}

	if (status == ProcessStatus::Terminated)  // process was terminated by user via the Terminate button
	{
		closeDialog( QDialog::Rejected );
	}
	else if (status == ProcessStatus::UnknownError)  // process was terminated due to unexpected error
	{
		closeDialog( QDialog::Accepted );
	}
	else if (exitStatus == QProcess::CrashExit)
	{
		setStatus( ProcessStatus::Crashed );
		QMessageBox::warning( this, "Program crashed", executableName % " has crashed." );
		closeDialog( QDialog::Accepted );
	}
	else if (exitCode != 0)
	{
		setStatus( ProcessStatus::ExitedWithError, QString::number( exitCode ) );
		//QMessageBox::warning( this, "Program exited with error",
		//	executableName % " has exited with error code " % QString::number( exitCode ) % "." );
		//closeDialog( QDialog::Accepted );
	}
	else
	{
		closeDialog( QDialog::Accepted );
	}
}


void ProcessOutputWindow::errorOccurred( QProcess::ProcessError error )
{
	qDebug() << "errorOccurred:" << error;

	// Qt calls this even after onDialogClose() when the QProcess is killed, which is undesirable.
	if (windowIsClosing)
		return;

	switch (error)
	{
		case QProcess::FailedToStart:
			setStatus( ProcessStatus::FailedToStart );
			QMessageBox::warning( this, "Process start error", "Failed to start " % executableName % "." );
			closeDialog( QDialog::Accepted );
			return;
		case QProcess::Crashed:
			setStatus( ProcessStatus::Crashed );
			QMessageBox::warning( this, "Program crashed", executableName % " has crashed." );
			closeDialog( QDialog::Accepted );
			return;
		case QProcess::Timedout:
			setStatus( ProcessStatus::FailedToStart );
			QMessageBox::warning( this, "Process start timeout", executableName % " process has timed out while starting." );
			closeDialog( QDialog::Accepted );
			return;
		case QProcess::ReadError:
			setStatus( ProcessStatus::UnknownError );
			QMessageBox::warning( this, "Cannot read process output", "Failed to read output of the process." );
			qDebug() << "    terminating process";
			process.terminate();  // wait for the process to quit, then close dialog
			break;
		case QProcess::WriteError:
			setStatus( ProcessStatus::UnknownError );
			QMessageBox::warning( this, "Cannot write to process input", "Failed to write to the process input." );
			qDebug() << "    terminating process";
			process.terminate();  // wait for the process to quit, then close dialog
			break;
		default:
			setStatus( ProcessStatus::UnknownError );
			QMessageBox::warning( this, "Unknown error", "Unknown error occured while executing command." );
			qDebug() << "    terminating process";
			process.terminate();  // wait for the process to quit, then close dialog
			break;
	}
}

void ProcessOutputWindow::terminateClicked( bool )
{
	qDebug() << "terminateClicked";

	if (process.state() != QProcess::NotRunning)
	{
		// Attempt to quit the process in a polite way (give it a chance to save data, release resources, ...).
		// This should lead to processFinished() being called soon. If it doesn't and user gets impatient,
		// he can still click the X button in the corner, which will kill the process the hard way.
		setStatus( ProcessStatus::Quitting );
		qDebug() << "    terminating process";
		process.terminate();
	}
	else
	{
		closeDialog( QDialog::Rejected );
	}
}

void ProcessOutputWindow::closeDialog( int resultCode )
{
	qDebug() << "closeDialog:" << resultCode;

	this->done( resultCode );
}

void ProcessOutputWindow::dialogClosed( int resultCode )
{
	qDebug() << "dialogClosed:" << resultCode;

	windowIsClosing = true;

	if (process.state() != QProcess::NotRunning)
	{
		// last resort, window is quiting, we cannot let the process continue
		setStatus( ProcessStatus::Quitting );
		qDebug() << "    killing process";
		process.kill();
	}
}
