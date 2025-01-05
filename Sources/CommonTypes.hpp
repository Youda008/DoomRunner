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

class QTextStream;


class QStringVec : public QVector< QString >
{
 public:
	using QVector<QString>::QVector;
	//QStringVec( const QVector< QString > & other ) : QVector( other ) {}
	//QStringVec( QVector< QString > & other ) : QVector( std::move(other) ) {}
	QString join( QChar delimiter ) const;
};

// there is no overload for QVector && in Qt
template< typename Elem >
QVector< Elem > & operator<<( QVector< Elem > & destVec, QVector< Elem > && vecToAppend )
{
	for (auto & elem : vecToAppend)
	{
		destVec.append( std::move(elem) );
	}
	return destVec;
}

QTextStream & operator<<( QTextStream & stream, const QStringVec & vec );


#endif // COMMON_TYPES_INCLUDED
