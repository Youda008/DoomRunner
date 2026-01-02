//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: types used by FileInfoCache.hpp, separated for less recompilation
//======================================================================================================================

#ifndef FILE_INFO_CACHE_TYPES_INCLUDED
#define FILE_INFO_CACHE_TYPES_INCLUDED


class QString;

enum class ReadStatus
{
	Success,
	NotSupported,     ///< reading this information is not implemented on this operating system
	CantOpen,         ///< the file could not be opened for reading
	FailedToRead,     ///< the content of the file could not be read
	InvalidFormat,    ///< the file does not have the expected format
	InfoNotPresent,   ///< the requested information is not present in this file

	Uninitialized,    ///< this read status has not been set properly
};
const char * statusToStr( ReadStatus status );
ReadStatus statusFromStr( const QString & statusStr );

template< typename FileInfo >
struct UncertainFileInfo : public FileInfo
{
	ReadStatus status = ReadStatus::Uninitialized;
};


#endif // FILE_INFO_CACHE_TYPES_INCLUDED
