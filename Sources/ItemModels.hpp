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


//======================================================================================================================
/** Abstract wrapper around list of arbitrary objects, mediating their content to UI view elements.
  * The model doesn't own the data, they are stored somewhere else, it just presents them to the UI. */

template< typename Object >
class AObjectListModel : public QAbstractListModel {

 protected:

	QList< Object > & objectList;

 public:

	AObjectListModel( QList< Object > & objectList ) : QAbstractListModel( nullptr ), objectList( objectList ) {}

	QList< Object > & list() const { return objectList; }

	int rowCount( const QModelIndex & = QModelIndex() ) const override
	{
		return objectList.size();
	}

	void updateView( int changeBeginIdx, int changeEndIdx = -1 )
	{
		if (changeEndIdx < 0)
			changeEndIdx = objectList.size();
		emit dataChanged( createIndex( changeBeginIdx, 0 ), createIndex( changeEndIdx - 1, 0 ), {Qt::DisplayRole} );
	}

};


//======================================================================================================================
/** Wrapper around list of arbitrary objects, mediating their content to UI view elements with read-only access. */

template< typename Object >
class ReadOnlyListModel : public AObjectListModel< Object > {

	using superClass = AObjectListModel<Object>;

 protected:

	// This template class doesn't know about the structure of Object, it's supposed to be universal for any.
	// Therefore only author of Object knows how to display Object in the widget, so he must specify it by a function.
	std::function< QString ( const Object & ) > makeDisplayString;

 public:

	ReadOnlyListModel( QList< Object > & objectList, std::function< QString ( const Object & ) > makeDisplayString )
		: AObjectListModel<Object>( objectList ), makeDisplayString( makeDisplayString ) {}

	void setDisplayStringFunc( std::function< QString ( const Object & ) > makeDisplayString )
		{ this->makeDisplayString = makeDisplayString; }

	QVariant data( const QModelIndex & index, int role ) const override
	{
		if (!index.isValid() || index.row() >= superClass::objectList.size())
			return QVariant();
		if (role != Qt::DisplayRole)
			return QVariant();

		// Some UI elements may want to display only the Object name, some others a string constructed from multiple
		// Object elements. This way we generalize from the way the display string is constructed from the Object.
		return makeDisplayString( superClass::objectList[ index.row() ] );
	}

};


//======================================================================================================================
/** Wrapper around list of arbitrary objects, mediating their names to UI view elements.
  * Supports in-place editing, internal drag&drop reordering, and external file drag&drops. */

template< typename Object >
class EditableListModel : public AObjectListModel< Object > {

	using superClass = AObjectListModel<Object>;

 protected:

	// This template class doesn't know about the structure of Object, it's supposed to be universal for any.
	// Therefore only author of Object knows how to perform certain operations on it, so he must specify it by functions

	/// function that points the model to a string member of Object containing the text to be displayed in the widget
	std::function< QString & ( Object & ) > displayString;

	/// function that assigns a dropped file into a newly created Object
	std::function< void	( Object &, const QFileInfo & ) > assignFile;

	/// function that points the model to a bool flag of Object indicating whether the item is checked
	std::function< bool & ( Object & ) > isChecked;

	bool checkableItems;

 public:

	EditableListModel( QList< Object > & objectList, std::function< QString & ( Object & ) > displayString )
		: AObjectListModel<Object>( objectList ), displayString( displayString ), checkableItems( false ) {}

	void setDisplayStringFunc( std::function< QString & ( Object & ) > displayString )
		{ this->displayString = displayString; }
	void setAssignFileFunc( std::function< void ( Object &, const QFileInfo & ) > assignFile )
		{ this->assignFile = assignFile; }
	void setIsCheckedFunc( std::function< bool & ( Object & ) > isChecked )
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
		if (index.parent().isValid() || !index.isValid() || index.row() >= superClass::objectList.size())
			return QVariant();

