//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: miscellaneous utilities that are needed in multiple places but don't belong anywhere else
//======================================================================================================================

#include "MiscUtils.hpp"

#include <QFileInfo>
#include <QMessageBox>


//======================================================================================================================

bool checkPath_MsgBox( const QString & path, const QString & errorMessage )
{
	if (!QFileInfo::exists( path ))
	{
		QMessageBox::warning( nullptr, "File or directory no longer exists", errorMessage.arg( path ) );
		return false;
	}
	return true;
}

void checkPath_exception( bool doVerify, const QString & path, const QString & errorMessage )
{
	if (doVerify)
		if (checkPath_MsgBox( path, errorMessage ))
			throw FileOrDirNotFound{ path };
}
