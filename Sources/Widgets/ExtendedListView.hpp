//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: list view that supports editing of item names and behaves correctly on both internal and external
//              drag&drop operations
//======================================================================================================================

#ifndef EXTENDED_LIST_VIEW_INCLUDED
#define EXTENDED_LIST_VIEW_INCLUDED


#include "Essential.hpp"

#include "ExtendedViewCommon.hpp"
#include "DataModels/ModelCommon.hpp"  // AccessStyle
#include "Utils/EventFilters.hpp"      // ModifierHandler
class AListModel;
class DnDProgressGuard;

#include <QListView>
class QString;

#include <type_traits>  // underlying_type
#include <optional>


//======================================================================================================================
// drag&drop properties

/// What kind of data will represent an item when it is dragged out of the widget.
namespace DnDOutputType { enum Values : uint
{
	None       = 0,
	FilePaths  = (1 << 0),   ///< the items will be exported as file-system paths
};}
using DnDOutputTypes = std::underlying_type_t< DnDOutputType::Values >;

/// Where the items have been dragged from.
namespace DnDSource { enum Values : uint
{
	None         = 0,
	ThisWidget   = (1 << 0),   ///< drag&drop within this widget for manual reordering of the items in the list
	OtherWidget  = (1 << 1),   ///< drag&drop from another widget for moving items between different widgets
	ExternalApp  = (1 << 2),   ///< drag&drop from a file explorer window for inserting file paths into the list
};}
using DnDSources = std::underlying_type_t< DnDSource::Values >;

struct DnDSourcesExp
{
	bool thisWidget;
	bool otherWidget;
	bool externalApp;
};
template<> inline DnDSourcesExp expandToBools< DnDSourcesExp, DnDSources >( DnDSources dndSources )
{
	return
	{
		isFlagSet( dndSources, DnDSource::ThisWidget ),
		isFlagSet( dndSources, DnDSource::OtherWidget ),
		isFlagSet( dndSources, DnDSource::ExternalApp ),
	};
}


//======================================================================================================================
/** List view that supports editing of item names and behaves correctly on both internal and external drag&drop actions.
  * Should be used together with GenericListModel. */

class ExtendedListView : public QListView, public ExtendedViewCommon< ExtendedListView > {

	Q_OBJECT

	using ThisClass = ExtendedListView;
	using QBaseView = QListView;  ///< The Qt's abstract view class we inherit from

 public:

	ExtendedListView( QWidget * parent );
	virtual ~ExtendedListView() override;

	virtual void setModel( QAbstractItemModel * model ) override;

	//-- icons ---------------------------------------------------------------------------------------------------------

	/// Enables/disables the item icons and updates the text of the context menu entry, default is disabled.
	void toggleIcons( bool enabled );

	bool areIconsEnabled() const;

	//-- editing item content ------------------------------------------------------------------------------------------

	/// Enables/disables checkboxes in front of items, default is disabled.
	void toggleCheckboxes( bool enabled );

	/// Enables/disables editing the item names by double-clicking on them, default is disabled.
	void toggleItemEditing( bool enabled );

	/// Returns whether any of the items is in edit mode (after double-click, F2, etc...).
	bool isItemBeingEdited() const;

	/// Opens edit mode for the current item.
	bool startEditingCurrentItem();

	/// Closes edit mode for the currently edited and commits the edit data into the model.
	void stopEditingAndCommit();

	//-- drag&drop -----------------------------------------------------------------------------------------------------

	/// Configures the view properties to export the items dragged out of this widget into selected types.
	/**
	  * \param outputTypes What kind of data will represent items when they are dragged out of the widget.
	  */
	void setDnDOutputTypes( DnDOutputTypes outputTypes );

	/// Configures the view properties to support the selected input drag&drop operations.
	/**
	  * \param dndSources From which drag&drop sources can items be dropped into this widget.
	  */
	void setAllowedDnDSources( DnDSources dndSources );

	void toggleAllowedDnDSources( DnDSources dndSourcesToSwitch, bool enabled );

	/// If this is true, it means some insert or remove operations are about to be performed.
	bool isDragAndDropInProgress() const  { return isBeingDraggedFrom || isBeingDroppedTo; }

	//-- context menu --------------------------------------------------------------------------------------------------

	/// \copydoc _enableContextMenu()
	void enableContextMenu( MenuActions actions );

	/// \copydoc _addAction()
	QAction * addAction( const QString & text, const QKeySequence & shortcut );

	/// \copydoc _toggleListModifications()
	void toggleListModifications( bool enabled );

 signals:

	/// Emitted either when items are dropped to this view from another widget or just moved within this view itself.
	/** This is always preceeded by signal rowsInserted() (or itemsInserted() in case of GenericListModel)
	  * and only means that all the list and selection updates have finished and the view + model is in its final state.
	  *
	  * \param destRowIdx Index of the first row in the destination list where the items were dropped.
	  * \param itemCount Number of the items that were dropped.
	  * \param type Type of the drag&drop event (see enum DnDType).
	  */
	void dragAndDropFinished( int destRowIdx, int itemCount, DnDSources source );

 private: // overriden event callbacks

	virtual void contextMenuEvent( QContextMenuEvent * event ) override;

	virtual void dragEnterEvent( QDragEnterEvent * event ) override;
	virtual void dragMoveEvent( QDragMoveEvent * event ) override;
	virtual void dropEvent( QDropEvent * event ) override;
	virtual void startDrag( Qt::DropActions supportedActions ) override;

	virtual void keyPressEvent( QKeyEvent * event ) override;
	virtual void keyReleaseEvent( QKeyEvent * event ) override;

 private slots:

	friend class ExtendedViewCommon;

	void openCurrentFile();
	void openCurrentFileLocation();

	void cutSelectedItems();
	void copySelectedItems();
	void pasteAboveSelectedItem();

	void toggleIcons();

 private: // helpers

	bool isReadOnly() const   { return access == AccessStyle::ReadOnly; }

	/// Sets the properties of the Qt's list view so that it supports our drag&drop settings.
	void updateQtViewProperties();
	/// Sets up the data model to support the export and import formats we need for our drag&drop settings to work.
	void updateModelExportImportFormats();

	/// Which one of our drag&drop types this drop event represents.
	DnDSources getDnDSource( QDropEvent * event ) const;
	/// Which one of the Qt's drop actions do we want to perform based on our drag&drop settings.
	Qt::DropAction getPreferredDnDAction( DnDSources dndSource ) const;
	/// Does proposed drop operation comply with our drag&drop settings?
	bool isDropAcceptable( QDragMoveEvent * event ) const;

	/// Retrieves drop indexes, updates selection and emits a signal.
	void onDragAndDropFinished( DnDSources source, DnDProgressGuard & dndProgressGuard );

	void toggleCheckStateOfSelectedItems();

	QString getCurrentFilePath() const;

 private: // internal members

	AListModel * ownModel = nullptr;  ///< quick access to our own specialized model
	ModifierHandler modifierHandler;
	bool isBeingDraggedFrom = false;
	bool isBeingDroppedTo = false;
	std::optional< DnDSources > droppedFrom;

 private: // configuration

	AccessStyle access = AccessStyle::ReadOnly;
	bool allowEditNames = false;
	DnDOutputTypes enabledDnDOutputTypes = 0;
	DnDSources allowedDnDSources = 0;

};


//======================================================================================================================


#endif // EXTENDED_LIST_VIEW_INCLUDED
