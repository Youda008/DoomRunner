//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: mediators between a list of arbitrary objects and list view or other widgets
//======================================================================================================================

#include "ListModel.hpp"


void ListModelCommon::contentChanged( int changedRowsBegin, int changedRowsEnd )
{
	if (changedRowsEnd < 0)
		changedRowsEnd = this->rowCount();

	const QModelIndex firstChangedIndex = createIndex( changedRowsBegin, /*column*/0 );
	const QModelIndex lastChangedIndex = createIndex( changedRowsEnd - 1, /*column*/0 );

	emit dataChanged( firstChangedIndex, lastChangedIndex, {
		Qt::DisplayRole, Qt::EditRole, Qt::CheckStateRole,
		Qt::ForegroundRole, Qt::BackgroundRole, Qt::TextAlignmentRole
	});
}
