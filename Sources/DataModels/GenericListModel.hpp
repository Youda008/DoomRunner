//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: mediators between a list of arbitrary objects and list view or other widgets
//======================================================================================================================

#ifndef GENERIC_LIST_MODEL_INCLUDED
#define GENERIC_LIST_MODEL_INCLUDED


#include "Essential.hpp"

#include "AModelItem.hpp"
#include "ModelCommon.hpp"
#include "CommonTypes.hpp"             // PtrList, DerefIterator
#include "Utils/JsonUtils.hpp"         // for mimeData, dropMimeData
#include "Utils/FileSystemUtils.hpp"   // PathConvertor
#include "Utils/ErrorHandling.hpp"     // LoggingComponent
#include "Themes.hpp"                  // separator colors

#include <QAbstractListModel>
#include <QList>
#include <QVector>
#include <QString>
#include <QStringView>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QRegularExpression>
#include <QBrush>

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
//
// The classes are split into so called "list implementations" and "model implementations".
// The list implementations are wrappers around a list data structure, enabling additional features
// like content searching and filtering.
// The model implementations are essentially just a boilerplate required by Qt to access those lists.
// They derive from Qt's abstract model classes and implement their abstract virtual methods.


//======================================================================================================================
/// A trivial wrapper around PtrList.
/** One of the possible list implementations for the ListModel variants. */

template< typename Item_ >
class DirectList {

	PtrList< Item_ > _list;

 public:

	using Item = Item_;
	using Container = PtrList< Item_ >;

	DirectList() = default;
	DirectList( const Container &  list ) : _list( list ) {}
	DirectList(       Container && list ) : _list( std::move(list) ) {}

	//-- wrapper functions for manipulating the list -------------------------------------------------------------------

	      auto & list()                                { return _list; }
	const auto & list() const                          { return _list; }
	void updateList( const Container &  list )         { _list = list; }
	void assignList(       Container && list )         { _list = std::move(list); }

	// content access

	using iterator = typename Container::iterator;
	using const_iterator = typename Container::const_iterator;

	auto count() const                                 { return _list.count(); }
	auto size() const                                  { return _list.size(); }
	auto isEmpty() const                               { return _list.isEmpty(); }

	      auto & operator[]( qsize_t idx )             { return _list[ idx ]; }
	const auto & operator[]( qsize_t idx ) const       { return _list[ idx ]; }

	      iterator begin()                             { return _list.begin(); }
	const_iterator begin() const                       { return _list.begin(); }
	const_iterator cbegin() const                      { return _list.cbegin(); }
	      iterator end()                               { return _list.end(); }
	const_iterator end() const                         { return _list.end(); }
	const_iterator cend() const                        { return _list.cend(); }

	      auto & first()                               { return _list.first(); }
	const auto & first() const                         { return _list.first(); }
	      auto & last()                                { return _list.last(); }
	const auto & last() const                          { return _list.last(); }

	// list modification

	void reserve( qsize_t size )                       { _list.reserve( size ); }
	void resize( qsize_t size )                        { _list.resize( size ); }

	void clear()                                       { _list.clear(); }

	void append( const Item &  item )                  { _list.append( item ); }
	void append(       Item && item )                  { _list.append( std::move(item) ); }
	void prepend( const Item &  item )                 { _list.prepend( item ); }
	void prepend(       Item && item )                 { _list.prepend( std::move(item) ); }
	void insert( qsize_t idx, const Item &  item )     { _list.insert( idx, item ); }
	void insert( qsize_t idx,       Item && item )     { _list.insert( idx, std::move(item) ); }

	void removeAt( qsize_t idx )                       { _list.removeAt( idx ); }

	void move( qsize_t from, qsize_t to )              { _list.move( from, to ); }
	void moveToFront( qsize_t from )                   { _list.move( from, 0 ); }
	void moveToBack( qsize_t from )                    { _list.move( from, size() - 1 ); }

	template< typename Range, REQUIRES( types::is_range_of< Range, Item > ) >
	void insertMultiple( qsize_t where, Range && range ) { _list.insertMultiple( where, std::forward< Range >( range ) ); }
	void removeCountAt( qsize_t idx, qsize_t cnt ) { _list.removeCountAt( idx, cnt ); }

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

	std::unique_ptr< Item > takePtr( qsize_t idx )               { return _list.takePtr( idx ); }
	void assignPtr( qsize_t idx, std::unique_ptr< Item > ptr )   { _list.assignPtr( idx, std::move(ptr) ); }

	void insertDefaults( qsize_t where, qsize_t count )          { _list.insertDefaults( where, count ); }
	template< typename PtrRange, REQUIRES( types::is_range_of< PtrRange, std::unique_ptr< Item > > ) >
	void insertPtrs( qsize_t where, PtrRange && ptrs )           { _list.insertPtrs( where, std::forward< PtrRange >( ptrs ) ); }

	bool isNull( qsize_t idx ) const                             { return _list.isNull( idx ); }

	//-- special -------------------------------------------------------------------------------------------------------

	/// Whether the list modification functions can be safely called.
	bool canBeModified() const { return true; }

};


