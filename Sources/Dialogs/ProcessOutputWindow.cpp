//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: window that shows a status and output of a started process
//======================================================================================================================

#include "ProcessOutputWindow.hpp"
#include "ui_ProcessOutputWindow.h"

#include "Utils/WidgetUtils.hpp"  // setTextColor
#include "Utils/FileSystemUtils.hpp"  // getFileNameFromPath
#include "Utils/ErrorHandling.hpp"

#include <QTextEdit>
#include <QFontDatabase>
#include <QPushButton>
#include <QStringBuilder>
#include <QColor>


//======================================================================================================================

static const char * const statusStrings [] =
{
	"Not started",
	"Starting",
	"Running",
	"Finished",
	"Exited with error",
	"Failed to start",
	"Crashed",
	"Shutting down",
	"Dying",
	"Terminated",
	"Killed",
	"Error (shutting down)",
	"Error occured"
};
static_assert( size_t(ProcessStatus::UnknownError) + 1 == std::size(statusStrings), "Please update this table" );

const char * toString( ProcessStatus status )
{
	if (size_t(status) < std::size(statusStrings))
		return statusStrings[ size_t(status) ];
	else
		return "<invalid>";
}

static const char * const terminateBtnText = "Terminate";
static const char * const killBtnText = "Kill";


//======================================================================================================================

ProcessOutputWindow::ProcessOutputWindow( QWidget * parent )
:
	QDialog( parent ),
	DialogCommon( this )
{
	logDebug() << "ProcessOutputWindow()";

	ui = new Ui::ProcessOutputWindow;
	ui->setupUi( this );
	abortBtn = ui->buttonBox->button( QDialogButtonBox::StandardButton::Abort );
	closeBtn = ui->buttonBox->button( QDialogButtonBox::StandardButton::Close );

	QFont font = QFontDatabase::systemFont( QFontDatabase::FixedFont );
	font.setPointSize( 10 );
	ui->textEdit->setFont( font );
	ui->textEdit->clear();
	ui->textEdit->setOverwriteMode( true );

	// capture key presses so that we can send them to the process
	keyPressFilter.toggleKeyPressSupression( true );  // stop Enter/Esc key events, otherwise they would close the window
	ui->textEdit->installEventFilter( &keyPressFilter );
	connect( &keyPressFilter, &KeyPressFilter::keyPressed, this, &thisClass::onKeyPressed );

	closeBtn->setText("Close");
	connect( abortBtn, &QPushButton::clicked, this, &thisClass::onAbortClicked );
	connect( closeBtn, &QPushButton::clicked, this, &thisClass::reject );

	// closeEvent() is not called when the dialog is closed, we have to connect this to the finished() signal
	connect( this, &QDialog::finished, this, &thisClass::onDialogClosed );

	setOwnStatus( ProcessStatus::NotStarted );
}

ProcessOutputWindow::~ProcessOutputWindow()
{
	logDebug() << "~ProcessOutputWindow()";

	if (process.state() != QProcess::NotRunning)
	{
		// last resort, window is quiting, we cannot let the process continue
		logInfo() << "    killing process";
		this->ownStatus = ProcessStatus::Dying;
		process.kill();
	}

	delete ui;
	ui = nullptr;
}

void ProcessOutputWindow::setOwnStatus( ProcessStatus status, const QString & detail )
{
	logDebug() << "    setOwnStatus: " << toString( status );

	this->ownStatus = status;

	// set status line text
	QString statusText = toString( status );
	if (!detail.isEmpty())
		statusText += " (" % detail % ")";
	ui->statusLine->setText( statusText );

	// set status line color
	QColor textColor;
	switch (status)
	{
		case ProcessStatus::NotStarted:
		case ProcessStatus::Starting:
			textColor = Qt::white;
			break;
		case ProcessStatus::Running:
		case ProcessStatus::Finished:
			textColor = QColor::fromHsv( 120, 200, 255 );  // lighter green
			break;
		case ProcessStatus::ShuttingDown:
		case ProcessStatus::Dying:
		case ProcessStatus::Terminated:
		case ProcessStatus::Killed:
			textColor = QColor::fromHsv( 50, 255, 255 );   // darker yellow
			break;
		default:  // all kinds of errors
			textColor = QColor::fromHsv( 4, 180, 255 );    // lighter red
			break;
	}
	wdg::setTextColor( ui->statusLine, textColor );

	// toggle buttons
	switch (status)
	{
		case ProcessStatus::NotStarted:
		case ProcessStatus::Finished:
		case ProcessStatus::ExitedWithError:
		case ProcessStatus::FailedToStart:
		case ProcessStatus::Crashed:
		case ProcessStatus::Terminated:
		case ProcessStatus::Killed:
			abortBtn->setText( terminateBtnText );
			abortBtn->setEnabled( false );
			closeBtn->setEnabled( true );
			break;
		case ProcessStatus::Starting:
			abortBtn->setText( killBtnText );
			abortBtn->setEnabled( true );
			closeBtn->setEnabled( false );
			break;
		case ProcessStatus::Running:
			abortBtn->setText( terminateBtnText );
			abortBtn->setEnabled( true );
			closeBtn->setEnabled( false );
			break;
		case ProcessStatus::ShuttingDown:
			abortBtn->setText( killBtnText );
			abortBtn->setEnabled( true );
			closeBtn->setEnabled( false );
			break;
		case ProcessStatus::Dying:
			abortBtn->setText( killBtnText );
			abortBtn->setEnabled( true );
			closeBtn->setEnabled( true );
			break;
		case ProcessStatus::UnknownError:
			abortBtn->setText( terminateBtnText );
			abortBtn->setEnabled( false );
			closeBtn->setEnabled( true );
			break;
		default:
			abortBtn->setText( killBtnText );
			abortBtn->setEnabled( true );
			closeBtn->setEnabled( false );
			break;
	}
}

