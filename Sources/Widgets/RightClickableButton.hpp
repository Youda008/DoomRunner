//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: QToolButton that emits a signal when right-clicked
//======================================================================================================================

#ifndef RIGHTCLICKABLE_BUTTON_INCLUDED
#define RIGHTCLICKABLE_BUTTON_INCLUDED


#include "Essential.hpp"

#include <QToolButton>


//======================================================================================================================
/// QToolButton that emits a signal when right-clicked

class RightClickableButton : public QToolButton {

	Q_OBJECT

	using ThisClass = RightClickableButton;
	using SuperClass = QToolButton;

 public:

	RightClickableButton( QWidget * parent );
	virtual ~RightClickableButton() override;

 private slots:

	void mousePressEvent( QMouseEvent * e ) override;

 signals:

	void rightClicked();

};


#endif // RIGHTCLICKABLE_BUTTON_INCLUDED
