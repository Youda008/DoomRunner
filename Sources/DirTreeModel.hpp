//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  11.4.2020
// Description: model representing files inside a directory hierarchy
//======================================================================================================================

#ifndef DIR_TREE_MODEL_INCLUDED
#define DIR_TREE_MODEL_INCLUDED


#include "Common.hpp"

#include "LangUtils.hpp"

#include <QAbstractItemModel>
#include <QList>
#include <QString>
#include <QMimeData>
#include <QUrl>

#include <functional>

#include <QDebug>


//======================================================================================================================
// We use model-view design pattern for several widgets, because it allows us to organize the data in a way we need,
// and have the widget (frontend) automatically mirror the underlying data (backend) without syncing them manually.
//
// You can read more about it here: https://doc.qt.io/qt-5/model-view-programming.html#model-subclassing-reference


//======================================================================================================================
// simple single-column icon-less string tree model capable of representing files and directories

enum class NodeType {
	DIR,
	FILE
};

/** one file-system entry (directory or file) */
class FSNode {

 public:


	explicit FSNode( const QString & name, NodeType type, FSNode * parent )
		: _name( name ), _type( type ), _parent( parent ) {}
	~FSNode() { deleteChildren(); }

	QString name() const               { return _name; }

	NodeType type() const              { return _type; }
	bool isDir() const                 { return _type == NodeType::DIR; }
	bool isFile() const                { return _type == NodeType::FILE; }
	FSNode * parent() const            { return _parent; }
	FSNode * child( int row ) const    { return _children[ row ]; }
	int childCount() const             { return _children.size(); }
	const auto & children() const      { return _children; }

	int row() const
	{
		return !_parent ? -1 : _parent->_children.indexOf( const_cast< FSNode * >( this ) );
	}

	/** returns a child with this name, or nullptr if it doesn't exist */
	FSNode * child( const QString & name )
	{
		// linear complexity, but we expect the sublists to be small
		int idx = findSuch( _children, [ &name ]( FSNode * node ) { return node->name() == name; } );
		return idx >= 0 ? _children[ idx ] : nullptr;
	}

	/** creates and returns a child of this name, or returns an existing one if it already exists */
	FSNode * addChild( const QString & name, NodeType type )
	{
		// linear complexity, but we expect the sublists to be small
		int idx = findSuch( _children, [ &name ]( FSNode * node ) { return node->name() == name; } );
		if (idx >= 0) {
			return _children[ idx ];
		} else {
			_children.append( new FSNode( name, type, this ) );
			return _children.last();
		}
	}

	void deleteChildren()
	{
		for (FSNode * child : _children)
			delete child;
		_children.clear();
	}

 private:

	QString _name;
	NodeType _type;

	QVector< FSNode * > _children;
	FSNode * _parent;

};


/** sequence of names of parent nodes ordered from root to leaf, unambiguously defining a node in a tree */
class TreePath : public QStringList {
 public:
	using QStringList::QStringList;

	/** makes TreePath from string in file-system format ("node1/node2/leaf") */
	TreePath( const QString & pathStr ) : QStringList( pathStr.split( '/', QString::SkipEmptyParts ) ) {}
};


/** Simple single-column icon-less tree model. */
class DirTreeModel : public QAbstractItemModel {

	FSNode * rootNode;  ///< internal node that stores all the other nodes without an explicit parent
	QString & baseDir;  ///< directory from which the MIME URLs are derived when items are dragged from this
	                    // TODO: this should be value and it should be instead of mapDir member in MainWindow
 public:

	DirTreeModel( QString & baseDir )
		: QAbstractItemModel( nullptr )
		, baseDir( baseDir )
	{
		rootNode = new FSNode( "", NodeType::DIR, nullptr );
	}

	~DirTreeModel() override
	{
		delete rootNode;  // recursively deletes all the child nodes
	}

	//-- custom methods for manipulating the tree ----------------------------------------------------------------------

