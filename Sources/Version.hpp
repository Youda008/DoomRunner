//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: information about application version
//======================================================================================================================

#ifndef VERSION_INCLUDED
#define VERSION_INCLUDED


#include "Essential.hpp"

#include <QString>
#include <QtGlobal>


//======================================================================================================================

constexpr const char appVersion [] =
	#include "../version.txt"
;

constexpr const char qtVersion [] = QT_VERSION_STR;

struct Version
{
	uint8_t major;
	uint8_t minor;
	uint8_t patch;

	//Version() : major(0) {}
	Version( const char * versionStr );
	Version( const QString & versionStr );

	bool isValid() const { return major != 0; }

	int compare( const Version & other ) const;
	bool operator==( const Version & other ) const  { return compare( other ) == 0; }
	bool operator!=( const Version & other ) const  { return compare( other ) != 0; }
	bool operator< ( const Version & other ) const  { return compare( other ) <  0; }
	bool operator> ( const Version & other ) const  { return compare( other ) >  0; }
	bool operator<=( const Version & other ) const  { return compare( other ) <= 0; }
	bool operator>=( const Version & other ) const  { return compare( other ) >= 0; }
};


#endif // VERSION_INCLUDED
