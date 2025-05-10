//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: QToolButton that pops up a context menu when right-clicked
//======================================================================================================================

#ifndef RIGHTCLICKABLE_BUTTON_INCLUDED
#define RIGHTCLICKABLE_BUTTON_INCLUDED


#include "RightClickableWidget.hpp"

#include <QToolButton>


//======================================================================================================================
/// QToolButton that pops up a context menu when right-clicked

class RightClickableButton : public RightClickableWidget< QToolButton > {

	Q_OBJECT

 public:

	RightClickableButton( QWidget * parent );

	QAction * addAction( const QString & text, const QKeySequence & shortcut );

 private: // overriden event callbacks

	//virtual void mousePressEvent( QMouseEvent * event ) override;
	virtual void contextMenuEvent( QContextMenuEvent * event ) override;

 signals:

	//void rightClicked();

};


//======================================================================================================================


#endif // RIGHTCLICKABLE_BUTTON_INCLUDED
