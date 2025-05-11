//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: QLabel that pops up a context menu when right-clicked
//======================================================================================================================

#ifndef RIGHTCLICKABLE_LABEL_INCLUDED
#define RIGHTCLICKABLE_LABEL_INCLUDED


#include "RightClickableWidget.hpp"

#include <QLabel>


//======================================================================================================================
/// QLabel that pops up a context menu when right-clicked

class RightClickableLabel : public RightClickableWidget< QLabel > {

	Q_OBJECT

 public:

	RightClickableLabel( QWidget * parent );

	QAction * addMenuAction( const QString & text, const QKeySequence & shortcut );

 private: // overriden event callbacks

	//virtual void mousePressEvent( QMouseEvent * event ) override;
	virtual void contextMenuEvent( QContextMenuEvent * event ) override;

 signals:

	//void rightClicked();

};


//======================================================================================================================


#endif // RIGHTCLICKABLE_LABEL_INCLUDED
