//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: information about application version
//======================================================================================================================

#include "Version.hpp"

#include <QString>
#include <QStringList>

#include <QDebug>

Version::Version( const char * versionStr ) : Version( QString( versionStr ) ) {}

Version::Version( const QString & versionStr )
{
	major = minor = patch = 0;

	QStringList substrings = versionStr.split('.');
	if (substrings.size() < 2)
		return;

	bool success = false;

	ushort major = substrings[0].toUShort( &success );
	if (!success || major > UINT8_MAX)
		return;

	this->major = uint8_t( major );

	ushort minor = substrings[1].toUShort( &success );
	if (!success || minor > UINT8_MAX)
		return;

	this->minor = uint8_t( minor );

	// this is optional, 0 by default
	if (substrings.size() > 2)
	{
		ushort patch = substrings[2].toUShort( &success );
		if (!success || patch > UINT8_MAX)
			return;

		this->patch = uint8_t( patch );
	}
}

int Version::compare( const Version & other ) const
{
	bool valid1 = this->isValid();
	bool valid2 = other.isValid();

	// any version is bigger than error, 2 errors are equal
	if (!valid1 && !valid2)
		return 0;
	else if (valid1 && !valid2)
		return 1;
	else if (!valid1 && valid2)
		return -1;

	return ((this->major << 16) + (this->minor << 8) + (this->patch))
	     - ((other.major << 16) + (other.minor << 8) + (other.patch));
}
