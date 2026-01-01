//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: miscellaneous utilities that are needed in multiple places but don't belong anywhere else
//======================================================================================================================

#ifndef MISC_UTILS_INCLUDED
#define MISC_UTILS_INCLUDED


#include "Essential.hpp"

#include <QVariant>
#include <QString>
#include <QList>


//----------------------------------------------------------------------------------------------------------------------

inline auto getType( const QVariant & variant )
{
 #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	return variant.type();
 #else
	return variant.typeId();
 #endif
}

/// Creates a file filter for the QFileSystemModel.
QStringList makeFileSystemModelFilter( const QStringList & suffixes );

/// Creates a file filter for the QFileDialog::getOpenFileNames.
QString makeFileDialogFilter( const char * filterName, const QStringList & suffixes );

struct Argument
{
	QString str;  ///< individual argument trimmed from whitespaces and quotes
	bool wasQuoted;  ///< whether this argument was originally quoted
};
/// Splits a command line string into individual arguments, taking into account quotes.
/** NOTE: This is simplified, it will not handle the full command line syntax, only the basic cases. */
QList< Argument > splitCommandLineArguments( const QString & argsStr );

bool areScreenCoordinatesValid( int x, int y );


//----------------------------------------------------------------------------------------------------------------------


#endif // MISC_UTILS_INCLUDED