ProcessStatus ProcessOutputWindow::runProcess(
	const QString & executable, const QStringVec & arguments, const QString & workingDir, const EnvVars & envVars
){
	logDebug() << "runProcess: " << executable;

	executableName = fs::getFileNameFromPath( executable );
	this->setWindowTitle( executableName % " output" );

	process.setProgram( executable );
	process.setArguments( arguments.toList() );
	process.setWorkingDirectory( workingDir );
	process.setProcessChannelMode( QProcess::MergedChannels );  // merge stdout and stderr

	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	for (const auto & envVar : envVars)
	{
		env.insert( envVar.name, envVar.value );
	}
	process.setProcessEnvironment(env);

	connect( &process, &QProcess::started, this, &thisClass::onProcessStarted );
	connect( &process, &QProcess::readyReadStandardOutput, this, &thisClass::readProcessOutput );
	connect( &process, QOverload< int, QProcess::ExitStatus >::of( &QProcess::finished ), this, &thisClass::onProcessFinished );
	connect( &process, &QProcess::errorOccurred, this, &thisClass::onErrorOccurred );

	setOwnStatus( ProcessStatus::Starting );

	// start asynchronously and wait for signals
	process.start();

	// When the error occurs early and the signal is sent from within process.start(),
	// the accept()/reject()/done() call does not initiate closing the dialog because "fuck yea Qt".
	// So we have to manually return here, otherwise the dialog would never quit.
	if (ownStatus != ProcessStatus::Starting && ownStatus != ProcessStatus::Running)
	{
		return ownStatus;
	}

	// start dialog event loop and wait for the process to finish or for the user to close it
	this->exec();

	return ownStatus;
}

void ProcessOutputWindow::onProcessStarted()
{
	logDebug() << "processStarted";

	setOwnStatus( ProcessStatus::Running );
}

void ProcessOutputWindow::readProcessOutput()
{
	QByteArray output = process.readAllStandardOutput();
 #if IS_WINDOWS
	output.replace( "\r\n", "\n" );
 #endif

	// If there are still CRs, the process probably wants to return the cursor to the start of the line to overwrite it.
	// In that case everytime we encounter CR, we need to move the cursor to the beginning of the current line manually.
	const QList< QByteArray > parts = output.split('\r');

	QTextCursor cursor = ui->textEdit->textCursor();

	const QByteArray & beforeCR = parts[0];
	cursor.insertText( QString::fromLatin1( beforeCR ) );

	for (int i = 1; i < parts.count(); ++i)
	{
		const QByteArray & afterCR = parts[i];
		// Of course in fucking Qt the override mode doesn't work so we have to select and delete the old text manually.
		cursor.movePosition( QTextCursor::StartOfLine, QTextCursor::KeepAnchor );
		cursor.removeSelectedText();
		cursor.insertText( QString::fromLatin1( afterCR ) );
	}

	ui->textEdit->setTextCursor( cursor );
}

void ProcessOutputWindow::onKeyPressed( int key, uint8_t modifiers )
{
	// Sometimes the process can print something like "Press 'Q' to quit",
	// so we need to forward key presses to the process to allow the user control.
	if (modifiers == 0 && key > 0 && key <= CHAR_MAX)
	{
		char ch = char( key );
		process.write( &ch, 1 );
	}
}

