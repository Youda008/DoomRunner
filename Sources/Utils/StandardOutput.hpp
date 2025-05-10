//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: Qt stream wrappers around stdout and stderr
//======================================================================================================================

#ifndef STANDARD_OUTPUT_INCLUDED
#define STANDARD_OUTPUT_INCLUDED


#include <QTextStream>


extern QTextStream stdoutStream;
extern QTextStream stderrStream;

/// Must be called at the beginning of main, before the streams above are used
void initStdStreams();


#endif // STANDARD_OUTPUT_INCLUDED
