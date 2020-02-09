//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  13.5.2019
// Description: mediators between a list of arbitrary objects and list view or other widgets
//======================================================================================================================

#ifndef ITEM_MODELS_INCLUDED
#define ITEM_MODELS_INCLUDED


#include "Common.hpp"

#include "Utils.hpp"

#include <QAbstractListModel>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>

#include <QDebug>


//======================================================================================================================
// We use model-view design pattern for several widgets, because it allows us to have all the related data
// packed together in one struct, and have the UI automatically mirror the underlying list without manually syncing
// the underlying list (backend) with the widget list (frontend), and also because the data can be shared in
// multiple widgets, even across multiple windows/dialogs.
//
// Model and its underlying list are separated, the model doesn't hold the list inside itself.
// It's because we want to display the same data differently in different widgets or different dialogs.
// Therefore the models are merely mediators between the data and views,
// which presents the data to the views and propagate user input from the views back to data.
//
// You can read more about it here: https://doc.qt.io/qt-5/model-view-programming.html#model-subclassing-reference


//======================================================================================================================
/** Abstract wrapper around list of arbitrary objects, mediating their content to UI view elements.
  * The model doesn't own the data, they are stored somewhere else, it just presents them to the UI. */

template< typename Item >
class AItemListModel : public QAbstractListModel {

 protected:

	QList< Item > & itemList;

 public:

	AItemListModel( QList< Item > & itemList ) : QAbstractListModel( nullptr ), itemList( itemList ) {}

	QList< Item > & list() const { return itemList; }

	int rowCount( const QModelIndex & = QModelIndex() ) const override
	{
		return itemList.size();
	}

	QModelIndex makeIndex( int row )
	{
		return index( row, /*column*/0, /*parent*/QModelIndex() );
	}


	/** notifies the view that the content of some items has been changed */
	void contentChanged( int changedRowsBegin, int changedRowsEnd = -1 )
	{
		if (changedRowsEnd < 0)
			changedRowsEnd = itemList.size();

		const QModelIndex firstChangedIndex = createIndex( changedRowsBegin, /*column*/0 );
		const QModelIndex lastChangedIndex = createIndex( changedRowsEnd - 1, /*column*/0 );

		emit dataChanged( firstChangedIndex, lastChangedIndex, {Qt::DisplayRole, Qt::EditRole, Qt::CheckStateRole} );
	}

	// One of the following functions must always be called before and after doing any modifications to the list,
	// otherwise the list might not update correctly or it might even crash trying to access items that no longer exist.

	void startAppending( int count = 1 )
	{
		beginInsertRows( QModelIndex(), itemList.size(), itemList.size() + count );
	}
	void finishAppending()
	{
		endInsertRows();
	}

	void startDeleting( int row )
	{
		beginRemoveRows( QModelIndex(), row, row );
	}
	void finishDeleting()
	{
		endRemoveRows();
	}

	void startCompleteUpdate()
	{
		beginResetModel();
	}
	void finishCompleteUpdate()
	{
		endResetModel();
	}

};


//======================================================================================================================
/** Wrapper around list of arbitrary objects, mediating their content to UI view elements with read-only access. */

template< typename Item >
class ReadOnlyListModel : public AItemListModel< Item > {

	using superClass = AItemListModel< Item >;

 protected:

	// This template class doesn't know about the structure of Item, it's supposed to be universal for any.
	// Therefore only author of Item knows how to display Item in the widget, so he must specify it by a function.
	std::function< QString ( const Item & ) > makeDisplayString;

 public:

	ReadOnlyListModel( QList< Item > & itemList, std::function< QString ( const Item & ) > makeDisplayString )
		: AItemListModel<Item>( itemList ), makeDisplayString( makeDisplayString ) {}

	void setDisplayStringFunc( std::function< QString ( const Item & ) > makeDisplayString )
		{ this->makeDisplayString = makeDisplayString; }

	QVariant data( const QModelIndex & index, int role ) const override
	{
		if (!index.isValid() || index.row() >= superClass::itemList.size())
			return QVariant();

		if (role == Qt::DisplayRole) {
			// Some UI elements may want to display only the Item name, some others a string constructed from multiple
			// Item elements. This way we generalize from the way the display string is constructed from the Item.
			return makeDisplayString( superClass::itemList[ index.row() ] );
		} else {
			return QVariant();
		}
	}

};


