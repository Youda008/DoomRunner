//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: commonly used types and type aliases
//======================================================================================================================

#ifndef COMMON_TYPES_INCLUDED
#define COMMON_TYPES_INCLUDED


#include <QString>
#include <QVector>
#include <QList>


// to be used when we want to pass empty string, but a reference or pointer is required
extern const QString emptyString;


class QStringVec : public QVector< QString >
{
 public:
	using QVector::QVector;
	QString join( QChar delimiter ) const;
};


#endif // COMMON_TYPES_INCLUDED
