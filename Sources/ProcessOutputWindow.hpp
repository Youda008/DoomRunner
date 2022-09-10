//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: window that shows a piped standard output from a started process
//======================================================================================================================

#ifndef ENGINE_OUTPUT_WINDOW_INCLUDED
#define ENGINE_OUTPUT_WINDOW_INCLUDED


#include <QDialog>
#include <QProcess>

class QPushButton;
class QCloseEvent;

namespace Ui {
	class ProcessOutputWindow;
}


//======================================================================================================================

/// Possible results of the attempt to run a process after the dialog exits.
enum class ProcessStatus
{
	NotStarted,
	Starting,
	Running,
	FinishedSuccessfully,
	ExitedWithError,
	FailedToStart,
	Crashed,
	Quitting,
	Terminated,
	UnknownError,
};

const char * toString( ProcessStatus status );


//======================================================================================================================

class ProcessOutputWindow : public QDialog {

	Q_OBJECT

	using thisClass = ProcessOutputWindow;

 public:

	explicit ProcessOutputWindow( QWidget * parent );
	virtual ~ProcessOutputWindow() override;

	/// Starts a process and shows a window displaying its console output until the process finishes.
	/** The process is started asynchronously, but this dialog will keep running until it quits and this function
	  * will return when the dialog quits. Any errors with starting the process are handled by this function. */
	ProcessStatus runProcess( const QString & executable, const QStringList & arguments );

 private slots:

	void processStarted();
	void readProcessOutput();
	void processFinished( int exitCode, QProcess::ExitStatus exitStatus );
	void errorOccurred( QProcess::ProcessError error );

	void terminateClicked( bool checked );

	// closeEvent() is not called when the dialog is closed, we have to connect this to the finished() signal
	void dialogClosed( int result );

 private: // methods

	void closeDialog( int resultCode );

	void setStatus( ProcessStatus status, const QString & detail = "" );

 private: // members

	Ui::ProcessOutputWindow * ui;
	QPushButton * terminateBtn;  ///< shortcut to the Terminate button in the list of ui->buttonBox
	QPushButton * closeBtn;      ///< shortcut to the Close button in the list of ui->buttonBox

	QProcess process;

	QString executableName;

	ProcessStatus status;
	bool windowIsClosing;

};


#endif // ENGINE_OUTPUT_WINDOW_INCLUDED
