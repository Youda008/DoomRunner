//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: commonly used types and type aliases
//======================================================================================================================

#ifndef COMMON_TYPES_INCLUDED
#define COMMON_TYPES_INCLUDED


#include <QList>  // decltype( size() )

// While Qt5 uses int where the std library would use size_t (size(), resize(), reserve(), operator[], ...),
// the Qt6 uses qsizetype which is a signed size_t.
// Commiting fully to one or another in our code causes compilation warnings about implicit integer conversion,
// so the best solution is to declare our own type alias that represents the correct type for the current Qt version.
using qsize_t = decltype( std::declval< QList<int> >().size() );


#endif // COMMON_TYPES_INCLUDED
