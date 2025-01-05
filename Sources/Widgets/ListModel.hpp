//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: mediators between a list of arbitrary objects and list view or other widgets
//======================================================================================================================

#ifndef LIST_MODEL_INCLUDED
#define LIST_MODEL_INCLUDED


#include "Essential.hpp"

#include "Utils/ContainerUtils.hpp"  // PointerIterator
#include "Utils/FileSystemUtils.hpp"  // PathConvertor
#include "Utils/ErrorHandling.hpp"
#include "Themes.hpp"  // separator colors

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QRegularExpression>
#include <QColor>
#include <QBrush>
#include <QIcon>

#include <functional>
#include <stdexcept>


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
// support structs

#ifdef __GNUC__
 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"  // std::optional sometimes makes false positive warnings
#endif

/** Each item of ReadOnlyListModel must inherit from this struct to satisfy the requirements of the model.
  * The following methods should be overriden to point to the appropriate members. */
struct ReadOnlyListModelItem
{
	mutable std::optional< QColor > textColor;
	mutable std::optional< QColor > backgroundColor;
	bool isSeparator = false;  ///< true means this is a special item used to mark a section

	const QString & getFilePath() const
	{
		throw std::logic_error(
			"File path has been requested, but getting Item's file path is not implemented. "
			"Either re-implement getFilePath() or disable actions requiring path in the view."
		);
	}

	const QIcon & getIcon() const
	{
		throw std::logic_error(
			"Icon has been requested, but getting Item's icon is not implemented. "
			"Either re-implement getIcon() or disable icons in the view."
		);
	}
};

#ifdef __GNUC__
 #pragma GCC diagnostic pop
#endif

/** Each item of EditableListModel must inherit from this struct to satisfy the requirements of the model.
  * The following methods should be overriden to point to the appropriate members. */
struct EditableListModelItem : public ReadOnlyListModelItem
{
	bool isEditable() const
	{
		return false;
	}

	const QString & getEditString() const
	{
		throw std::logic_error(
			"Edit has been requested, but editing this Item is not implemented. "
			"Either re-implement getEditString() or disable editing in the view."
		);
	}

	void setEditString( QString /*str*/ )
	{
		throw std::logic_error(
			"Edit has been requested, but editing this Item is not implemented. "
			"Either re-implement setEditString() or disable editing in the view."
		);
	}

	bool isCheckable() const
	{
		return false;
	}

	bool isChecked() const
	{
		throw std::logic_error(
			"Check state has been requested, but checking this Item is not implemented. "
			"Either re-implement isChecked() or disable checkable items in the view."
		);
	}

	void setChecked( bool /*checked*/ ) const
	{
		throw std::logic_error(
			"Check state has been requested, but checking this Item is not implemented. "
			"Either re-implement setChecked() or disable checkable items in the view."
		);
	}
};


//======================================================================================================================
/// A trivial wrapper around QList.
/** One of possible list implementations for ListModel variants. */

template< typename Item_ >
class DirectList {

	QList< Item_ > _list;

 public:

	using Item = Item_;

	DirectList() {}
	DirectList( const QList< Item > & itemList ) : _list( itemList ) {}

	//-- wrapper functions for manipulating the list -------------------------------------------------------------------

	      auto & list()                              { return _list; }
	const auto & list() const                        { return _list; }
	void updateList( const QList< Item > &  list )   { _list = list; }
	void assignList(       QList< Item > && list )   { _list = std::move(list); }

	// content access

	using iterator = decltype( _list.begin() );
	using const_iterator = decltype( _list.cbegin() );

	auto count() const                               { return _list.count(); }
	auto size() const                                { return _list.size(); }
	auto isEmpty() const                             { return _list.isEmpty(); }

	      auto & operator[]( int idx )               { return _list[ idx ]; }
	const auto & operator[]( int idx ) const         { return _list[ idx ]; }

	      iterator begin()                           { return _list.begin(); }
	const_iterator begin() const                     { return _list.begin(); }
	      iterator end()                             { return _list.end(); }
	const_iterator end() const                       { return _list.end(); }

