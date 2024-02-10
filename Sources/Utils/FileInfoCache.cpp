//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: templates and common code for application's internal caches
//======================================================================================================================

#include "FileInfoCache.hpp"

#include "ContainerUtils.hpp"


static const char * const ReadStatusStrings [] =
{
	"Success",
	"NotSupported",
	"CantOpen",
	"FailedToRead",
	"InvalidFormat",
	"InfoNotPresent",
};
static_assert( std::size(ReadStatusStrings) == size_t(ReadStatus::Uninitialized), "Please update this table too" );

const char * statusToStr( ReadStatus status )
{
	if (size_t(status) < std::size(ReadStatusStrings))
		return ReadStatusStrings[ size_t(status) ];
	else
		return "<invalid>";
}

ReadStatus statusFromStr( const QString & statusStr )
{
	int idx = find( ReadStatusStrings, statusStr );
	if (idx >= 0)
		return ReadStatus( idx );
	else
		return ReadStatus::Uninitialized;
}