//======================================================================================================================
/// A wrapper around PtrList allowing to temporarily filter the content present only items matching a specified criteria.
/** One of the possible list implementations for the ListModel variants. */

template< typename Item_ >
class FilteredList {

	PtrList< Item_ > _fullList;
	QVector< Item_ * > _filteredList;

 public:

	using Item = Item_;
	using Container = PtrList< Item_ >;

	FilteredList() = default;
	FilteredList( const Container &  list ) : _fullList( list ) { restore(); }
	FilteredList(       Container && list ) : _fullList( std::move(list) ) { restore(); }

	//-- wrapper functions for manipulating the list -------------------------------------------------------------------

	      auto & fullList()                            { return _fullList; }
	const auto & fullList() const                      { return _fullList; }
	      auto & filteredList()                        { return _filteredList; }
	const auto & filteredList() const                  { return _filteredList; }
	void updateList( const Container &  list )         { _fullList = list; restore(); }
	void assignList(       Container && list )         { _fullList = std::move(list); restore(); }

	// content access

	using iterator = DerefIterator< typename decltype( _filteredList )::iterator >;
	using const_iterator = DerefIterator< typename decltype( _filteredList )::const_iterator >;

	auto count() const                                 { return _filteredList.count(); }
	auto size() const                                  { return _filteredList.size(); }
	auto isEmpty() const                               { return _filteredList.isEmpty(); }

	      auto & operator[]( qsize_t idx )             { return *_filteredList[ idx ]; }
	const auto & operator[]( qsize_t idx ) const       { return *_filteredList[ idx ]; }

	      iterator begin()                             { return DerefIterator( _filteredList.begin() ); }
	const_iterator begin() const                       { return DerefIterator( _filteredList.begin() ); }
	const_iterator cbegin() const                      { return DerefIterator( _filteredList.cbegin() ); }
	      iterator end()                               { return DerefIterator( _filteredList.end() ); }
	const_iterator end() const                         { return DerefIterator( _filteredList.end() ); }
	const_iterator cend() const                        { return DerefIterator( _filteredList.cend() ); }

	      auto & first()                               { return *_filteredList.first(); }
	const auto & first() const                         { return *_filteredList.first(); }
	      auto & last()                                { return *_filteredList.last(); }
	const auto & last() const                          { return *_filteredList.last(); }

	// list modification - only when the list is not filtered

	void reserve( qsize_t newSize )
	{
		ensureCanBeModified();
		_fullList.reserve( newSize );
		_filteredList.reserve( newSize );
	}

	void resize( qsize_t newSize )
	{
		ensureCanBeModified();
		const auto oldSize = _fullList.size();
		const auto addedCount = newSize - oldSize;  // can be negative if the size is being reduced
		_fullList.resize( newSize );
		if (addedCount > 0)
			insertUpdatedPtrs( oldSize, addedCount );
	}

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

	void insert( qsize_t idx, const Item & item )
	{
		ensureCanBeModified();
		_fullList.insert( idx, item );
		_filteredList.insert( idx, &_fullList[ idx ] );
	}
	void insert( qsize_t idx, Item && item )
	{
		ensureCanBeModified();
		_fullList.insert( idx, std::move(item) );
		_filteredList.insert( idx, &_fullList[ idx ] );
	}

	void removeAt( qsize_t idx )
	{
		if (!isFiltered())
		{
			_fullList.removeAt( idx );
			_filteredList.removeAt( idx );
		}
		else
		{
			// can be allowed for filtered list, but the fullList entry needs to be found and removed too
			auto * ptr = _filteredList.takeAt( idx );
			for (qsize_t i = 0; i < _fullList.size(); ++i)
				if (&_fullList[i] == ptr)
					_fullList.removeAt( i );
		}
	}

	void move( qsize_t from, qsize_t to )
	{
		ensureCanBeModified();
		_fullList.move( from, to );
		_filteredList.move( from, to );
	}

	void moveToFront( qsize_t from )
	{
		ensureCanBeModified();
		_fullList.move( from, 0 );
		_filteredList.move( from, 0 );
	}

	void moveToBack( qsize_t from )
	{
		ensureCanBeModified();
		_fullList.move( from, size() - 1 );
		_filteredList.move( from, size() - 1 );
	}

	template< typename Range, REQUIRES( types::is_range_of< Range, Item > ) >
	void insertMultiple( qsize_t where, Range && range )
	{
		ensureCanBeModified();
		_fullList.insertMultiple( where, std::forward< Range >( range ) );
		insertUpdatedPtrs( where, qsize_t( std::size(range) ) );
	}

