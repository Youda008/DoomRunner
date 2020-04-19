//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  13.5.2019
// Description: mediators between a list of arbitrary objects and list view or other widgets
//======================================================================================================================

#ifndef LIST_MODEL_INCLUDED
#define LIST_MODEL_INCLUDED


#include "Common.hpp"

#include "LangUtils.hpp"
#include "FileSystemUtils.hpp"

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>

#include <functional>

#include <QDebug>


//======================================================================================================================
// We use model-view design pattern for several widgets, because it allows us to organize the data in a way we need,
// and have the widget (frontend) automatically mirror the underlying data (backend) without syncing them manually.
//
// You can read more about it here: https://doc.qt.io/qt-5/model-view-programming.html#model-subclassing-reference
//
// The following classes are written as templates, because there is a lot of boilerplate required by Qt for this job.
// So instead of writing such classes with all the boilerplate for every widget we need it for, we have them only once
// and the differences are extracted into user-defined functions. When you instantiate the model, you need to specify
// how the elements should be displayed, how they should be edited and how they should be created from file-system entry.


//======================================================================================================================
/** Abstract wrapper around list of arbitrary objects, mediating their content to UI view elements. */

template< typename Item >
class AListModel : public QAbstractListModel {

 protected:

	QList< Item > itemList;

 public:

	AListModel() : QAbstractListModel( nullptr ) {}

	AListModel( const QList< Item > & itemList ) : QAbstractListModel( nullptr ), itemList( itemList ) {}

	//-- wrapper functions for manipulating the list -------------------------------------------------------------------

	QList< Item > & list()                        { return itemList; }
	const QList< Item > & list() const            { return itemList; }
	void updateList( const QList< Item > & list ) { itemList = list; }

	int count() const                             { return itemList.count(); }
	int size() const                              { return itemList.size(); }
	bool isEmpty() const                          { return itemList.isEmpty(); }
	Item & operator[]( int idx )                  { return itemList[ idx ]; }
	const Item & operator[]( int idx ) const      { return itemList[ idx ]; }
	decltype( itemList.begin() ) begin()          { return itemList.begin(); }
	decltype( itemList.begin() ) begin() const    { return itemList.begin(); }
	decltype( itemList.end() ) end()              { return itemList.end(); }
	decltype( itemList.end() ) end() const        { return itemList.end(); }
	void clear()                                  { itemList.clear(); }
	void append( const Item & item )              { itemList.append( item ); }
	void prepend( const Item & item )             { itemList.prepend( item ); }
	void removeAt( int idx )                      { itemList.removeAt( idx ); }
	void move( int from, int to )                 { itemList.move( from, to ); }
	int indexOf( const Item & item ) const        { return itemList.indexOf( item ); }

	//-- data change notifications -------------------------------------------------------------------------------------

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

	//-- implementation of QAbstractItemModel's virtual methods --------------------------------------------------------

	virtual int rowCount( const QModelIndex & = QModelIndex() ) const override
	{
		return itemList.size();
	}

	//-- miscellaneous -------------------------------------------------------------------------------------------------

	QModelIndex makeIndex( int row )
	{
		return index( row, /*column*/0, /*parent*/QModelIndex() );
	}

};


//======================================================================================================================
/** Wrapper around list of arbitrary objects, mediating their content to UI view elements with read-only access. */

template< typename Item >
class ReadOnlyListModel : public AListModel< Item > {

	using superClass = AListModel< Item >;

 protected:

	// This template class doesn't know about the structure of Item, it's supposed to be universal for any.
	// Therefore only author of Item knows how to display Item in the widget, so he must specify it by a function.
	std::function< QString ( const Item & ) > makeDisplayString;

 public:

	ReadOnlyListModel( std::function< QString ( const Item & ) > makeDisplayString )
		: AListModel< Item >(), makeDisplayString( makeDisplayString ) {}

	ReadOnlyListModel( const QList< Item > & itemList, std::function< QString ( const Item & ) > makeDisplayString )
		: AListModel< Item >( itemList ), makeDisplayString( makeDisplayString ) {}

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
class EditableListModel : public AListModel< Item > {

	using superClass = AListModel< Item >;

 protected:

	// This template class doesn't know about the structure of Item, it's supposed to be universal for any.
	// Therefore only author of Item knows how to perform certain operations on it, so he must specify it by functions

	/** function that takes Item and constructs a String that will be displayed in the view */
	std::function< QString ( const Item & ) > makeDisplayString;

	// TODO: whether list is checkable and editable should be template parameter
	//       with a compile-time check for corresponding struct members or methods

	/** optional function that points the model to a String member of Item containing the editable data */
	std::function< QString & ( Item & ) > editString;

	/** optional function that points the model to a bool flag of Item indicating whether the item is checked */
	std::function< bool & ( Item & ) > isChecked;

	/** optional path helper that will convert paths dropped from directory to absolute or relative */
	const PathHelper * pathHelper;

 public:

	EditableListModel( std::function< QString ( const Item & ) > makeDisplayString )
		: AListModel< Item >(), makeDisplayString( makeDisplayString ), pathHelper( nullptr ) {}

	EditableListModel( const QList< Item > & itemList, std::function< QString ( const Item & ) > makeDisplayString )
		: AListModel< Item >( itemList ), makeDisplayString( makeDisplayString ), pathHelper( nullptr ) {}

	//-- customization of how data will be represented -----------------------------------------------------------------

