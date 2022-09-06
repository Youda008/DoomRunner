//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
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
#include <QBrush>

#include <functional>
#include <stdexcept>

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
// idiotic workaround because Qt is fucking retarded   (read the comment at the top of EditableListView.cpp)
//
// This non-template superclass exists because in EditableListView we don't know the template parameter Item,
// which is needed for casting in order to retrieve the destination drop index.

class DropTarget {

 public:

	DropTarget() : _dropped( false ), _droppedRow( 0 ), _droppedCount( 0 ) {}

	bool wasDroppedInto() const { return _dropped; }
	int droppedRow() const { return _droppedRow; }
	int droppedCount() const { return _droppedCount; }

	void resetDropState() { _dropped = false; }

 protected:

	void itemsDropped( int row, int count )
	{
		_dropped = true;
		_droppedRow = row;
		_droppedCount = count;
	}

	void decrementRow()
	{
		_droppedRow--;
	}

 private:

	bool _dropped;
	int _droppedRow;
	int _droppedCount;

};


//======================================================================================================================
//  support structs

/** Each item of ReadOnlyListModel must inherit from this struct to satisfy the requirements of the model.
  * The following methods should be overriden to point to the appropriate members. */
struct ReadOnlyListModelItem
{
	bool isSeparator = false;  ///< true means this is a special item used to mark a section

	const QString & getFilePath() const
	{
		throw std::runtime_error(
			"File path has been requested, but getting Item's file path is not implemented. "
			"Either re-implement getFilePath() or disable actions requiring path in the view."
		);
	}
};

/** Each item of EditableListModel must inherit from this struct to satisfy the requirements of the model.
  * The following methods should be overriden to point to the appropriate members. */
struct EditableListModelItem : public ReadOnlyListModelItem
{
	const QString & getEditString() const
	{
		throw std::runtime_error(
			"Edit has been requested, but editing this Item is not implemented. "
			"Either re-implement getEditString() or disable editing in the view."
		);
	}

	void setEditString( const QString & /*str*/ )
	{
		throw std::runtime_error(
			"Edit has been requested, but editing this Item is not implemented. "
			"Either re-implement setEditString() or disable editing in the view."
		);
	}

	bool isChecked() const
	{
		throw std::runtime_error(
			"Check state has been requested, but checking this Item is not implemented. "
			"Either re-implement isChecked() or disable checkable items in the view."
		);
	}

	void setChecked( bool /*checked*/ ) const
	{
		throw std::runtime_error(
			"Check state has been requested, but checking this Item is not implemented. "
			"Either re-implement setChecked() or disable checkable items in the view."
		);
	}
};


//======================================================================================================================
/** Abstract wrapper around list of arbitrary objects, mediating their content to UI view elements. */

template< typename Item >
class AListModel : public QAbstractListModel {

 protected:

	QList< Item > itemList;

 public:

	AListModel() {}
	AListModel( const QList< Item > & itemList ) : itemList( itemList ) {}

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
	decltype( itemList.cbegin() ) begin() const   { return itemList.begin(); }
	decltype( itemList.end() ) end()              { return itemList.end(); }
	decltype( itemList.cend() ) end() const       { return itemList.end(); }
	void clear()                                  { itemList.clear(); }
	void append( const Item & item )              { itemList.append( item ); }
	void prepend( const Item & item )             { itemList.prepend( item ); }
	void insert( int idx, const Item & item )     { itemList.insert( idx, item ); }
	void removeAt( int idx )                      { itemList.removeAt( idx ); }
	void move( int from, int to )                 { itemList.move( from, to ); }
	int indexOf( const Item & item ) const        { return itemList.indexOf( item ); }

	//-- data change notifications -------------------------------------------------------------------------------------

	/// Notifies the view that the content of some items has been changed.
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
		beginInsertRows( QModelIndex(), itemList.size(), itemList.size() + count - 1 );
	}
	void finishAppending()
	{
		endInsertRows();
	}

	void startInserting( int row )
	{
		beginInsertRows( QModelIndex(), row, row );
	}
	void finishInserting()
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

template< typename Item >  // must be a subclass of ReadOnlyListModelItem
class ReadOnlyListModel : public AListModel< Item > {

	using superClass = AListModel< Item >;

 protected:

	// Each list view might want to display the same data differently, so we allow the user of the list model
	// to specify it by a function for each view separately.
	/// function that takes Item and constructs a String that will be displayed in the view
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