	void removeCountAt( qsize_t idx, qsize_t cnt )
	{
		ensureCanBeModified();
		_fullList.removeCountAt( idx, cnt );
		::removeCountAt( _filteredList, idx, cnt );
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

	std::unique_ptr< Item > takePtr( qsize_t idx )
	{
		ensureCanBeModified();
		_filteredList[ idx ] = nullptr;
		return _fullList.takePtr( idx );
	}

	void assignPtr( qsize_t idx, std::unique_ptr< Item > ptr )
	{
		ensureCanBeModified();
		_fullList.assignPtr( idx, std::move(ptr) );
		_filteredList[ idx ] = &_fullList[ idx ];
	}

	void insertDefaults( qsize_t where, qsize_t count )
	{
		ensureCanBeModified();
		_fullList.insertDefaults( where, count );
		insertUpdatedPtrs( where, count );
	}

	template< typename PtrRange, REQUIRES( types::is_range_of< PtrRange, std::unique_ptr< Item > > ) >
	void insertPtrs( qsize_t where, PtrRange && ptrs )
	{
		ensureCanBeModified();
		_fullList.insertPtrs( where, std::forward< PtrRange >( ptrs ) );
		insertUpdatedPtrs( where, qsize_t( ptrs.size() ) );
	}

	bool isNull( qsize_t idx ) const
	{
		return _fullList.isNull( idx );
	}

	//-- searching/filtering -------------------------------------------------------------------------------------------

	/// Filters the list model entries to display only those that match a given criteria.
	void search( const QString & phrase, bool caseSensitive, bool useRegex )
	{
		clearButKeepAllocated( _filteredList );

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
		clearButKeepAllocated( _filteredList );
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
			::logLogicError( u"FilteredList" ) << "the list cannot be modified when it is filtered";
			throw std::logic_error("the list cannot be modified when it is filtered");
		}
	}

	// Takes addresses of {count} items starting at {where} in the _fullList and inserts them into _filteredList.
	void insertUpdatedPtrs( qsize_t where, qsize_t count )
	{
		reserveSpace( _filteredList, where, count );
		for (qsize_t i = 0; i < count; i++)
			_filteredList[ where + i ] = &_fullList[ where + i ];
	}

};


//======================================================================================================================
// idiotic workaround because Qt is fucking retarded   (read the comment at the top of ExtendedListView.cpp)
//
// Exists to allow ExtendedListView to retrieve the destination drop index.

class DropTarget {

 public:

	bool wasDroppedInto() const   { return _dropped; }
	int droppedRow() const        { return _droppedRow; }
	int droppedCount() const      { return _droppedCount; }

	void resetDropState()
	{
		_dropped = false;
		_droppedRow = -1;
		_droppedCount = -1;
	}

 protected:

	void itemsDropped( int row, int count )
	{
		_dropped = true;
		_droppedRow = row;
		_droppedCount = count;
	}

	void decrementRow( int count )
	{
		_droppedRow -= count;
	}

 private:

	bool _dropped = false;
	int _droppedRow = -1;
	int _droppedCount = -1;

};


//======================================================================================================================
/// Our own abstract list model.
/** Contains code of our list models that doesn't depend on the template parameter Item. */

class AListModel : public QAbstractListModel, public ErrorReportingComponent, public DropTarget {

	Q_OBJECT

 protected:

	using QBaseModel = QAbstractListModel;  ///< The Qt's abstract model class we inherit from

	AListModel( QStringView modelName )
		: QAbstractListModel( nullptr ), ErrorReportingComponent( nullptr, u"GenericListModel", modelName ) {}
	~AListModel() override;

 public:

	QStringView modelName() const   { return LoggingComponent::componentName(); }

	virtual AccessStyle accessStyle() const = 0;

	//-- model configuration -------------------------------------------------------------------------------------------

	void toggleIcons( bool enabled )         { iconsEnabled = enabled; }
	bool areIconsEnabled() const             { return iconsEnabled; }

	void toggleCheckboxes( bool enabled )    { checkboxesEnabled = enabled; }

	void toggleItemEditing( bool enabled )   { editingEnabled = enabled; }

	void setEnabledExportFormats( ExportFormats formats );
	void setEnabledImportFormats( ExportFormats formats );
	/// Required for import format FileUrls to work properly.
	void setPathConvertor( PathConvertor & pathConvertor ) { this->pathConvertor = &pathConvertor; }


	//-- data change notifications -------------------------------------------------------------------------------------

	/// Describes how exactly the model is being modified.
	enum class Operation
	{
		None = 0,
		SetData,         ///< The data of an item is being modified. All items remain in their place.
		Reorder,         ///< The items are being re-ordered. Data of the items however remains unchanged.
		Insert,          ///< New items are being inserted into the list. Some existing items may be moved as a result.
		Remove,          ///< Some existing items are being removed. Other existing items may be moved as a result.
		CompleteUpdate,  ///< The model is being filled from scratch. Anything can change.
	};

	/// Returns whether the content of this model is currently in the process of being modified.
	/** The model should not be accessed in such case because its data might in an inconsistent state. */
	bool isBeingModified() const
	{
		return _operInProgress != Operation::None;
	}

	// pre-defined commonly used lists of data roles
	static const QVector<int> onlyDisplayRole;
	static const QVector<int> onlyEditRole;
	static const QVector<int> onlyCheckStateRole;
	static const QVector<int> allDataRoles;         ///< all the data roles our models use

	// One of the following functions must always be called before and after doing any modifications to the list,
	// otherwise the list might not update correctly or it might even crash trying to access items that no longer exist.

