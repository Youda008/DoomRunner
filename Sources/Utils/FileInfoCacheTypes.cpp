//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: types used by FileInfoCache.hpp, separated for less recompilation
//======================================================================================================================

#include "FileInfoCacheTypes.hpp"

#include "ContainerUtils.hpp"  // find

#include <QString>


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
