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

void ExtendedTreeView::paintEvent( QPaintEvent * event )
{
	// The tree view operates in columns and text that does not fit in the column's width is clipped.
	// This is the only way how to always keep the column wide enough for all the currently visible items to fit in
	// and rather display a horizontal scrollbar when they are wider than the widget.
	//
	// TODO: hook only events that can cause the need for resizing, don't do it every update.
	if (automaticallyResizeColumns)
	{
		for (int columnIdx = 0; columnIdx < model()->columnCount(); ++columnIdx)
			if (!this->isColumnHidden( columnIdx ))
				this->resizeColumnToContents( columnIdx );
	}

	superClass::paintEvent( event );
}