	      auto & first()                             { return _list.first(); }
	const auto & first() const                       { return _list.first(); }
	      auto & last()                              { return _list.last(); }
	const auto & last() const                        { return _list.last(); }

	// list modification

	void clear()                                     { _list.clear(); }

	void append( const Item &  item )                { _list.append( item ); }
	void append(       Item && item )                { _list.append( std::move(item) ); }
	void prepend( const Item &  item )               { _list.prepend( item ); }
	void prepend(       Item && item )               { _list.prepend( std::move(item) ); }
	void insert( int idx, const Item &  item )       { _list.insert( idx, item ); }
	void insert( int idx,       Item && item )       { _list.insert( idx, std::move(item) ); }

	void removeAt( int idx )                         { _list.removeAt( idx ); }
	void move( int from, int to )                    { _list.move( from, to ); }

	//-- special -------------------------------------------------------------------------------------------------------

	/// Whether the list modification functions can be safely called.
	bool canBeModified() const { return true; }

};


//======================================================================================================================
/// A wrapper around QList allowing to temporarily filter the content present only items matching a specified criteria.
/** One of possible list implementations for ListModel variants. */

template< typename Item_ >
class FilteredList {

	QList< Item_ > _fullList;
	QVector< Item_ * > _filteredList;

 public:

	using Item = Item_;

	FilteredList() {}
	FilteredList( const QList< Item > & itemList ) : _fullList( itemList ) { restore(); }

	//-- wrapper functions for manipulating the list -------------------------------------------------------------------

	      auto & fullList()                          { return _fullList; }
	const auto & fullList() const                    { return _fullList; }
	      auto & filteredList()                      { return _filteredList; }
	const auto & filteredList() const                { return _filteredList; }
	void updateList( const QList< Item > &  list )   { _fullList = list; restore(); }
	void assignList(       QList< Item > && list )   { _fullList = std::move(list); restore(); }

	// content access

	using iterator = PointerIterator< decltype( _filteredList.begin() ) >;
	using const_iterator = PointerIterator< decltype( _filteredList.cbegin() ) >;

	auto count() const                               { return _filteredList.count(); }
	auto size() const                                { return _filteredList.size(); }
	auto isEmpty() const                             { return _filteredList.isEmpty(); }

	      auto & operator[]( int idx )               { return *_filteredList[ idx ]; }
	const auto & operator[]( int idx ) const         { return *_filteredList[ idx ]; }

	      iterator begin()                           { return PointerIterator( _filteredList.begin() ); }
	const_iterator begin() const                     { return PointerIterator( _filteredList.begin() ); }
	      iterator end()                             { return PointerIterator( _filteredList.end() ); }
	const_iterator end() const                       { return PointerIterator( _filteredList.end() ); }

	      auto & first()                             { return *_filteredList.first(); }
	const auto & first() const                       { return *_filteredList.first(); }
	      auto & last()                              { return *_filteredList.last(); }
	const auto & last() const                        { return *_filteredList.last(); }

	// list modification - only when the list is not filtered

	void clear()
	{
		ensureCanBeModified();
		_filteredList.clear();
		_fullList.clear();
	}

	void append( const Item & item )
	{
		ensureCanBeModified();
		_fullList.append( item );
		_filteredList.append( &_fullList.last() );
	}
	void append( Item && item )
	{
		ensureCanBeModified();
		_fullList.append( std::move(item) );
		_filteredList.append( &_fullList.last() );
	}

	void prepend( const Item & item )
	{
		ensureCanBeModified();
		_fullList.prepend( item );
		_filteredList.prepend( &_fullList.first() );
	}
	void prepend( Item && item )
	{
		ensureCanBeModified();
		_fullList.prepend( std::move(item) );
		_filteredList.prepend( &_fullList.first() );
	}

	void insert( int idx, const Item & item )
	{
		ensureCanBeModified();
		_fullList.insert( idx, item );
		_filteredList.insert( idx, &_fullList[ idx ] );
	}
	void insert( int idx, Item && item )
	{
		ensureCanBeModified();
		_fullList.insert( idx, std::move(item) );
		_filteredList.insert( idx, &_fullList[ idx ] );
	}

