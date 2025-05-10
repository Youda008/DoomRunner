//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: QTreeView extended by own general-purpose functionality
//======================================================================================================================

#ifndef EXTENDED_TREE_VIEW_INCLUDED
#define EXTENDED_TREE_VIEW_INCLUDED


#include "Essential.hpp"

#include "ExtendedViewCommon.hpp"

#include <QTreeView>
#include <QList>
class QFileSystemModel;


//======================================================================================================================
/// QTreeView extended by own general-purpose functionality

class ExtendedTreeView : public QTreeView, public ExtendedViewCommon< ExtendedTreeView > {

	Q_OBJECT

	using ThisClass = ExtendedTreeView;
	using QBaseView = QTreeView;

 public:

	ExtendedTreeView( QWidget * parent );
	virtual ~ExtendedTreeView() override;

	virtual void setModel( QAbstractItemModel * model ) override;

	//-- icons ---------------------------------------------------------------------------------------------------------

	/// Enables/disables the item icons and updates the text of the context menu entry, default is disabled.
	void toggleIcons( bool enabled );

	bool areIconsEnabled() const;

	//-- automatic column resizing -------------------------------------------------------------------------------------

	/// Enables/disables automatic resizing of the columns according to the content they hold.
	/** This will regularly keep checking width of the columns and extend them if the content is too wide to fit in,
	  * showing a horizontal scrollbar if the width is greater than the width of the widget. */
	void toggleAutomaticColumnResizing( bool enabled )  { automaticallyResizeColumns = enabled; }

	//-- context menu --------------------------------------------------------------------------------------------------

	/// \copydoc _enableContextMenu()
	void enableContextMenu( MenuActions actions );

	/// \copydoc _addAction()
	QAction * addAction( const QString & text, const QKeySequence & shortcut );

	/// \copydoc _toggleListModifications()
	void toggleListModifications( bool enabled );

 private: // overriden event callbacks

	virtual void contextMenuEvent( QContextMenuEvent * event ) override;

 public slots:

	void updateColumnSize();

 private slots:

	friend class ExtendedViewCommon;

	void onDataChanged( const QModelIndex &, const QModelIndex &, const QVector<int> & );
	void onLayoutChanged( const QList< QPersistentModelIndex > &, QAbstractItemModel::LayoutChangeHint );

	void openCurrentFile();
	void openCurrentFileLocation();

	void cutSelectedItems();
	void copySelectedItems();
	void pasteAboveSelectedItem();

	void toggleIcons();

 private: // helpers

	bool isReadOnly() const   { return true; }

	QString getCurrentFilePath() const;

 private: // internal members

	QFileSystemModel * fsModel = nullptr;  ///< quick access to specialized file-system model

 private: // configuration

	bool automaticallyResizeColumns = false;

};


//======================================================================================================================


#endif // EXTENDED_TREE_VIEW_INCLUDED
