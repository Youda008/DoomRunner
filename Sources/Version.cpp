//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: information about application version
//======================================================================================================================

#include "Version.hpp"

#include <QString>
#include <QStringList>

struct Version
{
	ushort major;
	ushort minor;
	ushort patch;
};

static bool parseVersion( const QString & versionStr, Version & version )
{
	QStringList substrings = versionStr.split('.');
	if (substrings.size() < 2)
		return false;

	bool success = false;

	version.major = substrings[0].toUShort( &success );
	if (!success)
		return false;

	version.minor = substrings[1].toUShort( &success );
	if (!success)
		return false;

	// this is optional, 0 by default
	version.patch = substrings.size() > 2 ? substrings[2].toUShort() : 0;

	return true;
}

int compareVersions( const QString & verStr1, const QString & verStr2 )
{
	Version ver1, ver2;

	bool parsed1 = parseVersion( verStr1, ver1 );
	bool parsed2 = parseVersion( verStr2, ver2 );

	// any version is bigger than error, 2 errors are equal
	if (!parsed1 && !parsed2)
		return 0;
	else if (parsed1 && !parsed2)
		return 1;
	else if (!parsed1 && parsed2)
		return -1;

	if (ver1.major != ver2.major)
		return ver1.major - ver2.major;
	else if (ver1.minor != ver2.minor)
		return ver1.minor - ver2.minor;
	else
		return ver1.patch - ver2.patch;
}