void ProcessOutputWindow::onProcessFinished( int exitCode, QProcess::ExitStatus exitStatus )
{
	logDebug() << "processFinished: " << exitCode << ", " << exitStatus;

	// This callback can be called even from destructor when destroying QProcess.
	// In that case, don't do anything and abort because our own data are already destroyed.
	if (ui == nullptr)
		return;

	if (ownStatus == ProcessStatus::ShuttingDown)  // user requested to terminate the process and now it finally shut down
	{
		setOwnStatus( ProcessStatus::Terminated );
	}
	else if (ownStatus == ProcessStatus::Dying)    // user requested to kill the process and now it finally died
	{
		setOwnStatus( ProcessStatus::Killed );
	}
	else if (ownStatus == ProcessStatus::UnknownErrorShtDn)  // process was terminated due to unexpected error and has finally shut down
	{
		setOwnStatus( ProcessStatus::UnknownError );
	}

	if (ownStatus == ProcessStatus::Terminated || ownStatus == ProcessStatus::Killed)  // the Terminate/Kill button was clicked
	{
		closeDialog( QDialog::Rejected );
	}
	else if (ownStatus == ProcessStatus::UnknownError)  // process was terminated due to unexpected error
	{
		closeDialog( QDialog::Accepted );
	}
	else if (exitStatus == QProcess::CrashExit)
	{
		setOwnStatus( ProcessStatus::Crashed );
		reportRuntimeError( this, "Program crashed", executableName % " has crashed." );
		closeDialog( QDialog::Accepted );
	}
	else if (exitCode != 0)
	{
		setOwnStatus( ProcessStatus::ExitedWithError, QString::number( exitCode ) );
		//reportRuntimeError( this, "Program exited with error",
		//	executableName % " has exited with error code " % QString::number( exitCode ) % "." );
		//closeDialog( QDialog::Accepted );
	}
	else
	{
		setOwnStatus( ProcessStatus::Finished );
		closeDialog( QDialog::Accepted );
	}
}


void ProcessOutputWindow::onErrorOccurred( QProcess::ProcessError error )
{
	logDebug() << "errorOccurred: " << error;

	// This callback can be called even from destructor when destroying QProcess.
	// In that case, don't do anything and abort because our own data are already destroyed.
	if (ui == nullptr)
		return;

	// When we kill the process, Qt consideres it crashed, so it calls this function.
	// But we don't want to report it as crashed because we essentially made it crash on purpose.
	if (ownStatus == ProcessStatus::Dying)
		return;

	switch (error)
	{
		case QProcess::FailedToStart:
			setOwnStatus( ProcessStatus::FailedToStart );
			reportRuntimeError( this, "Process start error", "Failed to start "%executableName%" ("%process.errorString()%")" );
			closeDialog( QDialog::Accepted );
			break;
		case QProcess::Crashed:
			setOwnStatus( ProcessStatus::Crashed );
			reportRuntimeError( this, "Program crashed", executableName%" has crashed." );
			closeDialog( QDialog::Accepted );
			break;
		case QProcess::Timedout:
			setOwnStatus( ProcessStatus::FailedToStart );
			reportRuntimeError( this, "Process start timeout", executableName%" process has timed out while starting." );
			closeDialog( QDialog::Accepted );
			break;
		case QProcess::ReadError:
			setOwnStatus( ProcessStatus::UnknownError );
			reportRuntimeError( this, "Cannot read process output", "Failed to read output of the process." );
			logDebug() << "    terminating process";
			process.terminate();  // wait for the process to quit, then close dialog
			break;
		case QProcess::WriteError:
			setOwnStatus( ProcessStatus::UnknownError );
			reportRuntimeError( this, "Cannot write to process input", "Failed to write to the process input." );
			logDebug() << "    terminating process";
			process.terminate();  // wait for the process to quit, then close dialog
			break;
		default:
			setOwnStatus( ProcessStatus::UnknownError );
			reportRuntimeError( this, "Unknown error", "Unknown error occured while executing command: "%process.errorString() );
			logDebug() << "    terminating process";
			process.terminate();  // wait for the process to quit, then close dialog
			break;
	}
}

void ProcessOutputWindow::onAbortClicked( bool )
{
	logDebug() << "abortClicked: " << abortBtn->text();

	if (process.state() != QProcess::NotRunning)
	{
		if (abortBtn->text() == terminateBtnText)
		{
			// Attempt to quit the process in a polite way (give it a chance to save data, release resources, ...).
			// This should lead to processFinished() being called soon. If it doesn't, this button will transform
			// into a Kill button, which will then kill the process the hard way.
			setOwnStatus( ProcessStatus::ShuttingDown );
			logDebug() << "    terminating process";
			process.terminate();
		}
		else
		{
			// If the process doesn't listen to terminate signals, we can kill it the hard way.
			setOwnStatus( ProcessStatus::Dying );
			logDebug() << "    killing process";
			process.kill();
		}
	}
	else
	{
		closeDialog( QDialog::Rejected );
	}
}

void ProcessOutputWindow::closeDialog( int resultCode )
{
	logDebug() << "    closeDialog: " << resultCode;

	this->done( resultCode );
}

void ProcessOutputWindow::onDialogClosed( int resultCode )
{
	logDebug() << "dialogClosed: " << resultCode;
}