	void removeAt( int idx )
	{
		if (!isFiltered())
		{
			_fullList.removeAt( idx );
			_filteredList.removeAt( idx );
		}
		else
		{
			// can be allowed for filtered list, but the fullList entry needs to be found and deleted too
			auto * ptr = _filteredList.takeAt( idx );
			for (int i = 0; i < _fullList.size(); ++i)
				if (&_fullList[i] == ptr)
					_fullList.removeAt( i );
		}
	}

	void move( int from, int to )
	{
		ensureCanBeModified();
		_fullList.move( from, to );
		_filteredList.move( from, to );
	}

	//-- searching/filtering -------------------------------------------------------------------------------------------

	/// Filters the list model entries to display only those that match a given criteria.
	void search( const QString & phrase, bool caseSensitive, bool useRegex )
	{
		_filteredList.clear();

		if (useRegex)
		{
			QRegularExpression regex( phrase );
			for (auto & item : _fullList)
				if (!item.isSeparator && regex.isValid() && regex.match( item.getEditString() ).hasMatch())
					_filteredList.append( &item );
		}
		else
		{
			Qt::CaseSensitivity caseSensitivity = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
			for (auto & item : _fullList)
				if (!item.isSeparator && item.getEditString().contains( phrase, caseSensitivity ))
					_filteredList.append( &item );
		}
	}

	/// Restores the list model to display the full unfiltered content.
	void restore()
	{
		_filteredList.clear();
		for (auto & item : _fullList)
			_filteredList.append( &item );
	}

	/// Whether the list is currently filtered or showing the full content.
	bool isFiltered() const
	{
		return _filteredList.size() != _fullList.size();
	}

	//-- special -------------------------------------------------------------------------------------------------------

	/// Whether the list modification functions can be safely called.
	/** This list cannot be modified when it is filtered. */
	bool canBeModified() const
	{
		return !isFiltered();
	}

 protected:

	void ensureCanBeModified()
	{
		if (!canBeModified())
		{
			logLogicError("FilteredList") << "the list cannot be modified when it is filtered";
			throw std::logic_error("the list cannot be modified when it is filtered");
		}
	}

};


//======================================================================================================================
/// Common functionality of all our list models.

class ListModelCommon : public QAbstractListModel, protected LoggingComponent {

 public:

	ListModelCommon() : LoggingComponent("ListModel") {}

	//-- model configuration -------------------------------------------------------------------------------------------

	void toggleIcons( bool enabled )  { iconsEnabled = enabled; }
	bool areIconsEnabled() const      { return iconsEnabled; }

	//-- data change notifications -------------------------------------------------------------------------------------

	/// Notifies the view that the content of some items has been changed.
	void contentChanged( int changedRowsBegin, int changedRowsEnd = -1 );

	// One of the following functions must always be called before and after doing any modifications to the list,
	// otherwise the list might not update correctly or it might even crash trying to access items that no longer exist.

	void orderAboutToChange()
	{
		emit layoutAboutToBeChanged( {}, LayoutChangeHint::VerticalSortHint );
	}
	void orderChanged()
	{
		emit layoutChanged( {}, LayoutChangeHint::VerticalSortHint );
	}

	void startAppending( int count = 1 )
	{
		beginInsertRows( QModelIndex(), this->rowCount(), this->rowCount() + count - 1 );
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

	//-- miscellaneous -------------------------------------------------------------------------------------------------

	QModelIndex makeIndex( int row )
	{
		return index( row, /*column*/0, /*parent*/QModelIndex() );
	}

 protected:

	bool iconsEnabled = false;

};


//======================================================================================================================
/// Wrapper around list of arbitrary objects, mediating their content to UI view elements with read-only access.

template< typename ListImpl >  // ListImpl::Item must be a subclass of ReadOnlyListModelItem
class ReadOnlyListModel : public ListModelCommon, public ListImpl {

 public:

	using Item = typename ListImpl::Item;

	ReadOnlyListModel( std::function< QString ( const Item & ) > makeDisplayString )
		: ListImpl(), makeDisplayString( makeDisplayString ) {}

	ReadOnlyListModel( const QList< Item > & itemList, std::function< QString ( const Item & ) > makeDisplayString )
		: ListImpl( itemList ), makeDisplayString( makeDisplayString ) {}

