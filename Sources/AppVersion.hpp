#ifndef APP_VERSION_INCLUDED
#define APP_VERSION_INCLUDED


#include <QtGlobal>  // QT_VERSION_STR


constexpr const char appVersion [] =
	#include "../version.txt"
;

constexpr const char qtVersion [] = QT_VERSION_STR;


#endif // APP_VERSION_INCLUDED
