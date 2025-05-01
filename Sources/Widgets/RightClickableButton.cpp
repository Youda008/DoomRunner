//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: QToolButton that emits a signal when right-clicked
//======================================================================================================================

#include "RightClickableButton.hpp"

#include <QMouseEvent>


//======================================================================================================================

RightClickableButton::RightClickableButton( QWidget * parent ) : QToolButton( parent ) {}

RightClickableButton::~RightClickableButton() {}

void RightClickableButton::mousePressEvent( QMouseEvent * e )
{
	SuperClass::mousePressEvent( e );

	if (e->button() == Qt::RightButton)
		emit rightClicked();
}