		if (role == Qt::DisplayRole || role == Qt::EditRole) {
			// This template class doesn't know about the structure of Object, it's supposed to be universal for any.
			// Therefore only author of Object knows which of its memebers he wants to display in the widget,
			// so he must specify it by a function.
			return displayString( superClass::objectList[ index.row() ] );
		} if (role == Qt::CheckStateRole && checkableItems) {
			// Same as above, exept that this function is optional to ease up initializations of models
			// that are supposed to be used in non-checkable widgets
			if (!isChecked) {
				qWarning() << "checkableItems has been set, but no isChecked function is specified. "
				              "Either specify an isChecked function or disable disable checkable items.";
				return QVariant();
			} else {
				return isChecked( superClass::objectList[ index.row() ] ) ? Qt::Checked : Qt::Unchecked;
			}
		} else {
			return QVariant();
		}
	}

	bool setData( const QModelIndex & index, const QVariant & value, int role ) override
	{
		if (index.parent().isValid() || !index.isValid() || index.row() >= superClass::objectList.size())
			return false;

		if (role == Qt::DisplayRole || role == Qt::EditRole) {
			displayString( superClass::objectList[ index.row() ] ) = value.toString();
			//emit superClass::dataChanged( index, index, {role} );
			return true;
		} else if (role == Qt::CheckStateRole && checkableItems) {
			if (!isChecked) {
				qWarning() << "checkableItems has been set, but no isChecked function is specified. "
				              "Either specify an isChecked function or disable disable checkable items.";
				return false;
			} else {
				isChecked( superClass::objectList[ index.row() ] ) = (value == Qt::Checked ? true : false );
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
			superClass::objectList.insert( row + i, Object() );

		QAbstractListModel::endInsertRows();
		return true;
	}

	bool removeRows( int row, int count, const QModelIndex & parent ) override
	{
		if (parent.isValid() || row < 0 || row + count > superClass::objectList.size()) {
			return false;
		}

		QAbstractListModel::beginRemoveRows( parent, row, row + count - 1 );

		// n times moving all the elements backward to insert one is not nice
		// but it happens only once in awhile and the number of elements is almost always very low
		for (int i = 0; i < count; i++) {
			superClass::objectList.removeAt( row );
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
		// content of each Object and then deserialize all of it back. Instead we can serialize only indexes of the items
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
		if (row < 0 || row > superClass::objectList.size())
			row = superClass::objectList.size();

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
		// retrieve the original indexes of the items to be moved
		const int * rawData = reinterpret_cast< int * >( encodedData.data() );
		int count = encodedData.size() / int(sizeof(int));

		// Because insertRows shifts the items and invalidates the indexes, we need to capture the original items before
		// allocating space at the target position. We can avoid making a local temporary copy of Object by abusing
		// the fact that QList is essentially an array of pointers to objects (official API documentation says so).
		// So we can just save the pointers to the items and when the rows are inserted and shifted, those pointers
		// will still point to the correct items and we can use them copy or move the data from them to the target pos.

		// We could probably avoid even the copy/move constructors and the allocation when inserting an item into QList,
		// but we would need a direct access to the pointers inside QList which it doesn't provide.
		// Alternativelly the QList<Object> could be replaced with QVector<Object*> or QVector<unique_ptr<Object>>
		// but that would just complicate a lot of other things.

		QVector< Object * > origObjectRefs;
		for (int i = 0; i < count; i++) {
			int origIndex = rawData[i];
			origObjectRefs.append( &superClass::objectList[ origIndex ] );
		}

		// allocate space for the items to move to
		insertRows( row, count, parent );

		// move the original items to the target position
		for (int i = 0; i < count; i++) {
			superClass::objectList[ row + i ] = std::move( *origObjectRefs[i] );
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

		superClass::updateView( _droppedRow );

		return true;
	}

	bool dropMimeUrls( QList< QUrl > urls, int row, const QModelIndex & /*parent*/ )
	{
		if (!assignFile) {
			qWarning() << "File has been dropped but no assignFile function was set. "
			              "Either specify an assignFile function or disable file dropping in the widget";
			return false;
		}

		// idiotic workaround because Qt is fucking retarded, read the big comment above
		_droppedRow = row;
		_droppedCount = 0;

		for (const QUrl & droppedUrl : urls)
		{
			QString localPath = droppedUrl.toLocalFile();
			if (localPath.isEmpty())
				continue;

			QFileInfo fileInfo( localPath );
			if (!fileInfo.exists())
				continue;

			superClass::objectList.insert( row, Object() );
			// This template class doesn't know about the structure of Object, it's supposed to be universal for any.
			// Therefore only author of Object knows how to assign a dropped file into it, so he must define it by a function.
			assignFile( superClass::objectList[ row ], fileInfo );
			row++;
			_droppedCount++;
		}

		_dropped = true;

		superClass::updateView( _droppedRow );

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


#endif // ITEM_MODELS_INCLUDED