		const Item & item = superClass::itemList[ index.row() ];

		if (role == Qt::DisplayRole)
		{
			// Some UI elements may want to display only the Item name, some others a string constructed from multiple
			// Item elements. This way we generalize from the way the display string is constructed from the Item.
			return makeDisplayString( item );
		}
		else
		{
			return QVariant();
		}
	}

};


//======================================================================================================================
/** Wrapper around list of arbitrary objects, mediating their names to UI view elements.
  * Supports in-place editing, internal drag&drop reordering, and external file drag&drops. */

template< typename Item >  // must be a subclass of EditableListModelItem
class EditableListModel : public AListModel< Item >, public DropTarget {

	using superClass = AListModel< Item >;

 protected:

	// Each list view might want to display the same data differently, so we allow the user of the list model
	// to specify it by a function for each view separately.
	/// function that takes Item and constructs a String that will be displayed in the view
	std::function< QString ( const Item & ) > makeDisplayString;

	/// whether editing of regular non-separator items is allowed
	bool editingEnabled = false;

	/// whether separators are allowed
	bool separatorsEnabled = false;

	/// whether items have a checkbox that can be checked and unchecked
	bool checkableItems = false;

	/// optional path helper that will convert paths dropped from directory to absolute or relative
	const PathContext * pathContext;

 public:

	EditableListModel( std::function< QString ( const Item & ) > makeDisplayString )
		: AListModel< Item >(), DropTarget(), makeDisplayString( makeDisplayString ), pathContext( nullptr ) {}

	EditableListModel( const QList< Item > & itemList, std::function< QString ( const Item & ) > makeDisplayString )
		: AListModel< Item >( itemList ), DropTarget(), makeDisplayString( makeDisplayString ), pathContext( nullptr ) {}


	//-- customization of how data will be represented -----------------------------------------------------------------

	void setDisplayStringFunc( std::function< QString ( const Item & ) > makeDisplayString )
		{ this->makeDisplayString = makeDisplayString; }

	void toggleEditing( bool enabled ) { editingEnabled = enabled; }

	void toggleSeparators( bool enabled ) { separatorsEnabled = enabled; }

	void toggleCheckableItems( bool enabled ) { checkableItems = enabled; }

	/** Must be set before external drag&drop is enabled in the parent widget. */
	void setPathContext( const PathContext * pathContext ) { this->pathContext = pathContext; }


	//-- implementation of QAbstractItemModel's virtual methods --------------------------------------------------------

	virtual Qt::ItemFlags flags( const QModelIndex & index ) const override
	{
		if (!index.isValid())
			return Qt::ItemIsDropEnabled;  // otherwise you can't append dragged items to the end of the list

		const Item & item = superClass::itemList[ index.row() ];
		bool isSeparator = separatorsEnabled && item.isSeparator;

		Qt::ItemFlags flags = QAbstractListModel::flags(index);

		flags |= Qt::ItemIsDragEnabled;
		if (editingEnabled || isSeparator)
			flags |= Qt::ItemIsEditable;
		if (checkableItems && !isSeparator)
			flags |= Qt::ItemIsUserCheckable;

		return flags;
	}

	virtual QVariant data( const QModelIndex & index, int role ) const override
	{
		if (!index.isValid() || index.parent().isValid() || index.row() >= superClass::itemList.size())
			return QVariant();

		const Item & item = superClass::itemList[ index.row() ];
		bool isSeparator = separatorsEnabled && item.isSeparator;

		try
		{
			if (role == Qt::DisplayRole)
			{
				// Each list view might want to display the same data differently, so we allow the user of the list model
				// to specify it by a function for each view separately.
				return makeDisplayString( item );
			}
			else if (role == Qt::EditRole && (editingEnabled || isSeparator))
			{
				return item.getEditString();
			}
			else if (role == Qt::CheckStateRole && checkableItems)
			{
				return item.isChecked() ? Qt::Checked : Qt::Unchecked;
			}
			else if (role == Qt::BackgroundRole && separatorsEnabled)
			{
				if (isSeparator)
					return QBrush( Qt::lightGray );
				else
					return QVariant();  // default
			}
			else if (role == Qt::TextAlignmentRole && separatorsEnabled)
			{
				if (isSeparator)
					return Qt::AlignHCenter;
				else
					return QVariant();  // default
			}
			else if (role == Qt::UserRole)  // required for "Open File Location" action
			{
				return item.getFilePath();
			}
			else
			{
				return QVariant();
			}
		}
		catch (const std::runtime_error & e)
		{
			qWarning() << e.what();
			return QVariant();
		}
	}

