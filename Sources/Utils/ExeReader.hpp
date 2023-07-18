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


//======================================================================================================================

namespace os {


/// Executable version information
struct ExeVersionInfo
{
	QString appName;
	QString description;
	Version version;
};

using optExeVersionInfo = std::optional< ExeVersionInfo >;

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

/// Reads executable version info from the file's built-in resource.
/** If the version info resource is not present in the executable, returns nullopt.
  * If some field is not present in the version info resource, valid ExeVersionInfo is returned
  * but that field stays empty or invalid. */
optExeVersionInfo readExeVersionInfo( const QString & filePath );

#endif


} // namespace os


#endif // EXE_READER_INCLUDED
