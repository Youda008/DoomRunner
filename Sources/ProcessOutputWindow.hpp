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

class ProcessOutputWindow : public QDialog {

	Q_OBJECT

	using thisClass = ProcessOutputWindow;

 public:

	explicit ProcessOutputWindow( QWidget * parent );
	virtual ~ProcessOutputWindow() override;

	/// Starts a process and shows a window displaying its console output until the process finishes.
	/** The process is started asynchronously, but this dialog will keep running until it quits and this function
	  * will return when the dialog quits. Any errors with starting the process are handled by this function. */
	void runProcess( const QString & executable, const QStringList & arguments );

	static constexpr int TODO = 0;

 private slots:

	void readProcessOutput();
	void onErrorOccurred( QProcess::ProcessError error );
	void onProcessFinished( int exitCode, QProcess::ExitStatus exitStatus );
	void onDialogClose( int result );

	void terminateClicked( bool checked );

 private: // methods

	void closeDialog( int resultCode );

 private: // members

	Ui::ProcessOutputWindow * ui;
	QPushButton * terminateBtn;  ///< shortcut to the list of ui->buttonBox

	QProcess process;

	QString executableName;

	bool windowIsClosing;
	bool errorOccured;
	bool terminated;

};

/// Possible return values of the QDialog::result() after the dialog finishes.
struct ProcessStatus
{
	static constexpr int Success = 0;
	static constexpr int FailedToStart = 0;
	static constexpr int Crashed = 0;
	static constexpr int ExitedWithError = 0;
	static constexpr int Terminated = 0;
	static constexpr int UnknownError = 0;
};


#endif // ENGINE_OUTPUT_WINDOW_INCLUDED
