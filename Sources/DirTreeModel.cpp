//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  11.4.2020
// Description: model representing files inside a directory hierarchy
//======================================================================================================================

#include "DirTreeModel.hpp"

#include <QList>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QDir>

#include <QDebug>


//======================================================================================================================
//  DirTreeModel

DirTreeModel::DirTreeModel( QString & baseDir )
	: QAbstractItemModel( nullptr )
	, _baseDir( baseDir )
{
	_rootNode = new FSNode( "", NodeType::DIR, nullptr );
}

DirTreeModel::~DirTreeModel()
{
	delete _rootNode;  // recursively deletes all the child nodes
}

//----------------------------------------------------------------------------------------------------------------------
//  custom methods for manipulating the tree

QModelIndex DirTreeModel::addNode( const QModelIndex & parentIndex, const QString & name, NodeType type )
{
	FSNode * parent = modelIndexToNode( parentIndex );
	return nodeToModelIndex( parent->addChild( name, type ) );
}

void DirTreeModel::clear()
{
	_rootNode->deleteChildren();
}

TreePosition DirTreeModel::getNodePosition( const QModelIndex & index ) const
{
	TreePosition path;

	FSNode * node = modelIndexToNode( index );
	while (node != _rootNode) {
		path.append( node->name() );
		node = node->parent();
	}
	reverse( path );

	return path;
}

QModelIndex DirTreeModel::getNodeByPosition( const TreePosition & path ) const
{
	FSNode * node = _rootNode;
	for (const QString & nodeName : path) {
		node = node->child( nodeName );  // linear complexity, but we expect the sublists to be small
		if (!node)
			return QModelIndex();  // node at this path no longer exists
	}
	return nodeToModelIndex( node );
}

QString DirTreeModel::getFSPath( const QModelIndex & index ) const
{
	return _baseDir+'/'+getNodePosition( index ).join('/');
}

void DirTreeModel::traverseNodes( std::function< void ( const QModelIndex & index ) > doOnNode,
                                  const QModelIndex & parentIndex ) const
{
	FSNode * const parent = modelIndexToNode( parentIndex );

	for (int childRow = 0; childRow < parent->childCount(); childRow++)
	{
		FSNode * const child = parent->child( childRow );
		const QModelIndex childIndex = nodeToModelIndex( child, childRow );

		doOnNode( childIndex );

		traverseNodes( doOnNode, childIndex );
	}
}

NodeType DirTreeModel::getType( const QModelIndex & index ) const
{
	FSNode * node = modelIndexToNode( index );
	return node->type();
}

//----------------------------------------------------------------------------------------------------------------------
//  data change notifications

void DirTreeModel::startCompleteUpdate()
{
	beginResetModel();
}

void DirTreeModel::finishCompleteUpdate()
{
	endResetModel();
}

//----------------------------------------------------------------------------------------------------------------------
//  implementation of QAbstractItemModel's virtual methods

int DirTreeModel::rowCount( const QModelIndex & parentIndex ) const
{
	FSNode * const parent = modelIndexToNode( parentIndex );
	return parent->childCount();
}

int DirTreeModel::columnCount( const QModelIndex & /*parentIndex*/ ) const
{
	return 1;
}

Qt::ItemFlags DirTreeModel::flags( const QModelIndex & index ) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;

	Qt::ItemFlags flags = QAbstractItemModel::flags( index );
	if (isFile( index ))
		flags |= Qt::ItemIsDragEnabled;

	return flags;
}

QVariant DirTreeModel::data( const QModelIndex & index, int role ) const
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

QVariant DirTreeModel::headerData( int /*section*/, Qt::Orientation /*orientation*/, int /*role*/ ) const
{
	return QVariant();  // no header support
}

QModelIndex DirTreeModel::index( int row, int column, const QModelIndex & parentIndex ) const
{
	if (!hasIndex( row, column, parentIndex ))  // checks bounds (>= 0 && < rowCount) for given parent
		return QModelIndex();

	FSNode * const parent = modelIndexToNode( parentIndex );
	FSNode * const sibling = parent->child( row );
	return createIndex( row, 0, sibling );
}

QModelIndex DirTreeModel::parent( const QModelIndex & index ) const
{
	if (!index.isValid())
		return QModelIndex();

	FSNode * const node = modelIndexToNode( index );
	FSNode * const parent = node->parent();
	return nodeToModelIndex( parent );
}

// serializes items into MIME URLs as if they were dragged from a directory window
QMimeData * DirTreeModel::mimeData( const QModelIndexList & indexes ) const
{
	QMimeData * mimeData = new QMimeData;

	QList< QUrl > urls;
	for (const QModelIndex & index : indexes) {
		urls.append( QUrl::fromLocalFile( getFSPath( index ) ) );
	}
	mimeData->setUrls( urls );

	return mimeData;
}

//----------------------------------------------------------------------------------------------------------------------
//  private helpers

// Internally we use TreeNode pointers, but the view accesses the model via QModelIndex.
// If the view passes in an empty index, it wants an item from the top level, an item that doesn't have any parent.
// We store such elements under a root node (default parent), because it simplifies the implementation.

FSNode * DirTreeModel::modelIndexToNode( const QModelIndex & index ) const
{
	// if no parent is specified, use our internal default parent
	if (!index.isValid())
		return _rootNode;
	else
		return static_cast< FSNode * >( index.internalPointer() );
}

QModelIndex DirTreeModel::nodeToModelIndex( FSNode * node, int rowInParent ) const
{
	// optimization for cases where the row index is known, otherwise we have to perform linear lookup at the parent
	const int row = rowInParent >= 0 ? rowInParent : node->row();

	// root node is internal only, don't expose it to the outside, for the caller it means having no parent at all
	if (node == _rootNode)
		return QModelIndex();
	else
		return createIndex( row, 0, node );
}
