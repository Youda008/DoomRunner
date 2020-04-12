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
#include <QString>

#include <functional>



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

	QString _name;
	NodeType _type;

	QVector< FSNode * > _children;
	FSNode * _parent;

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
		// linear complexity, but we expect the sublists to be small (who has directories with 100000 entries, right??)
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

};


/** sequence of names of parent nodes ordered from root to leaf, unambiguously defining a node in the tree */
class TreePosition : public QStringList {
 public:
	using QStringList::QStringList;

	/** makes TreePosition from string in file-system format ("node1/node2/leaf") */
	TreePosition( const QString & pathStr ) : QStringList( pathStr.split( '/', QString::SkipEmptyParts ) ) {}

	/** converts TreePosition back to string in file-system format ("node1/node2/leaf") */
	QString toString() const { return this->join('/'); }
};


/** Simple single-column icon-less tree model. */
class DirTreeModel : public QAbstractItemModel {

	FSNode * _rootNode;  ///< internal node that stores all the other nodes without an explicit parent
	QString & _baseDir;  ///< directory from which the MIME URLs are derived when items are dragged from this model
	                    // TODO: this should be value and it should be instead of mapDir member in MainWindow
 public:

	DirTreeModel( QString & _baseDir );
	virtual ~DirTreeModel() override;

	//-- custom methods for manipulating the tree ----------------------------------------------------------------------

	/** Note that before you start adding or deleting nodes in this model, you have to call startCompleteUpdate,
	  * and when you are finished with it, you have to call finishCompleteUpdate. */
	QModelIndex addNode( const QModelIndex & parentIndex, const QString & name, NodeType type );

	/** Note that before you start adding or deleting nodes in this model, you have to call startCompleteUpdate,
	  * and when you are finished with it, you have to call finishCompleteUpdate. */
	void clear();

	/** position in the tree that can be used as a persistent node identifier that survives node shifting, adding or removal */
	TreePosition getNodePosition( const QModelIndex & index ) const;

	/** attempts to find a node at a specified position, returns invalid model index when it doesn't exist */
	QModelIndex getNodeByPosition( const TreePosition & pos ) const;

	/** returns file-system path of a node selected by model index */
	QString getFSPath( const QModelIndex & index ) const;

	void traverseNodes( std::function< void ( const QModelIndex & index ) > doOnNode,
	                    const QModelIndex & parentIndex = QModelIndex() ) const;

	NodeType getType( const QModelIndex & index ) const;
	bool isDir( const QModelIndex & index ) const          { return getType( index ) == NodeType::DIR; }
	bool isFile( const QModelIndex & index ) const         { return getType( index ) == NodeType::FILE; }

	//-- data change notifications -------------------------------------------------------------------------------------

	/** Notifies Qt that all the model indexes and data retrieved before are no longer valid.
	  * Call this before every model update. */
	void startCompleteUpdate();

	/** Call this when an update process is finished and makes a view re-draw its content according to the new data. */
	void finishCompleteUpdate();

	//-- implementation of QAbstractItemModel's virtual methods --------------------------------------------------------

	virtual int rowCount( const QModelIndex & parentIndex = QModelIndex() ) const override;
	virtual int columnCount( const QModelIndex & parentIndex = QModelIndex() ) const override;
	virtual Qt::ItemFlags flags( const QModelIndex & index ) const override;
	virtual QVariant data( const QModelIndex & index, int role ) const override;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role ) const override;
	virtual QModelIndex index( int row, int column, const QModelIndex & parentIndex = QModelIndex() ) const override;
	virtual QModelIndex parent( const QModelIndex & index ) const override;
	virtual QMimeData * mimeData( const QModelIndexList & indexes ) const override;

	//-- miscellaneous -------------------------------------------------------------------------------------------------

	QModelIndex makeIndex( int row, const QModelIndex & parentIndex ) const
	{
		return index( row, 0, parentIndex );
	}

 private: // helpers

	FSNode * modelIndexToNode( const QModelIndex & index ) const;
	QModelIndex nodeToModelIndex( FSNode * node, int rowInParent = -1 ) const;

};


#endif // DIR_TREE_MODEL_INCLUDED