//======================================================================================================================
/** Wrapper around list of arbitrary objects, mediating their names to UI view elements.
  * Supports in-place editing, internal drag&drop reordering, and external file drag&drops. */

template< typename Item >
class EditableListModel : public AItemListModel< Item > {

	using superClass = AItemListModel<Item>;

 protected:

	// This template class doesn't know about the structure of Item, it's supposed to be universal for any.
	// Therefore only author of Item knows how to perform certain operations on it, so he must specify it by functions

	/// function that points the model to a string member of Item containing the text to be displayed in the widget
	std::function< QString & ( Item & ) > displayString;

	/// function that assigns a dropped file into a newly created Item
	std::function< void	( Item &, const QFileInfo & ) > assignFile;

	/// function that points the model to a bool flag of Item indicating whether the item is checked
	std::function< bool & ( Item & ) > isChecked;

	bool checkableItems;

 public:

	EditableListModel( QList< Item > & itemList, std::function< QString & ( Item & ) > displayString )
		: AItemListModel<Item>( itemList ), displayString( displayString ), checkableItems( false ) {}

	void setDisplayStringFunc( std::function< QString & ( Item & ) > displayString )
		{ this->displayString = displayString; }
	void setAssignFileFunc( std::function< void ( Item &, const QFileInfo & ) > assignFile )
		{ this->assignFile = assignFile; }
	void setIsCheckedFunc( std::function< bool & ( Item & ) > isChecked )
		{ this->isChecked = isChecked; }
	void toggleCheckable( bool enabled ) { checkableItems = enabled; }

	Qt::ItemFlags flags( const QModelIndex & index ) const override
	{
		if (!index.isValid())
			return Qt::ItemIsDropEnabled;

		Qt::ItemFlags flags = QAbstractListModel::flags(index);

		flags |= Qt::ItemIsDragEnabled;
		flags |= Qt::ItemIsEditable;
		if (checkableItems)
			flags |= Qt::ItemIsUserCheckable;

		return flags;
	}

	QVariant data( const QModelIndex & index, int role ) const override
	{
		if (!index.isValid() || index.parent().isValid() || index.row() >= superClass::itemList.size())
			return QVariant();

		// TODO: split displayString and editString

		if (role == Qt::DisplayRole || role == Qt::EditRole) {
			// This template class doesn't know about the structure of Item, it's supposed to be universal for any.
			// Therefore only author of Item knows which of its memebers he wants to display in the widget,
			// so he must specify it by a function.
			return displayString( superClass::itemList[ index.row() ] );
		} if (role == Qt::CheckStateRole && checkableItems) {
			// Same as above, exept that this function is optional to ease up initializations of models
			// that are supposed to be used in non-checkable widgets
			if (!isChecked) {
				qWarning() << "checkableItems has been set, but no isChecked function is specified. "
				              "Either specify an isChecked function or disable disable checkable items.";
				return QVariant();
			} else {
				return isChecked( superClass::itemList[ index.row() ] ) ? Qt::Checked : Qt::Unchecked;
			}
		} else {
			return QVariant();
		}
	}

	bool setData( const QModelIndex & index, const QVariant & value, int role ) override
	{
		if (index.parent().isValid() || !index.isValid() || index.row() >= superClass::itemList.size())
			return false;

		if (role == Qt::EditRole) {
			displayString( superClass::itemList[ index.row() ] ) = value.toString();
			emit superClass::dataChanged( index, index, {Qt::EditRole} );
			return true;
		} else if (role == Qt::CheckStateRole && checkableItems) {
			if (!isChecked) {
				qWarning() << "checkableItems has been set, but no isChecked function is specified. "
				              "Either specify an isChecked function or disable disable checkable items.";
				return false;
			} else {
				isChecked( superClass::itemList[ index.row() ] ) = (value == Qt::Checked ? true : false );
				emit superClass::dataChanged( index, index, {Qt::CheckStateRole} );
				return true;
			}
		} else {
			return false;
		}
	}