	//-- model configuration -------------------------------------------------------------------------------------------

	void setDisplayStringFunc( std::function< QString ( const Item & ) > makeDisplayString )
		{ this->makeDisplayString = makeDisplayString; }

	//-- implementation of QAbstractItemModel's virtual methods --------------------------------------------------------

	virtual int rowCount( const QModelIndex & = QModelIndex() ) const override
	{
		return this->size();
	}

	QVariant data( const QModelIndex & index, int role ) const override
	{
		if (!index.isValid() || index.row() >= this->size())
			return QVariant();

		const Item & item = (*this)[ index.row() ];

		try
		{
			if (role == Qt::DisplayRole)
			{
				// Some UI elements may want to display only the Item name, some others a string constructed from multiple
				// Item elements. This way we generalize from the way the display string is constructed from the Item.
				return makeDisplayString( item );
			}
			else if (role == Qt::ForegroundRole)
			{
				if (item.isSeparator)
					return QBrush( themes::getCurrentPalette().separatorText );
				else if (item.textColor)
					return QBrush( *item.textColor );
				else
					return QVariant();  // default
			}
			else if (role == Qt::BackgroundRole)
			{
				if (item.isSeparator)
					return QBrush( themes::getCurrentPalette().separatorBackground );
				else if (item.backgroundColor)
					return QBrush( *item.backgroundColor );
				else
					return QVariant();  // default
			}
			else if (role == Qt::TextAlignmentRole)
			{
				if (item.isSeparator)
					return Qt::AlignHCenter;
				else
					return QVariant();  // default
			}
			else if (role == Qt::DecorationRole && iconsEnabled && !item.isSeparator)
			{
				return item.getIcon();
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
		catch (const std::logic_error & e)
		{
			logLogicError() << e.what();
			return QVariant();
		}
	}

 protected: // configuration

	// Each list view might want to display the same data differently, so we allow the user of the list model
	// to specify it by a function for each view separately.
	/// function that takes Item and constructs a String that will be displayed in the view
	std::function< QString ( const Item & ) > makeDisplayString;

};


//======================================================================================================================
/// Wrapper around list of arbitrary objects, mediating their names to UI view elements.
/** Supports in-place editing, internal drag&drop reordering, and external file drag&drops. */

template< typename ListImpl >  // ListImpl::Item must be a subclass of EditableListModelItem
class EditableListModel : public ListModelCommon, public ListImpl, public DropTarget {

 public:

	using Item = typename ListImpl::Item;

	EditableListModel( std::function< QString ( const Item & ) > makeDisplayString )
		: ListImpl(), DropTarget(), makeDisplayString( makeDisplayString ), pathConvertor( nullptr ) {}

	EditableListModel( const QList< Item > & itemList, std::function< QString ( const Item & ) > makeDisplayString )
		: ListImpl( itemList ), DropTarget(), makeDisplayString( makeDisplayString ), pathConvertor( nullptr ) {}


	//-- customization of how data will be represented -----------------------------------------------------------------

	void setDisplayStringFunc( std::function< QString ( const Item & ) > makeDisplayString )
		{ this->makeDisplayString = makeDisplayString; }

	void toggleIcons( bool enabled ) { iconsEnabled = enabled; }

	void toggleEditing( bool enabled ) { editingEnabled = enabled; }

	void toggleCheckableItems( bool enabled ) { checkableItems = enabled; }

	/** Must be set before external drag&drop is enabled in the parent widget. */
	void setPathContext( const PathConvertor * pathConvertor ) { this->pathConvertor = pathConvertor; }


	//-- helpers -------------------------------------------------------------------------------------------------------

	bool canBeEdited( const Item & item ) const
	{
		return (editingEnabled && item.isEditable()) || item.isSeparator;
	}

	bool canBeChecked( const Item & item ) const
	{
		return (checkableItems && item.isCheckable()) && !item.isSeparator;
	}

	bool canHaveIcon( const Item & item ) const
	{
		return iconsEnabled && !item.isSeparator;
	}


	//-- implementation of QAbstractItemModel's virtual methods --------------------------------------------------------

	virtual int rowCount( const QModelIndex & = QModelIndex() ) const override
	{
		return this->size();
	}

	virtual Qt::ItemFlags flags( const QModelIndex & index ) const override
	{
		if (!index.isValid())
			return Qt::ItemIsDropEnabled;  // otherwise you can't append dragged items to the end of the list

		const Item & item = (*this)[ index.row() ];

		Qt::ItemFlags flags = QAbstractListModel::flags( index );

		flags |= Qt::ItemIsDragEnabled;
		if (canBeEdited( item ))
			flags |= Qt::ItemIsEditable;
		if (canBeChecked( item ))
			flags |= Qt::ItemIsUserCheckable;

		return flags;
	}

	virtual QVariant data( const QModelIndex & index, int role ) const override
	{
		if (!index.isValid() || index.parent().isValid() || index.row() >= this->size())
			return QVariant();

		const Item & item = (*this)[ index.row() ];

		try
		{
			if (role == Qt::DisplayRole)
			{
				// Each list view might want to display the same data differently, so we allow the user of the list model
				// to specify it by a function for each view separately.
				return makeDisplayString( item );
			}
			else if (role == Qt::EditRole && canBeEdited( item ))
			{
				return item.getEditString();
			}
			else if (role == Qt::CheckStateRole && canBeChecked( item ))
			{
				return item.isChecked() ? Qt::Checked : Qt::Unchecked;
			}
			else if (role == Qt::ForegroundRole)
			{
				if (item.isSeparator)
					return QBrush( themes::getCurrentPalette().separatorText );
				else if (item.textColor)
					return QBrush( *item.textColor );
				else
					return QVariant();  // default
			}
			else if (role == Qt::BackgroundRole)
			{
				if (item.isSeparator)
					return QBrush( themes::getCurrentPalette().separatorBackground );
				else if (item.backgroundColor)
					return QBrush( *item.backgroundColor );
				else
					return QVariant();  // default
			}
			else if (role == Qt::TextAlignmentRole)
			{
				if (item.isSeparator)
					return Qt::AlignHCenter;
				else
					return QVariant();  // default
			}
			else if (role == Qt::DecorationRole && canHaveIcon( item ))
			{
				return item.getIcon();
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
		catch (const std::logic_error & e)
		{
			logLogicError() << e.what();
			return QVariant();
		}
	}

	virtual bool setData( const QModelIndex & index, const QVariant & value, int role ) override
	{
		if (index.parent().isValid() || !index.isValid() || index.row() >= this->size())
			return false;

		Item & item = (*this)[ index.row() ];

		try
		{
			if (role == Qt::EditRole && canBeEdited( item ))
			{
				item.setEditString( value.toString() );
				emit QAbstractListModel::dataChanged( index, index, {Qt::EditRole} );
				return true;
			}
			else if (role == Qt::CheckStateRole && canBeChecked( item ))
			{
				item.setChecked( value == Qt::Checked ? true : false );
				emit QAbstractListModel::dataChanged( index, index, {Qt::CheckStateRole} );
				return true;
			}
			else
			{
				return false;
			}
		}
		catch (const std::logic_error & e)
		{
			logLogicError() << e.what();
			return false;
		}
	}

	virtual bool insertRows( int row, int count, const QModelIndex & parent ) override
	{
		if (parent.isValid() || row < 0)
			return false;

		if (!this->canBeModified())
		{
			logLogicError() << "Cannot insertRows into this model now. It should have been restricted by the ListView.";
			return false;
		}

		QAbstractListModel::beginInsertRows( parent, row, row + count - 1 );

		// n times moving all the elements forward to insert one is not nice
		// but it happens only once in awhile and the number of elements is almost always very low
		for (int i = 0; i < count; i++)
			this->insert( row + i, Item() );

		QAbstractListModel::endInsertRows();

		return true;
	}

	virtual bool removeRows( int row, int count, const QModelIndex & parent ) override
	{
		if (parent.isValid() || row < 0 || row + count > this->size())
			return false;

		if (!this->canBeModified())
		{
			logLogicError() << "Cannot removeRows from this model now. It should have been restricted by the ListView.";
			return false;
		}

		QAbstractListModel::beginRemoveRows( parent, row, row + count - 1 );

		// n times moving all the elements backward to insert one is not nice
		// but it happens only once in awhile and the number of elements is almost always very low
		for (int i = 0; i < count; i++)
		{
			this->removeAt( row );
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
		types << urlMimeType;  // for drag&drop from a file explorer window

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
		if (row < 0 || row > this->size())
			row = this->size();

		if (!this->canBeModified())
		{
			logLogicError() << "Cannot drop into this model now. It should have been restricted by the ListView.";
			return false;
		}

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
			logLogicError() << "This model doesn't support such drop operation. It should have been restricted by the ListView.";
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
		//
		// We could probably avoid even the copy/move constructors and the allocation when inserting an item into QList,
		// but we would need a direct access to the pointers inside QList which it doesn't provide.
		// Alternativelly the QList<Item> could be replaced with QVector<Item*> or QVector<unique_ptr<Item>>
		// but that would just complicate a lot of other things.

		QVector< int > origItemIndexes;
		for (int i = 0; i < count; i++)
			origItemIndexes.append( rawData[i] );

		// The indexes of selected items can come in arbitrary order, but we need to drop them in ascending order.
		std::sort( origItemIndexes.begin(), origItemIndexes.end() );

		QVector< Item * > origItemRefs;
		for (int origItemIdx : origItemIndexes)
		{
			origItemRefs.append( &(*this)[ origItemIdx ] );
		}

		// allocate space for the items to move to
		if (!insertRows( row, count, parent ))
		{
			return false;
		}

		// move the original items to the target position
		for (int i = 0; i < count; i++)
		{
			(*this)[ row + i ] = std::move( *origItemRefs[i] );
		}

		// idiotic workaround because Qt is fucking retarded   (read the comment at the top of EditableListView.cpp)
		//
		// note down the destination drop index, so it can be later retrieved by ListView
		DropTarget::itemsDropped( row, count );

		return true;
	}

	bool dropMimeUrls( QList< QUrl > urls, int row, const QModelIndex & parent )
	{
		if (!pathConvertor)
		{
			logLogicError() << "File has been dropped but no PathConvertor is set. "
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
				QFileInfo fileInfo( pathConvertor->convertPath( localPath ) );
				filesToBeInserted.append( std::move(fileInfo) );
			}
		}

		// allocate space for the items to be dropped to
		if (!insertRows( row, filesToBeInserted.count(), parent ))
		{
			return false;
		}

		for (int i = 0; i < filesToBeInserted.count(); i++)
		{
			// This template class doesn't know about the structure of Item, it's supposed to be universal for any.
			// Therefore only author of Item knows how to assign a dropped file into it, so he must define it by a constructor.
			(*this)[ row + i ] = Item( filesToBeInserted[ i ] );
		}

		// idiotic workaround because Qt is fucking retarded   (read the comment at the top of EditableListView.cpp)
		//
		// note down the destination drop index, so it can be later retrieved by ListView
		DropTarget::itemsDropped( row, filesToBeInserted.count() );

		return true;
	}

 protected: // configuration

	// Each list view might want to display the same data differently, so we allow the user of the list model
	// to specify it by a function for each view separately.
	/// function that takes Item and constructs a String that will be displayed in the view
	std::function< QString ( const Item & ) > makeDisplayString;

	/// whether editing of regular non-separator items is allowed
	bool editingEnabled = false;

	/// whether items have a checkbox that can be checked and unchecked
	bool checkableItems = false;

	/// optional path helper that will convert paths dropped from directory to absolute or relative
	const PathConvertor * pathConvertor;

};


//======================================================================================================================
// aliases

template< typename Item > using ReadOnlyDirectListModel   = ReadOnlyListModel< DirectList< Item > >;
template< typename Item > using ReadOnlyFilteredListModel = ReadOnlyListModel< FilteredList< Item > >;
template< typename Item > using EditableDirectListModel   = EditableListModel< DirectList< Item > >;
template< typename Item > using EditableFilteredListModel = EditableListModel< FilteredList< Item > >;


#endif // LIST_MODEL_INCLUDED
