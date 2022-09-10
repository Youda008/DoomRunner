//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: QTreeView extended by own general-purpose functionality
//======================================================================================================================

#ifndef EXTENDED_TREE_VIEW_INCLUDED
#define EXTENDED_TREE_VIEW_INCLUDED


#include "Common.hpp"

#include <QTreeView>


//======================================================================================================================
/// QTreeView extended by own general-purpose functionality

class ExtendedTreeView : public QTreeView {

	Q_OBJECT

	using thisClass = ExtendedTreeView;
	using superClass = QTreeView;

 public:

	ExtendedTreeView( QWidget * parent );
	virtual ~ExtendedTreeView() override;

	/// Enables/disables automatic resizing of the columns according to the content they hold.
	/** This will regularly keep checking width of the columns and extend them if the content is too wide to fit in,
	  * showing a horizontal scrollbar if the width is greater than the width of the widget. */
	void toggleAutomaticColumnResizing( bool enabled )  { automaticallyResizeColumns = enabled; }

 private: // overridden methods

	virtual void paintEvent( QPaintEvent * event ) override;

 protected: // members

	bool automaticallyResizeColumns = false;

};


#endif // EXTENDED_TREE_VIEW_INCLUDED
