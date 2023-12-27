//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: Qt stream wrappers around stdout and stderr
//======================================================================================================================

#include "StandardOutput.hpp"

#include <QFile>

#if IS_WINDOWS
	#include <windows.h>
#endif


//----------------------------------------------------------------------------------------------------------------------

QTextStream stdoutStream;//( stdout ); // undefined behavior due to non-deterministic order of static initializations
QTextStream stderrStream;//( stderr );

static QFile stdoutFile;
static QFile stderrFile;

void initStdStreams()
{
 #if IS_WINDOWS
	// On Windows, graphical applications have their standard output streams closed, even when started from a command line.
	// We need to manually re-attach it to the console of the parent process (cmd.exe) in order to display our output.
	if (AttachConsole( ATTACH_PARENT_PROCESS ))
	{
		freopen( "CONOUT$", "w", stdout );
		freopen( "CONOUT$", "w", stderr );
	}
 #endif

	if (stdoutFile.open( stdout, QIODevice::WriteOnly, QFile::DontCloseHandle ))
		stdoutStream.setDevice( &stdoutFile );
	if (stderrFile.open( stderr, QIODevice::WriteOnly, QFile::DontCloseHandle ))
		stderrStream.setDevice( &stderrFile );
}