	bool insertRows( int row, int count, const QModelIndex & parent ) override
	{
		if (parent.isValid()) {
			return false;
		}

		QAbstractListModel::beginInsertRows( parent, row, row + count - 1 );

		// n times moving all the elements forward to insert one is not nice
		// but it happens only once in awhile and the number of elements is almost always very low
		for (int i = 0; i < count; i++)
			superClass::itemList.insert( row + i, Item() );

		QAbstractListModel::endInsertRows();

		return true;
	}

	bool removeRows( int row, int count, const QModelIndex & parent ) override
	{
		if (parent.isValid() || row < 0 || row + count > superClass::itemList.size()) {
			return false;
		}

		QAbstractListModel::beginRemoveRows( parent, row, row + count - 1 );

		// n times moving all the elements backward to insert one is not nice
		// but it happens only once in awhile and the number of elements is almost always very low
		for (int i = 0; i < count; i++) {
			superClass::itemList.removeAt( row );
			if (row < _droppedRow)  // we are removing a row that is before the target row
				_droppedRow--;      // so target row's index is moving backwards
		}

		QAbstractListModel::endRemoveRows();

		return true;
	}

	Qt::DropActions supportedDropActions() const override
	{
		return Qt::MoveAction | Qt::CopyAction;
	}

	static constexpr const char * const internalMimeType = "application/EditableListModel-internal";
	static constexpr const char * const itemListMimeType = "application/x-qabstractitemmodeldatalist";
	static constexpr const char * const filePathMimeType = "application/x-qt-windows-mime;value=\"FileName\"";

	QStringList mimeTypes() const override
	{
		QStringList types;

		types << internalMimeType;  // for internal drag&drop reordering
		types << itemListMimeType;  // for drag&drop from other list list widgets
		types << filePathMimeType;  // for drag&drop from directory window

		return types;
	}

	bool canDropMimeData( const QMimeData * mime, Qt::DropAction action, int /*row*/, int /*col*/, const QModelIndex & ) const override
	{
		return (mime->hasFormat( internalMimeType ) && action == Qt::MoveAction) // for internal drag&drop reordering
		    || (mime->hasFormat( itemListMimeType ))                             // for drag&drop from other list list widgets
		    || (mime->hasFormat( filePathMimeType ));                            // for drag&drop from directory window
	}

	/// serializes items at <indexes> into MIME data
	QMimeData * mimeData( const QModelIndexList & indexes ) const override
	{
		QMimeData * mimeData = new QMimeData;

		// Because we want only internal drag&drop for reordering the items, we don't need to serialize the whole rich
		// content of each Item and then deserialize all of it back. Instead we can serialize only indexes of the items
		// and then use them in dropMimeData to find the original items and copy/move them to the target position
		QByteArray encodedData( indexes.size() * int(sizeof(int)), 0 );
		int * rawData = reinterpret_cast< int * >( encodedData.data() );

		for (const QModelIndex & index : indexes) {
			*rawData = index.row();
			rawData++;
		}

		mimeData->setData( internalMimeType, encodedData );
		return mimeData;
	}

	/// deserializes items from MIME data and inserts them before <row>
	bool dropMimeData( const QMimeData * mime, Qt::DropAction action, int row, int, const QModelIndex & parent ) override
	{
		// in edge cases always append to the end of the list
		if (row < 0 || row > superClass::itemList.size())
			row = superClass::itemList.size();

		if (mime->hasFormat( internalMimeType ) && action == Qt::MoveAction) {
			return dropInternalItems( mime->data( internalMimeType ), row, parent );
		} else if (mime->hasUrls()) {
			return dropMimeUrls( mime->urls(), row, parent );
		} else {
			qWarning() << "This model doesn't support such drop operation. It should have been restricted by the ListView.";
			return false;
		}
	}