	/** defines how items should be edited and enables editing */
	void setEditStringFunc( std::function< QString & ( Item & ) > editString )
		{ this->editString = editString; }

	/** defines how checkbox state should be read and written and enables checkboxes for all items */
	void setIsCheckedFunc( std::function< bool & ( Item & ) > isChecked )
		{ this->isChecked = isChecked; }

	/** must be set before external drag&drop is enabled in the parent widget */
	void setPathHelper( const PathHelper * pathHelper )
		{ this->pathHelper = pathHelper; }

	//-- implementation of QAbstractItemModel's virtual methods --------------------------------------------------------

	virtual Qt::ItemFlags flags( const QModelIndex & index ) const override
	{
		if (!index.isValid())
			return Qt::ItemIsDropEnabled;  // otherwise you can't append dragged items to the end of the list

		Qt::ItemFlags flags = QAbstractListModel::flags(index);

		flags |= Qt::ItemIsDragEnabled;
		if (isSet( editString ))
			flags |= Qt::ItemIsEditable;
		if (isSet( isChecked ))
			flags |= Qt::ItemIsUserCheckable;

		return flags;
	}

	virtual QVariant data( const QModelIndex & index, int role ) const override
	{
		if (!index.isValid() || index.parent().isValid() || index.row() >= superClass::itemList.size())
			return QVariant();

		if (role == Qt::DisplayRole) {
			// This template class doesn't know about the structure of Item, it's supposed to be universal for any.
			// Therefore only author of Item knows which of its memebers he wants to display in the widget,
			// so he must specify it by a function.
			return makeDisplayString( superClass::itemList[ index.row() ] );
		} else if (role == Qt::EditRole) {
			// Same as above, exept that this function is optional to ease up initializations of models
			// that are supposed to be used in non-editable widgets
			if (!isSet( editString )) {
				qWarning() << "Edit has been requested, but no editString function is set. "
				              "Either specify an editString function or disable editing in the widget.";
				return QVariant();
			}
			return editString( const_cast< Item & >( superClass::itemList[ index.row() ] ) );
		} else if (role == Qt::CheckStateRole) {
			// Same as above, exept that this function is optional to ease up initializations of models
			// that are supposed to be used in non-checkable widgets
			if (!isSet( isChecked )) {
				return QVariant();
			}
			bool checked = isChecked( const_cast< Item & >( superClass::itemList[ index.row() ] ) );
			return checked ? Qt::Checked : Qt::Unchecked;
		} else {
			return QVariant();
		}
	}

	virtual bool setData( const QModelIndex & index, const QVariant & value, int role ) override
	{
		if (index.parent().isValid() || !index.isValid() || index.row() >= superClass::itemList.size())
			return false;

		if (role == Qt::EditRole) {
			if (!isSet( editString )) {
				qWarning() << "Edit has been requested, but no editString function is set. "
				              "Either specify an editString function or disable editing in the widget.";
				return false;
			}
			editString( superClass::itemList[ index.row() ] ) = value.toString();
			emit superClass::dataChanged( index, index, {Qt::EditRole} );
			return true;
		} else if (role == Qt::CheckStateRole) {
			if (!isSet( isChecked )) {
				qCritical() << "Attempted to change the check state, but no isChecked function is set. WTF?";
				return false;
			}
			isChecked( superClass::itemList[ index.row() ] ) = (value == Qt::Checked ? true : false );
			emit superClass::dataChanged( index, index, {Qt::CheckStateRole} );
			return true;
		} else {
			return false;
		}
	}

	virtual bool insertRows( int row, int count, const QModelIndex & parent ) override
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

	virtual bool removeRows( int row, int count, const QModelIndex & parent ) override
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

	virtual Qt::DropActions supportedDropActions() const override
	{
		return Qt::MoveAction | Qt::CopyAction;
	}

	static constexpr const char * const internalMimeType = "application/EditableListModel-internal";
	static constexpr const char * const urlMimeType = "text/uri-list";

	virtual QStringList mimeTypes() const override
	{
		QStringList types;

		types << internalMimeType;  // for internal drag&drop reordering
		types << urlMimeType;  // for drag&drop from directory window

		return types;
	}

	virtual bool canDropMimeData( const QMimeData * mime, Qt::DropAction action, int /*row*/, int /*col*/, const QModelIndex & ) const override
	{
		return (mime->hasFormat( internalMimeType ) && action == Qt::MoveAction) // for internal drag&drop reordering
		    || (mime->hasUrls());                                                // for drag&drop from other list list widgets
	}

	/// serializes items at <indexes> into MIME data
	virtual QMimeData * mimeData( const QModelIndexList & indexes ) const override
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
	virtual bool dropMimeData( const QMimeData * mime, Qt::DropAction action, int row, int, const QModelIndex & parent ) override
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
		if (!pathHelper) {
			qWarning() << "File has been dropped but no makeItemFromFile function is set. "
			              "Either specify a makeItemFromFile function or disable file dropping in the widget.";
			return false;
		}

		// first we need to know how many items will be inserted, so that we can allocate space for them
		QList< QFileInfo > filesToBeInserted;
		for (const QUrl & droppedUrl : urls) {
			QString localPath = droppedUrl.toLocalFile();
			if (!localPath.isEmpty()) {
				QFileInfo fileInfo( pathHelper->convertPath( localPath ) );
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
			superClass::itemList[ row + i ] = Item( filesToBeInserted[ i ] );
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


#endif // LIST_MODEL_INCLUDED
