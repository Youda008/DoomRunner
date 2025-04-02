//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: mediators between a list of arbitrary objects and list view or other widgets
//======================================================================================================================

#ifndef LIST_MODEL_INCLUDED
#define LIST_MODEL_INCLUDED


#include "Essential.hpp"

#include "CommonTypes.hpp"  // PtrList, DerefIterator
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
#include <vector>
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

// The following structs have dummy method declarations to denote what the model classes expect.
//
// Virtual methods (runtime polymorphism) is not needed here, because the item type is a template parameter
// of the list model so the type is always fully known.
//
// However, some of the methods must have an dummy implementation as well, because the model/view properties
// are not known at compile time, therefore the compiler expects them to always be there, even if they are not used.

#ifdef __GNUC__
 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"  // std::optional sometimes makes false positive warnings
#endif

/** Every item type of ReadOnlyListModel must inherit from this struct to satisfy the requirements of the model.
  * The following methods should be overriden to point to the appropriate members. */
struct ReadOnlyListModelItem
{
	mutable std::optional< QColor > textColor;
	mutable std::optional< QColor > backgroundColor;
	bool isSeparator = false;  ///< true means this is a special item used to mark a section

	// Should return an ID of this item that's unique within the list. Used for remembering selected items.
	const QString & getID() const;

	// Used for special purposes such as "Open File Location" action. Must be overriden when such action is enabled.
	const QString & getFilePath() const
	{
		throw std::logic_error(
			"File path has been requested, but getting Item's file path is not implemented. "
			"Either re-implement getFilePath() or disable actions requiring path in the view."
		);
	}

	// When icons are enabled, this must return the icon for this particular item.
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

	// When the model is set up to be editable, this must return the text to be edited in the view.
	const QString & getEditString() const
	{
		throw std::logic_error(
			"Edit has been requested, but editing this Item is not implemented. "
			"Either re-implement getEditString() or disable editing in the view."
		);
	}

	// When the model is set up to be editable, this must apply the user edit from the view.
	void setEditString( QString /*str*/ )
	{
		throw std::logic_error(
			"Edit has been requested, but editing this Item is not implemented. "
			"Either re-implement setEditString() or disable editing in the view."
		);
	}

	// Whether this item has an active checkbox in the view.
	bool isCheckable() const
	{
		return false;
	}

	// When the model is set up to have checkboxes, this must return whether the checkbox should be displayed as checked.
	bool isChecked() const
	{
		throw std::logic_error(
			"Check state has been requested, but checking this Item is not implemented. "
			"Either re-implement isChecked() or disable checkable items in the view."
		);
	}

	// When the model is set up to have checkboxes, this must apply the new status of the checkbox.
	void setChecked( bool /*checked*/ ) const
	{
		throw std::logic_error(
			"Check state has been requested, but checking this Item is not implemented. "
			"Either re-implement setChecked() or disable checkable items in the view."
		);
	}
};


//======================================================================================================================
/// A trivial wrapper around PtrList.
/** One of possible list implementations for ListModel variants. */

template< typename Item_ >
class DirectList {

	PtrList< Item_ > _list;

 public:

	using Item = Item_;

	DirectList() {}
	DirectList( const PtrList< Item > & itemList ) : _list( itemList ) {}

	//-- wrapper functions for manipulating the list -------------------------------------------------------------------

	      auto & list()                                { return _list; }
	const auto & list() const                          { return _list; }
	void updateList( const PtrList< Item > &  list )   { _list = list; }
	void assignList(       PtrList< Item > && list )   { _list = std::move(list); }

	// content access

	using iterator = typename decltype( _list )::iterator;
	using const_iterator = typename decltype( _list )::const_iterator;

	auto count() const                                 { return _list.count(); }
	auto size() const                                  { return _list.size(); }
	auto isEmpty() const                               { return _list.isEmpty(); }

	      auto & operator[]( qsizetype idx )           { return _list[ idx ]; }
	const auto & operator[]( qsizetype idx ) const     { return _list[ idx ]; }

	      iterator begin()                             { return _list.begin(); }
	const_iterator begin() const                       { return _list.begin(); }
	      iterator end()                               { return _list.end(); }
	const_iterator end() const                         { return _list.end(); }

	      auto & first()                               { return _list.first(); }
	const auto & first() const                         { return _list.first(); }
	      auto & last()                                { return _list.last(); }
	const auto & last() const                          { return _list.last(); }

	// list modification

	void clear()                                       { _list.clear(); }

