//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: information about application version
//======================================================================================================================

#ifndef VERSION_INCLUDED
#define VERSION_INCLUDED


#include "Essential.hpp"

#include <QtGlobal>  // QT_VERSION_STR

class QString;


//======================================================================================================================

constexpr const char appVersion [] =
	#include "../version.txt"
;

constexpr const char qtVersion [] = QT_VERSION_STR;

struct Version
{
	uint16_t major;
	uint16_t minor;
	uint16_t patch;
	uint16_t build;

	Version() : major(0), minor(0), patch(0), build(0) {}
	Version( uint16_t m, uint16_t n, uint16_t p = 0, uint16_t b = 0 ) : major(m), minor(n), patch(p), build(b) {}
	Version( const char * versionStr );
	Version( const QString & versionStr );

	bool isValid() const { return major != 0; }

	int64_t compare( const Version & other ) const;
	bool operator==( const Version & other ) const  { return compare( other ) == 0; }
	bool operator!=( const Version & other ) const  { return compare( other ) != 0; }
	bool operator< ( const Version & other ) const  { return compare( other ) <  0; }
	bool operator> ( const Version & other ) const  { return compare( other ) >  0; }
	bool operator<=( const Version & other ) const  { return compare( other ) <= 0; }
	bool operator>=( const Version & other ) const  { return compare( other ) >= 0; }
};


#endif // VERSION_INCLUDED
