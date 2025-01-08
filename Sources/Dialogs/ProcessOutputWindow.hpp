//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: window that shows a status and output of a started process
//======================================================================================================================

#ifndef ENGINE_OUTPUT_WINDOW_INCLUDED
#define ENGINE_OUTPUT_WINDOW_INCLUDED


#include "DialogCommon.hpp"

#include "CommonTypes.hpp"
#include "UserData.hpp"  // EnvVars
#include "Utils/EventFilters.hpp"

#include <QDialog>
#include <QProcess>

class QPushButton;
class QCloseEvent;

namespace Ui {
	class ProcessOutputWindow;
}


//======================================================================================================================
// we have to use our own process state because the Qt one isn't detailed enough

/// All the possible states the process can go through while this dialog is running.
enum class ProcessStatus
{
	NotStarted,        ///< process has not been even started, only true before calling runProcess
	Starting,          ///< OS is loading the process and preparing it to run
	Running,           ///< process is loaded and running
	Finished,          ///< process has successfully finished and exited with exit code 0
	ExitedWithError,   ///< process has exited regularly but returned non-zero exit code
	FailedToStart,     ///< OS has failed to start the proccess (most likely due to wrong executable path or permissions)
	Crashed,           ///< process has crashed
	ShuttingDown,      ///< terminate signal has been sent to the process and we're waiting for it to react
	Dying,             ///< kill signal has been sent to the process and we're waiting for the OS to shut it down
	Terminated,        ///< the process was terminated and has finally shut down
	Killed,            ///< the process was killed and has finally died
	UnknownErrorShtDn, ///< unknown error has occured during the process handling and the process is being terminated
	UnknownError       ///< unknown error has occured during the process handling and the process has been terminated
};

const char * toString( ProcessStatus status );


//======================================================================================================================
/** Dialog that displays process state and its standard output and error output as if it was a terminal. */

class ProcessOutputWindow : public QDialog, private DialogCommon {

	Q_OBJECT

	using thisClass = ProcessOutputWindow;

 public:

	explicit ProcessOutputWindow( QWidget * parent );
	virtual ~ProcessOutputWindow() override;

	/// Starts a process and shows a window displaying its standard output until the process finishes.
	/**
	  * The process is started asynchronously, but this dialog will keep running until it quits and this function
	  * will return when the dialog quits.
	  * Any errors with starting the process are handled by this function.
	  *
	  * \param executable Path to the executable file. Must be either absolute or relative to the current working dir.
	  * \param arguments Program arguments. Any file paths must be either absolute or relative to the workingDir argument.
	  * \param workingDir Working directory for the started process. All file paths given via arguments must be relative to this.
	  *                   If not specified, the current working directory is used.
	  * \param envVars Optional evironment variables to be set for the starting process.
	  * \return In which state the process was when the the dialog was closed.
	  */
	ProcessStatus runProcess(
		const QString & executable, const QStringList & arguments, const QString & workingDir = {}, const EnvVars & envVars = {}
	);

 private slots:

	void onProcessStarted();
	void readProcessOutput();
	void onProcessFinished( int exitCode, QProcess::ExitStatus exitStatus );
	void onErrorOccurred( QProcess::ProcessError error );

	void onKeyPressed( int key, uint8_t modifiers );

	void onAbortClicked( bool checked );

	// closeEvent() is not called when the dialog is closed, we have to connect this to the finished() signal
	void onDialogClosed( int result );

 private: // methods

	void closeDialog( int resultCode );

	void setOwnStatus( ProcessStatus status, const QString & detail = QString() );

 private: // members

	Ui::ProcessOutputWindow * ui;
	QPushButton * abortBtn;  ///< shortcut to the Terminate button in the list of ui->buttonBox
	QPushButton * closeBtn;  ///< shortcut to the Close button in the list of ui->buttonBox

	QProcess process;

	QString executableName;

	ProcessStatus ownStatus;

	KeyPressFilter keyPressFilter;
};


/// Alternative to ProcessOutputWindow::runProcess(). Starts the process, detaches from it, and ignores its output.
bool startDetachedProcess(
	const QString & executable, const QStringList & arguments, const QString & workingDir = {}, const EnvVars & envVars = {}
);


#endif // ENGINE_OUTPUT_WINDOW_INCLUDED