	void startEditingItemData() {}
	void finishEditingItemData( int row = 0, int count = -1, const QVector<int> & roles = allDataRoles );

	void startReorderingItems()
	{
		_operInProgress = Operation::Reorder;
		emit QBaseModel::layoutAboutToBeChanged( {}, LayoutChangeHint::VerticalSortHint );
	}
	void finishReorderingItems()
	{
		_operInProgress = Operation::None;
		emit QBaseModel::layoutChanged( {}, LayoutChangeHint::VerticalSortHint );
	}

	void startAppendingItems( int count )
	{
		startInsertingItems( this->rowCount(), count );
	}
	void finishAppendingItems()
	{
		finishInsertingItems();
	}

	void startInsertingItems( int row, int count = 1 )
	{
		_operInProgress = Operation::Insert;
		QBaseModel::beginInsertRows( QModelIndex(), row, row + count - 1 );
	}
	void finishInsertingItems()
	{
		_operInProgress = Operation::None;
		QBaseModel::endInsertRows();
	}

	void startRemovingItems( int row, int count = 1 )
	{
		_operInProgress = Operation::Remove;
		QBaseModel::beginRemoveRows( QModelIndex(), row, row + count - 1 );
	}
	void finishRemovingItems()
	{
		_operInProgress = Operation::None;
		QBaseModel::endRemoveRows();
	}

	void startCompleteUpdate()
	{
		_operInProgress = Operation::CompleteUpdate;
		QBaseModel::beginResetModel();
	}
	void finishCompleteUpdate()
	{
		_operInProgress = Operation::None;
		QBaseModel::endResetModel();
	}

	// Additionally, one of these should be called after finishing externally triggered modifications of the model,
	// which means modifications requested by a view object via the QAbstractItemModel's methods
	// (setData, insertRows, removeRows, ...), commonly due to some user action like drag&drop.

	void notifyDataChanged( int row = 0, int count = -1, const QVector<int> & roles = allDataRoles )
	{
		if (count < 0)
			count = this->rowCount();
		emit itemDataChanged( row, count, roles );
	}

	void notifyItemsReordered()
	{
		emit itemsReordered();
	}

	void notifyItemsInserted( int row, int count )
	{
		emit itemsInserted( row, count );
	}

	void notifyItemsRemoved( int row, int count )
	{
		emit itemsRemoved( row, count );
	}

	//-- miscellaneous -------------------------------------------------------------------------------------------------

	QModelIndex makeModelIndex( int row ) const
	{
		return QBaseModel::index( row, /*column*/0, /*parent*/QModelIndex() );
	}

 signals:

	// customized variants of QAbstractItemModel's generic signals that are emitted only on externally triggered operations
	void itemDataChanged( int row, int count, const QVector<int> & roles );
	void itemsReordered();
	void itemsInserted( int row, int count );
	void itemsRemoved( int row, int count );

 protected: // internal state

	Operation _operInProgress = Operation::None;

 protected: // configuration

	/// whether items have an icon
	bool iconsEnabled = false;

	/// whether items have a checkbox that can be checked and unchecked
	bool checkboxesEnabled = false;

	/// whether editing of regular non-separator items is allowed
	bool editingEnabled = false;

	/// Allowed ways how items can be exported from this model when dragging them out or copying them to clipboard.
	ExportFormats enabledExportFormats = 0;
	bool canExportItems() const           { return enabledExportFormats != 0; }
	bool canExportItemsAsUrls() const     { return areFlagsSet( enabledExportFormats, ExportFormat::FileUrls ); }
	bool canExportItemsAsJson() const     { return areFlagsSet( enabledExportFormats, ExportFormat::Json ); }
	bool canExportItemsAsIndexes() const  { return areFlagsSet( enabledExportFormats, ExportFormat::Indexes ); }

	/// Allowed ways how items can be imported into this model when dropping them in or pasting them from clipboard.
	ExportFormats enabledImportFormats = 0;
	bool canImportItems() const           { return enabledImportFormats != 0; }
	bool canImportItemsAsUrls() const     { return areFlagsSet( enabledImportFormats, ExportFormat::FileUrls ); }
	bool canImportItemsAsJson() const     { return areFlagsSet( enabledImportFormats, ExportFormat::Json ); }
	bool canImportItemsAsIndexes() const  { return areFlagsSet( enabledImportFormats, ExportFormat::Indexes ); }

	/// optional path convertor that will convert paths dropped from directory to absolute or relative
	const PathConvertor * pathConvertor = nullptr;

};


//======================================================================================================================
/// Wrapper around list of arbitrary objects, mediating their content to the UI component.
/** Supports in-place editing, internal drag&drop reordering, and external file drag&drops. */

template< typename ListImpl, AccessStyle access >  // ListImpl::Item must be a subclass of EditableModelItem
class GenericListModel : public AListModel, public ListImpl {

	DECLARE_MODEL_SUPERCLASS_ACCESSORS( AListModel, ListImpl, list )

 public:

