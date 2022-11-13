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

	virtual void setModel( QAbstractItemModel * model ) override;

 public slots:

	void updateColumnSize();

 private slots:

	void onDataChanged( const QModelIndex &, const QModelIndex &, const QVector<int> & );
	void onLayoutChanged( const QList< QPersistentModelIndex > &, QAbstractItemModel::LayoutChangeHint );

 protected: // members

	bool automaticallyResizeColumns = false;

};


#endif // EXTENDED_TREE_VIEW_INCLUDED
