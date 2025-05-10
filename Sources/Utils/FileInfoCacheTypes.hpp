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
	NotSupported,
	CantOpen,
	FailedToRead,
	InvalidFormat,
	InfoNotPresent,

	Uninitialized,
};
const char * statusToStr( ReadStatus status );
ReadStatus statusFromStr( const QString & statusStr );

template< typename FileInfo >
struct UncertainFileInfo : public FileInfo
{
	ReadStatus status = ReadStatus::Uninitialized;
};


#endif // FILE_INFO_CACHE_TYPES_INCLUDED
