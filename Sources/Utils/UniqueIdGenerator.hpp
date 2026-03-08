//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: generator of unique IDs for persistent object referencing
//======================================================================================================================

#ifndef UNIQUE_ID_GENERATOR_INCLUDED
#define UNIQUE_ID_GENERATOR_INCLUDED


#include "Essential.hpp"

#include <QString>
#include <QHash>


//======================================================================================================================

class UniqueIdGenerator
{
 public:

	UniqueIdGenerator() = default;
	UniqueIdGenerator( const UniqueIdGenerator & ) = delete;

	/// Generates a new unique ID.
	/** Currently these ID are random, but that's a implementation detail that shouldn't be relied on. */
	QString generateID();

	/// Stores an ID generated earlier to prevent generating it again.
	/** \param objectName name of the object that holds this ID, for logging purposes only */
	void addExistingID( QStringView objectName, const QString & id );

 private:

	QHash< QString, bool > _existingIDs;
};


#endif // UNIQUE_ID_GENERATOR_INCLUDED