	/** Note that before starting to add or delete items in this model, you have to call startCompleteUpdate,
	  * and when you are finished with it, you have to call finishCompleteUpdate. */
	QModelIndex addItem( const QModelIndex & parentIndex, const QString & name, NodeType type )
	{
		FSNode * parent = modelIndexToTreeNode( parentIndex );
		return treeNodeToModelIndex( parent->addChild( name, type ) );
	}

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

		FSNode * node = modelIndexToTreeNode( index );
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
		FSNode * node = rootNode;
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
		FSNode * const parent = modelIndexToTreeNode( parentIndex );

		for (int childRow = 0; childRow < parent->childCount(); childRow++)
		{
			FSNode * const child = parent->child( childRow );
			const QModelIndex childIndex = treeNodeToModelIndex( child, childRow );

			doOnItem( childIndex );

			traverseItems( doOnItem, childIndex );
		}
	}

	NodeType getType( const QModelIndex & index ) const
	{
		FSNode * node = modelIndexToTreeNode( index );
		return node->type();
	}
	bool isDir( const QModelIndex & index ) const
	{
		return getType( index ) == NodeType::DIR;
	}
	bool isFile( const QModelIndex & index ) const
	{
		return getType( index ) == NodeType::FILE;
	}

	//-- data change notifications -------------------------------------------------------------------------------------

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

	//-- implementation of QAbstractItemModel's virtual methods --------------------------------------------------------

	int rowCount( const QModelIndex & parentIndex = QModelIndex() ) const override
	{
		FSNode * const parent = modelIndexToTreeNode( parentIndex );
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

		Qt::ItemFlags flags = QAbstractItemModel::flags( index );
		if (isFile( index ))
			flags |= Qt::ItemIsDragEnabled;

		return flags;
	}

	QVariant data( const QModelIndex & index, int role ) const override
	{
		if (!index.isValid())
			return QVariant();

		FSNode * const node = static_cast< FSNode * >( index.internalPointer() );

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

		FSNode * const parent = modelIndexToTreeNode( parentIndex );
		FSNode * const sibling = parent->child( row );
		return createIndex( row, 0, sibling );
	}

	QModelIndex parent( const QModelIndex & index ) const override
	{
		if (!index.isValid())
			return QModelIndex();

		FSNode * const node = modelIndexToTreeNode( index );
		FSNode * const parent = node->parent();
		return treeNodeToModelIndex( parent );
	}

	/// serializes items into MIME URLs as if they were dragged from a directory window
	QMimeData * mimeData( const QModelIndexList & indexes ) const override
	{
		QMimeData * mimeData = new QMimeData;

		QList< QUrl > urls;
		for (const QModelIndex & index : indexes) {
			QString filePath = baseDir + '/' + getItemPath( index ).join('/');
			urls.append( QUrl::fromLocalFile( filePath ) );
		}
		mimeData->setUrls( urls );

		return mimeData;
	}

	//-- miscellaneous -------------------------------------------------------------------------------------------------

	QModelIndex makeIndex( int row, const QModelIndex & parentIndex ) const
	{
		return index( row, 0, parentIndex );
	}

 private: // helpers

	// Internally we use TreeNode pointers, but the view accesses the model via QModelIndex.
	// If the view passes in an empty index, it wants an item from the top level, an item that doesn't have any parent.
	// We store such elements under a root node (default parent), because it simplifies the implementation.

	FSNode * modelIndexToTreeNode( const QModelIndex & index ) const
	{
		// if no parent is specified, use our internal default parent
		if (!index.isValid())
			return rootNode;
		else
			return static_cast< FSNode * >( index.internalPointer() );
	}
	QModelIndex treeNodeToModelIndex( FSNode * node, int rowInParent = -1 ) const
	{
		// optimization for cases where the row index is known, otherwise we have to perform linear lookup at the parent
		const int row = rowInParent >= 0 ? rowInParent : node->row();

		// root node is internal only, don't expose it to the outside, for the caller it means having no parent at all
		if (node == rootNode)
			return QModelIndex();
		else
			return createIndex( row, 0, node );
	}

};


#endif // DIR_TREE_MODEL_INCLUDED
