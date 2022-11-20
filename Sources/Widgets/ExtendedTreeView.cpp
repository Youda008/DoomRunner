//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: QTreeView extended by own general-purpose functionality
//======================================================================================================================

#include "ExtendedTreeView.hpp"


//======================================================================================================================

ExtendedTreeView::ExtendedTreeView( QWidget * parent ) : QTreeView( parent ) {}

ExtendedTreeView::~ExtendedTreeView() {}

void ExtendedTreeView::setModel( QAbstractItemModel * model )
{
	superClass::setModel( model );

	updateColumnSize();  // adapt view to the current state of the new model
	connect( model, &QAbstractItemModel::dataChanged, this, &thisClass::onDataChanged );  // prepare for future changes
	connect( model, &QAbstractItemModel::layoutChanged, this, &thisClass::onLayoutChanged );  // prepare for future changes
}

void ExtendedTreeView::onDataChanged( const QModelIndex &, const QModelIndex &, const QVector<int> & )
{
	updateColumnSize();
}

void ExtendedTreeView::onLayoutChanged( const QList< QPersistentModelIndex > &, QAbstractItemModel::LayoutChangeHint )
{
	updateColumnSize();
}

void ExtendedTreeView::updateColumnSize()
{
	// The tree view operates in columns and text that does not fit in the column's width is clipped.
	// This is the only way how to always keep the column wide enough for all the currently visible items to fit in
	// and rather display a horizontal scrollbar when they are wider than the widget.
	if (automaticallyResizeColumns)
	{
		for (int columnIdx = 0; columnIdx < model()->columnCount(); ++columnIdx)
			if (!this->isColumnHidden( columnIdx ))
				this->resizeColumnToContents( columnIdx );
	}
}