	void append( const Item &  item )                  { _list.append( item ); }
	void append(       Item && item )                  { _list.append( std::move(item) ); }
	void prepend( const Item &  item )                 { _list.prepend( item ); }
	void prepend(       Item && item )                 { _list.prepend( std::move(item) ); }
	void insert( qsizetype idx, const Item &  item )   { _list.insert( idx, item ); }
	void insert( qsizetype idx,       Item && item )   { _list.insert( idx, std::move(item) ); }

	void removeAt( qsizetype idx )                     { _list.removeAt( idx ); }

	void move( qsizetype from, qsizetype to )          { _list.move( from, to ); }
	void moveToFront( qsizetype from )                 { _list.move( from, 0 ); }
	void moveToBack( qsizetype from )                  { _list.move( from, size() - 1 ); }

	//-- custom access helpers -----------------------------------------------------------------------------------------

	// sorting

	template< typename IsLessThan,
		std::enable_if_t< std::is_invocable_v< IsLessThan, const Item &, const Item & >, int > = 0 >
	void sortBy( const IsLessThan & isLessThan )
	{
		std::sort( begin(), end(), isLessThan );
	}

	void sortByID()
	{
		sortBy( []( const Item & i1, const Item & i2 ) { return i1.getID() < i2.getID(); } );
	}

	// low-level pointer manipulation for implementing optimized high-level operations

	bool isNull( qsizetype idx ) const                            { return _list.isNull( idx ); }
	std::unique_ptr< Item > takePtr( qsizetype idx )              { return _list.takePtr( idx ); }
	void removePtr( qsizetype idx )                               { return _list.removePtr( idx ); }
	void insertPtr( qsizetype idx, std::unique_ptr< Item > ptr )  { _list.insertPtr( idx, std::move(ptr) ); }

	//-- special -------------------------------------------------------------------------------------------------------

	/// Whether the list modification functions can be safely called.
	bool canBeModified() const { return true; }

};


//======================================================================================================================
/// A wrapper around PtrList allowing to temporarily filter the content present only items matching a specified criteria.
/** One of possible list implementations for ListModel variants. */

template< typename Item_ >
class FilteredList {

	PtrList< Item_ > _fullList;
	QList< Item_ * > _filteredList;

 public:

	using Item = Item_;

	FilteredList() {}
	FilteredList( const PtrList< Item > & itemList ) : _fullList( itemList ) { restore(); }

	//-- wrapper functions for manipulating the list -------------------------------------------------------------------

	      auto & fullList()                            { return _fullList; }
	const auto & fullList() const                      { return _fullList; }
	      auto & filteredList()                        { return _filteredList; }
	const auto & filteredList() const                  { return _filteredList; }
	void updateList( const PtrList< Item > &  list )   { _fullList = list; restore(); }
	void assignList(       PtrList< Item > && list )   { _fullList = std::move(list); restore(); }

	// content access

	using iterator = DerefIterator< typename decltype( _filteredList )::iterator >;
	using const_iterator = DerefIterator< typename decltype( _filteredList )::const_iterator >;

	auto count() const                                 { return _filteredList.count(); }
	auto size() const                                  { return _filteredList.size(); }
	auto isEmpty() const                               { return _filteredList.isEmpty(); }

	      auto & operator[]( qsizetype idx )           { return *_filteredList[ idx ]; }
	const auto & operator[]( qsizetype idx ) const     { return *_filteredList[ idx ]; }

	      iterator begin()                             { return DerefIterator( _filteredList.begin() ); }
	const_iterator begin() const                       { return DerefIterator( _filteredList.begin() ); }
	      iterator end()                               { return DerefIterator( _filteredList.end() ); }
	const_iterator end() const                         { return DerefIterator( _filteredList.end() ); }

	      auto & first()                               { return *_filteredList.first(); }
	const auto & first() const                         { return *_filteredList.first(); }
	      auto & last()                                { return *_filteredList.last(); }
	const auto & last() const                          { return *_filteredList.last(); }

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

	void insert( qsizetype idx, const Item & item )
	{
		ensureCanBeModified();
		_fullList.insert( idx, item );
		_filteredList.insert( idx, &_fullList[ idx ] );
	}
	void insert( qsizetype idx, Item && item )
	{
		ensureCanBeModified();
		_fullList.insert( idx, std::move(item) );
		_filteredList.insert( idx, &_fullList[ idx ] );
	}

