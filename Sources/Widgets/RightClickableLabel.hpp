//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: QLabel that emits a signal when right-clicked
//======================================================================================================================

#ifndef RIGHTCLICKABLE_LABEL_INCLUDED
#define RIGHTCLICKABLE_LABEL_INCLUDED


#include "Essential.hpp"

#include <QLabel>


//======================================================================================================================
/// QLabel that emits a signal when right-clicked

class RightClickableLabel : public QLabel {

	Q_OBJECT

	using ThisClass = RightClickableLabel;
	using SuperClass = QLabel;

 public:

	RightClickableLabel( QWidget * parent );
	virtual ~RightClickableLabel() override;

	QAction * addAction( const QString & text, const QKeySequence & shortcut );

 private: // overriden event callbacks

	//virtual void mousePressEvent( QMouseEvent * e ) override;
	virtual void contextMenuEvent( QContextMenuEvent * e ) override;

 signals:

	//void rightClicked();

 private:

	QMenu * contextMenu = nullptr;

};


#endif // RIGHTCLICKABLE_LABEL_INCLUDED
