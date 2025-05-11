//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: QLabel that emits a signal when right-clicked
//======================================================================================================================

#include "RightClickableLabel.hpp"

#include "RightClickableWidget.impl.hpp"


//======================================================================================================================

RightClickableLabel::RightClickableLabel( QWidget * parent ) : RightClickableWidget( parent ) {}

QAction * RightClickableLabel::addMenuAction( const QString & text, const QKeySequence & shortcut )
{
	return _addMenuAction( text, shortcut );
}

/*void RightClickableLabel::mousePressEvent( QMouseEvent * event )
{
	return _mousePressEvent( event );
}*/

void RightClickableLabel::contextMenuEvent( QContextMenuEvent * event )
{
	return _contextMenuEvent( event );
}
