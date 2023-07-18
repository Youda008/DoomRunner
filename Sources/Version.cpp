//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: information about application version
//======================================================================================================================

#include "Version.hpp"

#include <QString>
#include <QRegularExpression>


Version::Version( const char * versionStr ) : Version( QString( versionStr ) ) {}

Version::Version( const QString & versionStr )
{
	major = minor = patch = build = 0;

	static const QRegularExpression versionRegex("^(\\d+).(\\d+)(?:.(\\d+))?(?:.(\\d+))?$");
	auto match = versionRegex.match( versionStr );

	if (!match.hasMatch())
		return;

	major = match.captured(1).toUShort();
	minor = match.captured(2).toUShort();
	if (match.lastCapturedIndex() >= 3)
		patch = match.captured(3).toUShort();  // optional, will stay 0 if not present
	if (match.lastCapturedIndex() >= 4)
		build = match.captured(4).toUShort();  // optional, will stay 0 if not present
}

int64_t Version::compare( const Version & other ) const
{
	// The following will produce intuitive result even for invalid (all-zero) versions,
	// that is: Any version is bigger than error, 2 errors are equal.
	// The highest bit must not be used but there will never be such high version number.
	using u64 = uint64_t; using i64 = int64_t;
	return
	  i64(((u64(this->major) << 48) + (u64(this->minor) << 32) + (u64(this->patch) << 16) + u64(this->build)))
	- i64(((u64(other.major) << 48) + (u64(other.minor) << 32) + (u64(other.patch) << 16) + u64(other.build)));
}