	virtual bool setData( const QModelIndex & index, const QVariant & value, int role ) override
	{
		if (index.parent().isValid() || !index.isValid() || index.row() >= superClass::itemList.size())
			return false;

		Item & item = superClass::itemList[ index.row() ];
		bool isSeparator = separatorsEnabled && item.isSeparator;

		try
		{
			if (role == Qt::EditRole && (editingEnabled || isSeparator))
			{
				item.setEditString( value.toString() );
				emit superClass::dataChanged( index, index, {Qt::EditRole} );
				return true;
			}
			else if (role == Qt::CheckStateRole && checkableItems)
			{
				item.setChecked( value == Qt::Checked ? true : false );
				emit superClass::dataChanged( index, index, {Qt::CheckStateRole} );
				return true;
			}
			else
			{
				return false;
			}
		}
		catch (const std::runtime_error & e)
		{
			qWarning() << e.what();
			return false;
		}
	}

	virtual bool insertRows( int row, int count, const QModelIndex & parent ) override
	{
		if (parent.isValid())
			return false;

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
		if (parent.isValid() || row < 0 || row + count > superClass::itemList.size())
			return false;

		QAbstractListModel::beginRemoveRows( parent, row, row + count - 1 );

		// n times moving all the elements backward to insert one is not nice
		// but it happens only once in awhile and the number of elements is almost always very low
		for (int i = 0; i < count; i++)
		{
			superClass::itemList.removeAt( row );
			if (row < DropTarget::droppedRow())  // we are removing a row that is before the target row
				DropTarget::decrementRow();      // so target row's index is moving backwards
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

		for (const QModelIndex & index : indexes)
		{
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

		if (mime->hasFormat( internalMimeType ) && action == Qt::MoveAction)
		{
			return dropInternalItems( mime->data( internalMimeType ), row, parent );
		}
		else if (mime->hasUrls())
		{
			return dropMimeUrls( mime->urls(), row, parent );
		}
		else
		{
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
		for (int i = 0; i < count; i++)
		{
			int origRowIdx = rawData[i];
			origItemRefs.append( &superClass::itemList[ origRowIdx ] );
		}

		// allocate space for the items to move to
		insertRows( row, count, parent );

		// move the original items to the target position
		for (int i = 0; i < count; i++)
		{
			superClass::itemList[ row + i ] = std::move( *origItemRefs[i] );
		}

		// idiotic workaround because Qt is fucking retarded   (read the comment at the top of EditableListView.cpp)
		//
		// note down the destination drop index, so it can be later retrieved by ListView
		DropTarget::itemsDropped( row, count );

		return true;
	}

	bool dropMimeUrls( QList< QUrl > urls, int row, const QModelIndex & parent )
	{
		if (!pathContext)
		{
			qWarning() << "File has been dropped but no PathContext is set. "
			              "Either use setPathContext or disable file dropping in the widget.";
			return false;
		}

		// first we need to know how many items will be inserted, so that we can allocate space for them
		QList< QFileInfo > filesToBeInserted;
		for (const QUrl & droppedUrl : urls)
		{
			QString localPath = droppedUrl.toLocalFile();
			if (!localPath.isEmpty())
			{
				QFileInfo fileInfo( pathContext->convertPath( localPath ) );
				if (fileInfo.exists())
				{
					filesToBeInserted.append( fileInfo );
				}
			}
		}

		// allocate space for the items to be dropped to
		insertRows( row, filesToBeInserted.count(), parent );

		for (int i = 0; i < filesToBeInserted.count(); i++)
		{
			// This template class doesn't know about the structure of Item, it's supposed to be universal for any.
			// Therefore only author of Item knows how to assign a dropped file into it, so he must define it by a constructor.
			superClass::itemList[ row + i ] = Item( filesToBeInserted[ i ] );
		}

		// idiotic workaround because Qt is fucking retarded   (read the comment at the top of EditableListView.cpp)
		//
		// note down the destination drop index, so it can be later retrieved by ListView
		DropTarget::itemsDropped( row, filesToBeInserted.count() );

		return true;
	}

};


#endif // LIST_MODEL_INCLUDED