	bool dropInternalItems( QByteArray encodedData, int row, const QModelIndex & parent )
	{
		// retrieve the original row indexes of the items to be moved
		const int * rawData = reinterpret_cast< int * >( encodedData.data() );
		int count = encodedData.size() / int(sizeof(int));

		// Because insertRows shifts the items and invalidates the indexes, we need to capture the original items before
		// allocating space at the target position. We can avoid making a local temporary copy of Item by abusing
		// the fact that QList is essentially an array of pointers to objects (official API documentation says so).
		// So we can just save the pointers to the items and when the rows are inserted and shifted, those pointers
		// will still point to the correct items and we can use them copy or move the data from them to the target pos.

		// We could probably avoid even the copy/move constructors and the allocation when inserting an item into QList,
		// but we would need a direct access to the pointers inside QList which it doesn't provide.
		// Alternativelly the QList<Item> could be replaced with QVector<Item*> or QVector<unique_ptr<Item>>
		// but that would just complicate a lot of other things.

		QVector< Item * > origItemRefs;
		for (int i = 0; i < count; i++) {
			int origRowIdx = rawData[i];
			origItemRefs.append( &superClass::itemList[ origRowIdx ] );
		}

		// allocate space for the items to move to
		insertRows( row, count, parent );

		// move the original items to the target position
		for (int i = 0; i < count; i++) {
			superClass::itemList[ row + i ] = std::move( *origItemRefs[i] );
		}

		// idiotic workaround because Qt is fucking retarded
		//
		// When an internal reordering drag&drop is performed, Qt doesn't update the selection and leaves the selection
		// on the old indexes, where are now some completely different items.
		// You can't manually update the indexes here, because at some point after dropMimeData Qt calls removeRows on items
		// that are CURRENTLY SELECTED, instead of on items that were selected at the beginning of this drag&drop operation.
		// So we must update the selection at some point AFTER the drag&drop operation is finished and the rows removed.
		//
		// But outside an item model, there is no information abouth the target drop index. So we must write down
		// the index here and then let other classes retrieve it at the right time.
		_dropped = true;
		_droppedRow = row;
		_droppedCount = count;

		superClass::contentChanged( row, row + count );

		return true;
	}

	bool dropMimeUrls( QList< QUrl > urls, int row, const QModelIndex & parent )
	{
		if (!assignFile) {
			qWarning() << "File has been dropped but no assignFile function was set. "
			              "Either specify an assignFile function or disable file dropping in the widget";
			return false;
		}

		// first we need to know how many items will be inserted, so that we can allocate space for them
		QList< QFileInfo > filesToBeInserted;
		for (const QUrl & droppedUrl : urls) {
			QString localPath = droppedUrl.toLocalFile();
			if (!localPath.isEmpty()) {
				QFileInfo fileInfo( localPath );
				if (fileInfo.exists()) {
					filesToBeInserted.append( fileInfo );
				}
			}
		}

		// allocate space for the items to be dropped to
		insertRows( row, filesToBeInserted.count(), parent );

		for (int i = 0; i < filesToBeInserted.count(); i++) {
			// This template class doesn't know about the structure of Item, it's supposed to be universal for any.
			// Therefore only author of Item knows how to assign a dropped file into it, so he must define it by a function.
			assignFile( superClass::itemList[ row + i ], filesToBeInserted[ i ] );
		}

		// idiotic workaround because Qt is fucking retarded, read the big comment above
		_dropped = true;
		_droppedRow = row;
		_droppedCount = filesToBeInserted.count();

		superClass::contentChanged( row, _droppedCount );

		return true;
	}

 protected:

	// idiotic workaround because Qt is fucking retarded, read the big comment above
	bool _dropped;
	int _droppedRow;
	int _droppedCount;

 public:

	bool wasDroppedInto() { bool d = _dropped; _dropped = false; return d; }
	int droppedRow() const { return _droppedRow; }
	int droppedCount() const { return _droppedCount; }

};


//======================================================================================================================
//  simple single-column icon-less tree model

class TreeNode {

 public:

	explicit TreeNode( const QString & name, TreeNode * parent ) : _name( name ), _parent( parent ) {}
	~TreeNode() { deleteChildren(); }

	QString name() const               { return _name; }

	TreeNode * parent() const          { return _parent; }
	TreeNode * child( int row ) const  { return _children[ row ]; }
	int childCount() const             { return _children.size(); }
	const auto & children() const      { return _children; }

