//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: generator of unique IDs for persistent object referencing
//======================================================================================================================

#include "UniqueIdGenerator.hpp"

#include "ErrorHandling.hpp"

#include <QRandomGenerator>


//======================================================================================================================

QString UniqueIdGenerator::generateID()
{
	QString randomID;

	do
	{
		quint32 randomInt = QRandomGenerator::global()->generate();
		randomID = QString("%1").arg( randomInt, 8, 16, QChar('0') ).toUpper();
	}
	// If we generate an ID that already exists, try again until we get a unique one.
	while (_existingIDs.contains( randomID ));

	// Remember already generated IDs to make sure the new ones are unique.
	_existingIDs.insert( randomID, true );

	return randomID;
}

void UniqueIdGenerator::addExistingID( QStringView objectName, const QString & id )
{
	if (_existingIDs.contains( id ))
	{
		logLogicError( u"UniqueIdGenerator" )
			<< "Attempting to add a unique ID that already exists: "<<id<<" ("<<objectName<<")";
		return;
	}

	_existingIDs.insert( id, true );
}