	void removeAt( qsizetype idx )
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
			for (qsizetype i = 0; i < _fullList.size(); ++i)
				if (&_fullList[i] == ptr)
					_fullList.removeAt( i );
		}
	}

	void move( qsizetype from, qsizetype to )
	{
		ensureCanBeModified();
		_fullList.move( from, to );
		_filteredList.move( from, to );
	}

	void moveToFront( qsizetype from )
	{
		ensureCanBeModified();
		_fullList.move( from, 0 );
		_filteredList.move( from, 0 );
	}

	void moveToBack( qsizetype from )
	{
		ensureCanBeModified();
		_fullList.move( from, size() - 1 );
		_filteredList.move( from, size() - 1 );
	}

	//-- custom access helpers -----------------------------------------------------------------------------------------

	// sorting

	template< typename IsLessThan,
		std::enable_if_t< std::is_invocable_v< IsLessThan, const Item &, const Item & >, int > = 0 >
	void sortBy( const IsLessThan & isLessThan )
	{
		std::sort( begin(), end(), isLessThan );
	}

	void sortByID()
	{
		sortBy( []( const Item & i1, const Item & i2 ) { return i1.getID() < i2.getID(); } );
	}

	// low-level pointer manipulation for implementing optimized high-level operations

	bool isNull( qsizetype idx ) const
	{
		return _fullList.isNull( idx );
	}

	std::unique_ptr< Item > takePtr( qsizetype idx )
	{
		ensureCanBeModified();
		_filteredList[ idx ] = nullptr;
		return _fullList.takePtr( idx );
	}

	void removePtr( qsizetype idx )
	{
		ensureCanBeModified();
		_fullList.removePtr( idx );
		_filteredList.removeAt( idx );
	}

	void insertPtr( qsizetype idx, std::unique_ptr< Item > ptr )
	{
		ensureCanBeModified();
		_fullList.insertPtr( idx, std::move(ptr) );
		_filteredList.insert( idx, &_fullList[ idx ] );
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
		movingInProgress = true;
		emit layoutAboutToBeChanged( {}, LayoutChangeHint::VerticalSortHint );
	}
	void orderChanged()
	{
		movingInProgress = false;
		emit layoutChanged( {}, LayoutChangeHint::VerticalSortHint );
	}

	void startAppending( int count = 1 )
	{
		movingInProgress = true;
		beginInsertRows( QModelIndex(), this->rowCount(), this->rowCount() + count - 1 );
	}
	void finishAppending()
	{
		movingInProgress = false;
		endInsertRows();
	}

	void startInserting( int row, int count = 1 )
	{
		movingInProgress = true;
		beginInsertRows( QModelIndex(), row, row + count - 1 );
	}
	void finishInserting()
	{
		movingInProgress = false;
		endInsertRows();
	}

	void startDeleting( int row, int count = 1 )
	{
		movingInProgress = true;
		beginRemoveRows( QModelIndex(), row, row + count - 1 );
	}
	void finishDeleting()
	{
		movingInProgress = false;
		endRemoveRows();
	}

	void startCompleteUpdate()
	{
		movingInProgress = true;
		beginResetModel();
	}
	void finishCompleteUpdate()
	{
		movingInProgress = false;
		endResetModel();
	}

	//-- miscellaneous -------------------------------------------------------------------------------------------------

	QModelIndex makeIndex( int row )
	{
		return index( row, /*column*/0, /*parent*/QModelIndex() );
	}

	// Optimization to prevent updating some things when we know they are going to change again right away.
	bool isMovingInProgress() const
	{
		return movingInProgress;
	}

	void setMovingInProgress( bool val )
	{
		movingInProgress = val;
	}

 protected:

	bool iconsEnabled = false;
	bool movingInProgress = false;

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

	EditableListModel( const PtrList< Item > & itemList, std::function< QString ( const Item & ) > makeDisplayString )
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
		return int( this->size() );
	}

	virtual Qt::ItemFlags flags( const QModelIndex & index ) const override
	{
		if (!index.isValid() || index.row() >= this->size())
			return Qt::ItemIsDropEnabled;  // otherwise you can't append dragged items to the end of the list

		// On some OSes Qt calls flags inside beginRemoveRows(),
		// which means when moving items within a list, it can catch us while there is temporarily nullptr.
		// (see the code and comments in dropInternalItems)
		if (this->isNull( index.row() ))
			return Qt::NoItemFlags;

		const Item & item = (*this)[ index.row() ];

		Qt::ItemFlags flags = QAbstractListModel::flags( index );  // default flags
		flags |= Qt::ItemIsDragEnabled;
		if (canBeEdited( item ))
			flags |= Qt::ItemIsEditable;
		if (canBeChecked( item ))
			flags |= Qt::ItemIsUserCheckable;

		return flags;
	}

	virtual QVariant data( const QModelIndex & index, int role ) const override
	{
		if (!index.isValid() || index.row() >= this->size())
			return QVariant();

		if (this->isNull( index.row() ))
		{
			logLogicError() << QStringLiteral("EditableListModel::data: item at index %1 is null").arg( index.row() );
			return QVariant();
		}

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
		if (!index.isValid() || index.row() >= this->size())
			return false;

		if (this->isNull( index.row() ))
		{
			logLogicError() << QStringLiteral("EditableListModel::setData: item at index %1 is null").arg( index.row() );
			return false;
		}

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

	virtual bool insertRows( int row, int count, const QModelIndex & ) override
	{
		if (row < 0 || row > this->size())
			return false;

		if (!this->canBeModified())
		{
			logLogicError() << "Cannot insertRows into this model now. It should have been restricted by the ListView.";
			return false;
		}

		startInserting( row, count );

		// n times moving all the elements forward to insert one is not nice, but they are only pointers,
		// it happens only once in awhile, and the number of elements is almost always very low
		for (int i = 0; i < count; i++)
			this->insert( row + i, Item() );

		finishInserting();

		return true;
	}

	virtual bool removeRows( int row, int count, const QModelIndex & ) override
	{
		if (row < 0 || count < 0 || row + count > this->size())
			return false;

		if (!this->canBeModified())
		{
			logLogicError() << "Cannot removeRows from this model now. It should have been restricted by the ListView.";
			return false;
		}

		startDeleting( row, count );

		// n times moving all the elements backward to insert one is not nice
		// but it happens only once in awhile and the number of elements is almost always very low
		for (int i = 0; i < count; i++)
		{
			this->removeAt( row );
			if (row < DropTarget::droppedRow())  // we are removing a row that is before the target row
				DropTarget::decrementRow();      // so target row's index is moving backwards
		}

		finishDeleting();

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
		QByteArray encodedData( indexes.size() * qsizetype( sizeof(int) ), 0 );
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
	virtual bool dropMimeData( const QMimeData * mime, Qt::DropAction action, int row, int, const QModelIndex & ) override
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
			return dropInternalItems( mime->data( internalMimeType ), row );
		}
		else if (mime->hasUrls())
		{
			return dropMimeUrls( mime->urls(), row );
		}
		else
		{
			logLogicError() << "This model doesn't support such drop operation. It should have been restricted by the ListView.";
			return false;
		}
	}

	bool dropInternalItems( QByteArray encodedData, int row )
	{
		// retrieve the original row indexes of the items to be moved
		const int * rawData = reinterpret_cast< int * >( encodedData.data() );
		qsizetype count = encodedData.size() / qsizetype( sizeof(int) );

		// The indexes of selected items can come in arbitrary order, but we need to drop them in ascending order.
		std::vector< int > sortedItemIndexes( rawData, rawData + count );
		std::sort( sortedItemIndexes.begin(), sortedItemIndexes.end() );

		// Because every insert or remove operation shifts the items and invalidates the indexes (or iterators),
		// we need to capture the original items before inserting anything at the target position. We can avoid
		// copying or moving the instances of Item by abusing the fact that PtrList is an array of pointers to Item.
		//
		// First we take the pointers to the selected items, leaving null in their place to avoid shifting,
		// then we insert the pointers in the new place, and then we leave it to Qt to call removeRows() and remove the
		// null pointers where the items were originally.

		// cannot use QList or QVector here because those require copyable objects
		std::vector< std::unique_ptr< Item > > droppedItems;
		droppedItems.reserve( count );
		for (int idx : std::as_const( sortedItemIndexes ))
			droppedItems.push_back( this->takePtr( idx ) );  // leaves null at idx

		startInserting( row, int(count) );

		for (size_t i = 0; i < droppedItems.size(); ++i)
		{
			this->insertPtr( row + i, std::move( droppedItems[i] ) );
		}

		finishInserting();

		// And now we wait for a call to removeRows to remove those null pointers.

		// idiotic workaround because Qt is fucking retarded   (read the comment at the top of EditableListView.cpp)
		//
		// note down the destination drop index, so it can be later retrieved by ListView
		DropTarget::itemsDropped( row, count );

		return true;
	}

	bool dropMimeUrls( const QList< QUrl > & urls, int row )
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
		if (!insertRows( row, filesToBeInserted.size(), QModelIndex() ))
		{
			return false;
		}

		for (qsizetype i = 0; i < filesToBeInserted.size(); i++)
		{
			// This template class doesn't know about the structure of Item, it's supposed to be universal for any.
			// Therefore only author of Item knows how to assign a dropped file into it, so he must define it by a constructor.
			(*this)[ row + i ] = Item( filesToBeInserted[ i ] );
		}

		// idiotic workaround because Qt is fucking retarded   (read the comment at the top of EditableListView.cpp)
		//
		// note down the destination drop index, so it can be later retrieved by ListView
		DropTarget::itemsDropped( row, int( filesToBeInserted.size() ) );

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
