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


class QStringVec : public QVector< QString >
{
 public:
	using QVector::QVector;
	QString join( QChar delimiter ) const;
};


#endif // COMMON_TYPES_INCLUDED