	using Item = typename ListImpl::Item;
	using Container = typename ListImpl::Container;

	GenericListModel( QStringView modelName, std::function< QString ( const Item & ) > makeDisplayString )
		: AListModel( modelName ), ListImpl(), makeDisplayString( std::move(makeDisplayString) ) {}

	GenericListModel( QStringView modelName, const Container & list, std::function< QString ( const Item & ) > makeDisplayString )
		: AListModel( modelName ), ListImpl( list ), makeDisplayString( std::move(makeDisplayString) ) {}

	// Allow the AListModel to read this configuration property (a compile-time template parameter)
	// via a virtual method call (runtime polymorphism).
	virtual AccessStyle accessStyle() const override
	{
		return access;
	}

	//-- implementation of QAbstractItemModel's virtual methods --------------------------------------------------------

	public: virtual int rowCount( const QModelIndex & /*parent*/ = QModelIndex() ) const override
	{
		return listImpl().size();
	}

	public: virtual Qt::ItemFlags flags( const QModelIndex & index ) const override
	{
		if (index.row() < 0 || index.row() >= listImpl().size())
		{
			if (isReadOnly())
				return Qt::NoItemFlags;
			else
				return Qt::ItemIsDropEnabled;  // otherwise you can't append dragged items to the end of the list
		}

		// On some OSes Qt calls flags inside beginRemoveRows(),
		// which means when moving items within a list, it can catch us while there is temporarily nullptr.
		// (see the code and comments in dropInternalIndexes())
		if (listImpl().isNull( index.row() ))
		{
			return Qt::NoItemFlags;
		}

		const Item & item = listImpl()[ index.row() ];

		Qt::ItemFlags flags = QBaseModel::flags( index );  // default flags
		if (canExportItems())
			flags |= Qt::ItemIsDragEnabled;
		if (canBeChecked( item ))
			flags |= Qt::ItemIsUserCheckable;
		if (canBeEdited( item ))
			flags |= Qt::ItemIsEditable;

		// Note: Qt::ItemIsDropEnabled is not desirable in a list (it's meant for a table),
		// and it's useless unless you view->setDragDropOverwriteMode(true) anyway.

		return flags;
	}

	public: virtual Qt::DropActions supportedDragActions() const override
	{
		Qt::DropActions actions = Qt::IgnoreAction;
		if (canExportItemsAsUrls() || canExportItemsAsJson())  // can drag items to an external destination
			actions |= Qt::CopyAction;
		if (canExportItems() && !isReadOnly())  // can drag items and remove them from this list
			actions |= Qt::MoveAction;
		return actions;
	}

	public: virtual Qt::DropActions supportedDropActions() const override
	{
		Qt::DropActions actions = Qt::IgnoreAction;
		if (!isReadOnly())
		{
			// the drag&drop source should determine whether the items will be moved or copied, not the destination
			actions |= Qt::CopyAction;
			actions |= Qt::MoveAction;
		}
		return actions;
	}

	public: virtual QStringList mimeTypes() const override
	{
		QStringList types;

		types << MimeTypes::ModelPtr;     // to recognize where the data came from

		if (canExportItemsAsUrls())
			types << MimeTypes::UriList;  // for drag&drop from an external source
		if (canExportItemsAsJson())
			types << MimeTypes::Json;     // for copy&pasting within the same model
		if (!isReadOnly() && canExportItemsAsIndexes())
			types << MimeTypes::Indexes;  // for drag&drop reordering within the same model

		return types;
	}

