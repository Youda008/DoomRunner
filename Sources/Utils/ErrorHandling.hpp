//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: common error-handling routines
//======================================================================================================================

#ifndef ERROR_HANDLING_INCLUDED
#define ERROR_HANDLING_INCLUDED


#include "Essential.hpp"

#include <QString>

class QWidget;


void reportBugToUser( QWidget * parent, QString title, QString message );


#endif // ERROR_HANDLING_INCLUDED
