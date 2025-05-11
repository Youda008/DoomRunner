//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: mediators between a list of arbitrary objects and list view or other widgets
//======================================================================================================================

#include "GenericListModel.hpp"

#include "Utils/ErrorHandling.hpp"


//======================================================================================================================

// Prevents generating VTable in every translation where these classes are included and used.
AListModel::~AListModel() = default;

// pre-defined commonly used lists of data roles
const QVector<int> AListModel::onlyDisplayRole = { Qt::DisplayRole };
const QVector<int> AListModel::onlyEditRole = { Qt::EditRole };
const QVector<int> AListModel::onlyCheckStateRole = { Qt::CheckStateRole };
const QVector<int> AListModel::allDataRoles = {
	Qt::DisplayRole, Qt::EditRole, Qt::CheckStateRole, Qt::ForegroundRole, Qt::BackgroundRole, Qt::TextAlignmentRole
};

void AListModel::finishEditingItemData( int row, int count, const QVector<int> & roles )
{
	if (count == 0)
		return;
	if (count < 0)
		count = this->rowCount();

	const QModelIndex firstChangedIndex = createIndex( row, /*column*/0 );
	const QModelIndex lastChangedIndex = createIndex( row + count - 1, /*column*/0 );

	emit QAbstractListModel::dataChanged( firstChangedIndex, lastChangedIndex, roles );
}

void AListModel::setEnabledExportFormats( ExportFormats formats )
{
	if (accessStyle() == AccessStyle::ReadOnly && isAnyOfFlagsSet( formats, ExportFormat::Indexes ))
	{
		logLogicError() << "Attempted to enable item export formats not allowed in read-only models: " << Qt::hex << formats;
		unsetFlags( formats, ExportFormat::Indexes );
	}
	if (withoutFlags( formats, ExportFormat::All ) != 0)  // there are unknown format flags
	{
		logLogicError() << "Attempted to enable unknown item export formats: " << Qt::hex << formats;
		unsetFlags( formats, withoutFlags( formats, ExportFormat::All ) );
	}

	enabledExportFormats = formats;
}

void AListModel::setEnabledImportFormats( ExportFormats formats )
{
	if (formats != ExportFormat::None && accessStyle() == AccessStyle::ReadOnly)
	{
		logLogicError() << "Attempted to enable item importing in read-only models: " << Qt::hex << formats;
		return;
	}
	if (withoutFlags( formats, ExportFormat::All ) != 0)  // there are unknown format flags
	{
		logLogicError() << "Attempted to enable unknown item export formats: " << Qt::hex << formats;
		unsetFlags( formats, withoutFlags( formats, ExportFormat::All ) );
	}

	enabledImportFormats = formats;
}