	int row() const
	{
		return !_parent ? -1 : _parent->_children.indexOf( const_cast< TreeNode * >( this ) );
	}

	/** returns a child with this name, or nullptr if it doesn't exist */
	TreeNode * child( const QString & name )
	{
		// linear complexity, but we expect the sublists to be small
		int idx = findSuch< TreeNode * >( _children, [ &name ]( TreeNode * node ) { return node->name() == name; } );
		return idx >= 0 ? _children[ idx ] : nullptr;
	}

	/** creates and returns a child of this name, or returns an existing one if it already exists */
	TreeNode * addChild( const QString & name )
	{
		// linear complexity, but we expect the sublists to be small
		int idx = findSuch< TreeNode * >( _children, [ &name ]( TreeNode * node ) { return node->name() == name; } );
		if (idx >= 0) {
			return _children[ idx ];
		} else {
			_children.append( new TreeNode( name, this ) );
			return _children.last();
		}
	}

	void deleteChildren()
	{
		for (TreeNode * child : _children)
			delete child;
		_children.clear();
	}

 private:

	// Each node must have a name in order to be able to construct persistent tree paths. When the tree is then updated
	// (new parent nodes and leaf nodes are inserted), indexes change but the paths stay the same.
	QString _name;

	QVector< TreeNode * > _children;
	TreeNode * _parent;

};


/** sequence of names of parent nodes ordered from root to leaf, unambiguously defining a node in a tree */
class TreePath : public QStringList {
 public:
	using QStringList::QStringList;

	/** makes TreePath from string in file-system format ("node1/node2/leaf") */
	TreePath( const QString & pathStr ) : QStringList( pathStr.split( '/', QString::SkipEmptyParts ) ) {}
};


//----------------------------------------------------------------------------------------------------------------------
/** Simple single-column icon-less tree model.
  * Unlike the other models, this one holds the data, and so it cannot be displayed differently in different windows. */
class TreeModel : public QAbstractItemModel {

	// Note for someone who would is going to separate the thee from tree model to allow data sharing between windows:
	// Don't forget that everytime the tree changes (child nodes are added or deleted) you have to call corresponding
	// beginSomething(...) and endSomething(...) so that the abstract class and the view properly update themselfs.
	// Read the documentation on these functions: https://doc.qt.io/qt-5/qabstractitemmodel.html#protected-functions

	Q_OBJECT

 public:

	TreeModel( QObject * parent = nullptr )
		: QAbstractItemModel( parent )
	{
		rootNode = new TreeNode( "", nullptr );
	}

	~TreeModel() override
	{
		delete rootNode;  // recursively deletes all the child nodes
	}

	//-- implementation of QAbstractItemModel's virtual methods --------------------------------------------------------

	int rowCount( const QModelIndex & parentIndex = QModelIndex() ) const override
	{
		TreeNode * const parent = modelIndexToTreeNode( parentIndex );
		return parent->childCount();
    }

	int columnCount( const QModelIndex & /*parentIndex*/ = QModelIndex() ) const override
	{
		return 1;
	}

	Qt::ItemFlags flags( const QModelIndex & index ) const override
	{
		if (!index.isValid())
			return Qt::NoItemFlags;
		return QAbstractItemModel::flags( index );
	}

	QVariant data( const QModelIndex & index, int role ) const override
	{
		if (!index.isValid())
			return QVariant();

		TreeNode * const node = static_cast< TreeNode * >( index.internalPointer() );

		if (role == Qt::DisplayRole) {
			return node->name();
		} else {
			return QVariant();
		}
	}

    QVariant headerData( int /*section*/, Qt::Orientation /*orientation*/, int /*role*/ ) const override
    {
		return QVariant();  // no header support
    }

	QModelIndex index( int row, int column, const QModelIndex & parentIndex = QModelIndex() ) const override
	{
		if (!hasIndex( row, column, parentIndex ))  // checks bounds (>= 0 && < rowCount) for given parent
			return QModelIndex();

		TreeNode * const parent = modelIndexToTreeNode( parentIndex );
		TreeNode * const sibling = parent->child( row );
		return createIndex( row, 0, sibling );
	}

