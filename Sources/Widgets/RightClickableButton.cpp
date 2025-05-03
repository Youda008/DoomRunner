//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: QToolButton that pops up a context menu when right-clicked
//======================================================================================================================

#include "RightClickableButton.hpp"

#include "RightClickableWidget.impl.hpp"


//======================================================================================================================

RightClickableButton::RightClickableButton( QWidget * parent ) : RightClickableWidget( parent ) {}

QAction * RightClickableButton::addAction( const QString & text, const QKeySequence & shortcut )
{
	return _addAction( text, shortcut );
}

/*void RightClickableButton::mousePressEvent( QMouseEvent * event )
{
	return _mousePressEvent( event );
}*/

void RightClickableButton::contextMenuEvent( QContextMenuEvent * event )
{
	return _contextMenuEvent( event );
}