	public: virtual QVariant data( const QModelIndex & index, int role ) const override
	{
		if (index.row() < 0 || index.row() >= listImpl().size())
		{
			logLogicError( u"data" ) << "invalid row index: "<<index.row();
			return QVariant();
		}
		else if (listImpl().isNull( index.row() ))
		{
			logLogicError( u"data" ) << "item at index "<<index.row()<<" is null";
			return QVariant();
		}

		const Item & item = listImpl()[ index.row() ];

		try
		{
			if (role == Qt::DisplayRole)
			{
				// Some UI elements may want to display only the Item name, some others a string constructed from multiple
				// Item elements. This way we generalize from the way the display string is constructed from the Item.
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
			logLogicError( u"data" ) << e.what();
			return QVariant();
		}
	}

	public: virtual bool setData( const QModelIndex & index, const QVariant & value, int role ) override
	{
		if constexpr (isReadOnly())
		{
			return false;
		}

		if (index.row() < 0 || index.row() >= listImpl().size())
		{
			logLogicError( u"setData" ) << "invalid row index: "<<index.row();
			return false;
		}
		else if (listImpl().isNull( index.row() ))
		{
			logLogicError( u"setData" ) << "item at row "<<index.row()<<" is null";
			return false;
		}

		Item & item = listImpl()[ index.row() ];

		try
		{
			if (role == Qt::EditRole && canBeEdited( item ))
			{
				AListModel::startEditingItemData();
				item.setEditString( value.toString() );
				AListModel::finishEditingItemData( index.row(), 1, AListModel::onlyEditRole );
				AListModel::notifyDataChanged( index.row(), 1, AListModel::onlyEditRole );  // notify the model owner about this external modification
				return true;
			}
			else if (role == Qt::CheckStateRole && canBeChecked( item ))
			{
				AListModel::startEditingItemData();
				item.setChecked( value == Qt::Checked ? true : false );
				AListModel::finishEditingItemData( index.row(), 1, AListModel::onlyCheckStateRole );
				AListModel::notifyDataChanged( index.row(), 1, AListModel::onlyCheckStateRole );  // notify the model owner about this external modification
				return true;
			}
			else
			{
				logLogicError( u"setData" ) << "attempted to set unsupported role "<<role<<" to item at row "<<index.row();
				return false;
			}
		}
		catch (const std::logic_error & e)
		{
			logLogicError( u"setData" ) << e.what();
			return false;
		}
	}

	/// Serializes items at \p indexes into MIME data.
	public: virtual QMimeData * mimeData( const QModelIndexList & indexes ) const override
	{
		if (indexes.isEmpty())
		{
			logLogicError( u"mimeData" ) << "empty list of indexes";
			return nullptr;
		}

		if (!canExportItems())
		{
			return nullptr;  // nothing to produce
		}

		QMimeData * mimeData = new QMimeData;

		mimeData->setData( MimeTypes::ModelPtr, makeMimeModelPtr() );  // to recognize the source of the data

		if (canExportItemsAsUrls())
		{
			mimeData->setUrls( makeMimeUrls( indexes ) );
		}
		if (!isReadOnly() && canExportItemsAsJson())
		{
			mimeData->setData( MimeTypes::Json, makeMimeJsonItems( indexes ) );
		}
		if (!isReadOnly() && canExportItemsAsIndexes())
		{
			mimeData->setData( MimeTypes::Indexes, makeMimeRowIndexes( indexes ) );
		}

		return mimeData;
	}

	private: QByteArray makeMimeModelPtr() const
	{
		const auto * aModelPtr = static_cast< const AListModel * >( this );
		return QByteArray( reinterpret_cast< const char * >( &aModelPtr ), qsize_t( sizeof( &aModelPtr ) ) );
	}

	private: QList< QUrl > makeMimeUrls( const QModelIndexList & indexes ) const
	{
		QList< QUrl > urls;
		urls.reserve( indexes.size() );
		for (const QModelIndex & index : indexes)
		{
			if (index.row() < 0 || index.row() >= listImpl().size())
			{
				reportLogicError( u"mimeData", "Cannot export items", "Invalid index: "%QString::number( index.row() ) );
				continue;
			}
			const Item & item = listImpl()[ index.row() ];
			urls.append( QUrl::fromLocalFile( item.getFilePath() ) );
		}
		return urls;
	}

	private: QByteArray makeMimeJsonItems( const QModelIndexList & indexes ) const
	{
		QJsonArray itemsJs;
		for (const QModelIndex & index : indexes)
		{
			const Item & item = listImpl()[ index.row() ];
			itemsJs.append( item.serialize() );
		}
		return QJsonDocument( itemsJs ).toJson( QJsonDocument::Compact );
	}

	private: QByteArray makeMimeRowIndexes( const QModelIndexList & indexes ) const
	{
		// If we only want to reorder the items, we don't need to serialize the whole rich content
		// of each Item and then deserialize all of it back. Instead we can serialize only indexes of the items
		// and then use them in dropMimeData to find the original items and move them to the target position.
		// BEWARE that these MIME data are only usable within the same list.
		//        Outside of this list we must use the other MIME types.
		QByteArray encodedData( indexes.size() * qsize_t( sizeof(int) ), 0 );
		int * rawData = reinterpret_cast< int * >( encodedData.data() );
		for (const QModelIndex & index : indexes)
		{
			*rawData = index.row();
			rawData++;
		}
		return encodedData;
	}

	private: const AListModel * getMimeModelPtr( const QMimeData * mimeData ) const
	{
		if (mimeData->hasFormat( MimeTypes::ModelPtr ))
		{
			QByteArray data = mimeData->data( MimeTypes::ModelPtr );
			if (data.size() == qsize_t( sizeof( AListModel * ) ))
			{
				return *reinterpret_cast< const AListModel * * >( data.data() );
			}
		}
		return nullptr;
	}

	private: bool hasImportableUrls( const QMimeData * mimeData, const AListModel * sourceModel ) const
	{
		return canImportItemsAsUrls() && mimeData->hasUrls() && sourceModel != this;
	}
	private: bool hasImportableJson( const QMimeData * mimeData, const AListModel * sourceModel ) const
	{
		return canImportItemsAsJson() && mimeData->hasFormat( MimeTypes::Json ) && sourceModel == this;
	}
	private: bool hasImportableIndexes( const QMimeData * mimeData, const AListModel * sourceModel, Qt::DropAction action ) const
	{
		return canImportItemsAsIndexes() && mimeData->hasFormat( MimeTypes::Indexes ) && sourceModel == this
			&& action == Qt::MoveAction;
	}

	// called to determine what drag&drop cursor to draw
	public: virtual bool canDropMimeData( const QMimeData * mimeData, Qt::DropAction action, int /*row*/, int /*col*/, const QModelIndex & ) const override
	{
		if (!listImpl().canBeModified())
			return false;

		const auto * sourceModel = getMimeModelPtr( mimeData );
		return hasImportableUrls( mimeData, sourceModel )
		    || hasImportableIndexes( mimeData, sourceModel, action )
		    || hasImportableJson( mimeData, sourceModel );
	}

	/// Deserializes items from MIME data and inserts them before \p row.
	public: virtual bool dropMimeData( const QMimeData * mimeData, Qt::DropAction action, int row, int, const QModelIndex & ) override
	{
		// in edge cases always append to the end of the list
		if (row < 0 || row > listImpl().size())
		{
			row = listImpl().size();
		}

		if (!listImpl().canBeModified())
		{
			// The parent view is probably configured incorrectly. It should have restricted this operation.
			reportLogicError( u"dropMimeData", "Cannot import data", "Model is currently locked and cannot be modified." );
			return false;
		}

		const auto * sourceModel = getMimeModelPtr( mimeData );

		if (hasImportableUrls( mimeData, sourceModel ))
		{
			return dropMimeUrls( mimeData->urls(), row );
		}
		else if (hasImportableIndexes( mimeData, sourceModel, action ))
		{
			return dropMimeInternalIndexes( mimeData->data( MimeTypes::Indexes ), row );
		}
		else if (hasImportableJson( mimeData, sourceModel ))
		{
			return dropMimeSerializedItems( mimeData->data( MimeTypes::Json ), row );
		}
		else
		{
			reportUserError( "Cannot import data", "Inserted unsupported data type:\n" % mimeData->formats().join('\n') );
			return false;
		}
	}

	private: bool dropMimeUrls( const QList< QUrl > & urls, int row )
	{
		if (!pathConvertor)
		{
			// Either use setPathConvertor() or disable file dropping in the view.
			reportLogicError( u"dropMimeData", "Cannot import data", "File has been dropped but PathConvertor is not set." );
		}

		// verify the dropped items so that we don't drop invalid ones
		std::vector< std::unique_ptr< Item > > validDroppedFiles;
		validDroppedFiles.reserve( size_t( urls.size() ) );
		for (const QUrl & droppedUrl : urls)
		{
			QString localPath = droppedUrl.toLocalFile();
			if (!localPath.isEmpty())
			{
				if (pathConvertor)
					localPath = pathConvertor->convertPath( localPath );

				QFileInfo fileInfo( localPath );

				// This template class doesn't know about the structure of Item, it's supposed to be universal for any.
				// Only the author of Item knows how to construct it from a dropped file, so he must define it by a constructor.
				validDroppedFiles.push_back( std::make_unique< Item >( fileInfo ) );
			}
		}
		auto count = int( validDroppedFiles.size() );

		// insert the dropped items in one pass
		AListModel::startInsertingItems( row, count );
		listImpl().insertPtrs( row, std::move( validDroppedFiles ) );
		AListModel::finishInsertingItems();

		// notify the model owner about this external modification
		AListModel::notifyItemsInserted( row, count );

		// idiotic workaround because Qt is fucking retarded   (read the comment at the top of ExtendedListView.cpp)
		//
		// note down the destination drop index, so it can be later retrieved by ListView
		DropTarget::itemsDropped( row, count );

		return true;
	}

	private: bool dropMimeSerializedItems( const QByteArray & encodedData, int row )
	{
		QJsonParseError parseError;
		QJsonDocument jsonDoc = QJsonDocument::fromJson( encodedData, &parseError );
		if (!jsonDoc.isArray())
		{
			reportLogicError( u"dropMimeData", "Cannot import data", "dropped serialized items are not a valid JSON" );
			return false;
		}

		// verify the dropped items so that we don't drop invalid ones
		ParsingContext context;
		context.sourceDesc = "the pasted clipboard content";
		context.dontShowAgain = true;  // don't show message box errors to the user
		JsonArrayCtx itemsJs( jsonDoc.array(), context );
		std::vector< std::unique_ptr< Item > > validDroppedItems;  // cannot use QVector here because those require copyable objects
		validDroppedItems.reserve( size_t( itemsJs.size() ) );
		for (qsize_t i = 0; i < itemsJs.size(); i++)
		{
			JsonObjectCtx itemJs = itemsJs.getObject( i );
			if (!itemJs)  // wrong type on position i - skip this entry
			{
				reportLogicError( u"dropMimeData", "Cannot import data", "dropped item "%QString::number(i)%" is not a JSON object" );
				continue;
			}
			auto item = std::make_unique< Item >();
			if (!item->deserialize( itemJs ))
			{
				reportLogicError( u"dropMimeData", "Cannot import data", "dropped item "%QString::number(i)%" doesn't have the expected structure" );
				continue;
			}
			validDroppedItems.push_back( std::move( item ) );
		}
		auto count = int( validDroppedItems.size() );

		// insert the dropped items in one pass
		AListModel::startInsertingItems( row, count );
		listImpl().insertPtrs( row, std::move( validDroppedItems ) );
		AListModel::finishInsertingItems();

		// notify the model owner about this external modification
		AListModel::notifyItemsInserted( row, count );

		// idiotic workaround because Qt is fucking retarded   (read the comment at the top of ExtendedListView.cpp)
		//
		// note down the destination drop index, so it can be later retrieved by ListView
		DropTarget::itemsDropped( row, count );

		return true;
	}

	private: bool dropMimeInternalIndexes( const QByteArray & encodedData, int row )
	{
		// retrieve the original row indexes of the items to be moved
		const int * rawData = reinterpret_cast< const int * >( encodedData.data() );
		qsize_t count = encodedData.size() / qsize_t( sizeof(int) );

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

		// take the Item pointers out of the list
		std::vector< std::unique_ptr< Item > > movedPointers;  // cannot use QVector here because those require copyable objects
		movedPointers.reserve( size_t( count ) );
		for (int idx : std::as_const( sortedItemIndexes ))
			movedPointers.push_back( listImpl().takePtr( idx ) );  // leaves null at idx

		// insert them to the new positions
		AListModel::startInsertingItems( row, int( movedPointers.size() ) );
		listImpl().insertPtrs( row, std::move( movedPointers ) );
		AListModel::finishInsertingItems();

		// notify the model owner about this external modification
		AListModel::notifyItemsInserted( row, count );

		// and now wait for a call to removeRows() to remove those null pointers

		// idiotic workaround because Qt is fucking retarded   (read the comment at the top of ExtendedListView.cpp)
		//
		// note down the destination drop index, so it can be later retrieved by ListView
		DropTarget::itemsDropped( row, int( movedPointers.size() ) );

		return true;
	}

	public: virtual bool insertRows( int row, int count, const QModelIndex & ) override
	{
		if (count < 0 || row < 0 || row > listImpl().size())
		{
			reportLogicError( u"insertRows", "Cannot insert rows",
				"Invalid arguments, row = "%QString::number(row)%", count = "%QString::number(count)
			);
			return false;
		}

		if (!listImpl().canBeModified())
		{
			// The parent view is probably configured incorrectly. It should have restricted this operation.
			reportLogicError( u"insertRows", "Cannot insert rows", "Model is currently locked and cannot be modified" );
			return false;
		}

		// insert default-constructed (empty) items
		AListModel::startInsertingItems( row, count );
		listImpl().insertDefaults( row, count );
		AListModel::finishInsertingItems();

		// notify the model owner about this external modification
		AListModel::notifyItemsInserted( row, count );

		return true;
	}

	public: virtual bool removeRows( int row, int count, const QModelIndex & ) override
	{
		if (count < 0 || row < 0 || row + count > listImpl().size())
		{
			reportLogicError( u"removeRows", "Cannot remove rows",
				"Invalid arguments, row = "%QString::number(row)%", count = "%QString::number(count)
			);
			return false;
		}

		if (!listImpl().canBeModified())
		{
			// The parent view is probably configured incorrectly. It should have restricted this operation.
			reportLogicError( u"removeRows", "Cannot remove rows", "Model is currently locked and cannot be modified" );
			return false;
		}

		// remove the items
		AListModel::startRemovingItems( row, count );
		listImpl().removeCountAt( row, count );
		AListModel::finishRemovingItems();

		// notify the model owner about this external modification
		AListModel::notifyItemsRemoved( row, count );

		if (row < DropTarget::droppedRow())     // we are removing a row that is before the drop target row
			DropTarget::decrementRow( count );  // so target drop row's index is moving backwards

		return true;
	}

 private: // helpers

	static constexpr bool isReadOnly()
	{
		return access == AccessStyle::ReadOnly;
	}

	bool canHaveIcon( const Item & item ) const
	{
		return iconsEnabled && !item.isSeparator;
	}

	bool canBeChecked( const Item & item ) const
	{
		return !isReadOnly() && ((checkboxesEnabled && item.isCheckable()) && !item.isSeparator);
	}

	bool canBeEdited( const Item & item ) const
	{
		return !isReadOnly() && ((editingEnabled && item.isEditable()) || item.isSeparator);
	}

 protected: // configuration

	// Each list view might want to display the same data differently, so we allow the user of the list model
	// to specify it by a function for each view separately.
	/// function that takes Item and constructs a String that will be displayed in the view
	std::function< QString ( const Item & ) > makeDisplayString;

};


//======================================================================================================================
// aliases

template< typename Item > using ReadOnlyDirectListModel   = GenericListModel< DirectList< Item >, AccessStyle::ReadOnly >;
template< typename Item > using ReadOnlyFilteredListModel = GenericListModel< FilteredList< Item >, AccessStyle::ReadOnly >;
template< typename Item > using EditableDirectListModel   = GenericListModel< DirectList< Item >, AccessStyle::Editable >;
template< typename Item > using EditableFilteredListModel = GenericListModel< FilteredList< Item >, AccessStyle::Editable >;


#endif // GENERIC_LIST_MODEL_INCLUDED