	QModelIndex parent( const QModelIndex & index ) const override
	{
		if (!index.isValid())
			return QModelIndex();

		TreeNode * const node = modelIndexToTreeNode( index );
		TreeNode * const parent = node->parent();
		return treeNodeToModelIndex( parent );
	}

	//-- custom methods for manipulating the tree ----------------------------------------------------------------------

	QModelIndex makeIndex( int row, const QModelIndex & parentIndex ) const
	{
		return index( row, 0, parentIndex );
	}

	/** Notifies Qt that all the model indexes and data retrieved before are no longer valid.
	  * Call this before every model update. */
	void startCompleteUpdate()
	{
		beginResetModel();
	}

	/** Call this when an update process is finished and makes a view re-draw its content according to the new data. */
	void finishCompleteUpdate()
	{
		endResetModel();
	}

	/** Note that before starting to add or delete items in this model, you have to call startCompleteUpdate,
	  * and when you are finished with it, you have to call finishCompleteUpdate. */
	QModelIndex addItem( const QModelIndex & parentIndex, QString name )
	{
		TreeNode * parent = modelIndexToTreeNode( parentIndex );
		return treeNodeToModelIndex( parent->addChild( name ) );
	}

	/*QModelIndex addItem( const TreePath & path )
	{
		TreeNode * node = rootNode;
		for (const QString & nodeName : path) {
			node = node->addChild( nodeName );
		}
		return treeNodeToModelIndex( node );
	}*/

	/** Note that before starting to add or delete items in this model, you have to call startCompleteUpdate,
	  * and when you are finished with it, you have to call finishCompleteUpdate. */
	void clear()
	{
		rootNode->deleteChildren();
	}

	/** item's path that can be used as a persistent item identifier that survives node shifting, adding or removal */
	TreePath getItemPath( const QModelIndex & index ) const
	{
		TreePath path;

		TreeNode * node = modelIndexToTreeNode( index );
		while (node != rootNode) {
			path.append( node->name() );
			node = node->parent();
		}
		reverse( path );

		return path;
	}

	/** attempts to find an item on specified path */
	QModelIndex getItemByPath( const TreePath & path ) const
	{
		TreeNode * node = rootNode;
		for (const QString & nodeName : path) {
			node = node->child( nodeName );  // linear complexity, but we expect the sublists to be small
			if (!node)
				return QModelIndex();  // node at this path no longer exists
		}
		return treeNodeToModelIndex( node );
	}

	void traverseItems( std::function< void ( const QModelIndex & index ) > doOnItem,
	                    const QModelIndex & parentIndex = QModelIndex() ) const
	{
		TreeNode * const parent = modelIndexToTreeNode( parentIndex );

		for (int childRow = 0; childRow < parent->childCount(); childRow++)
		{
			TreeNode * const child = parent->child( childRow );
			const QModelIndex childIndex = treeNodeToModelIndex( child, childRow );

			doOnItem( childIndex );

			traverseItems( doOnItem, childIndex );
		}
	}

 private:

	TreeNode * rootNode;  ///< internal node that stores all the other nodes without a parent

	// Internally we use TreeNode pointers, but the view accesses the model via QModelIndex.
	// If the view passes in an empty index, it wants an item from the top level, an item that doesn't have any parent.
	// We store such elements under a root node (default parent), because it simplifies the implementation.

	TreeNode * modelIndexToTreeNode( const QModelIndex & index ) const
	{
		// if no parent is specified, use our internal default parent
		if (!index.isValid())
			return rootNode;
		else
			return static_cast< TreeNode * >( index.internalPointer() );
	}
	QModelIndex treeNodeToModelIndex( TreeNode * node, int rowInParent = -1 ) const
	{
		// optimization for cases where the row index is known, otherwise we have to perform linear lookup at the parent
		const int row = rowInParent >= 0 ? rowInParent : node->row();

		// root node is internal only, don't expose it to the outside, for the caller it means having no parent at all.
		if (node == rootNode)
			return QModelIndex();
		else
			return createIndex( row, 0, node );
	}

};


#endif // ITEM_MODELS_INCLUDED
