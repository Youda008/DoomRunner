//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: executable file parsing and information extraction
//======================================================================================================================

#ifndef EXE_READER_INCLUDED
#define EXE_READER_INCLUDED


#include "Essential.hpp"
#include "Version.hpp"

#include <QString>


//----------------------------------------------------------------------------------------------------------------------

struct ExeVersionInfo
{
	QString appName;
	QString description;
	Version v;
};

#if IS_WINDOWS
/*
enum class ExeReadStatus
{
	Success,
	CantOpen,
	InvalidFormat,
	ResourceNotFound,
};

struct ExeInfo
{
	ExeReadStatus status = ExeReadStatus::CantOpen;
	QString manifest;
};

ExeInfo readExeInfo( const QString & filePath );
*/

std::optional< ExeVersionInfo > readExeVersionInfo( const QString & filePath );

#endif


#endif // EXE_READER_INCLUDED
