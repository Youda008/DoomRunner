//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: miscellaneous utilities that are needed in multiple places but don't belong anywhere else
//======================================================================================================================

#include "MiscUtils.hpp"

#include <QFileInfo>
#include <QTextStream>
#include <QMessageBox>


//----------------------------------------------------------------------------------------------------------------------
//  path verification

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


//----------------------------------------------------------------------------------------------------------------------
//  other

QString makeFileFilter( const char * filterName, const QVector< QString > & suffixes )
{
	QString filter;
	QTextStream filterStream( &filter, QIODevice::WriteOnly );

	filterStream << filterName << " (";
	for (const QString & suffix : suffixes)
		if (&suffix == &suffixes[0])
			filterStream <<  "*." << suffix << " *." << suffix.toUpper();
		else
			filterStream << " *." << suffix << " *." << suffix.toUpper();
	filterStream << ");;";

	filterStream.flush();
	return filter;
}
