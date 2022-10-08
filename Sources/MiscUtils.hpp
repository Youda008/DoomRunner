//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: miscellaneous utilities that are needed in multiple places but don't belong anywhere else
//======================================================================================================================

#ifndef MISC_UTILS_INCLUDED
#define MISC_UTILS_INCLUDED


#include "Common.hpp"

#include <QString>


//======================================================================================================================
//  path verification

bool checkPath_MsgBox( const QString & path, const QString & errorMessage );

class FileOrDirNotFound
{
	public: QString path;
};
void checkPath_exception( bool doVerify, const QString & path, const QString & errorMessage );


#endif // MISC_UTILS_INCLUDED
