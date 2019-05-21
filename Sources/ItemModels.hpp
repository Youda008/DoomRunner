//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  13.5.2019
// Description:
//======================================================================================================================

#ifndef ITEM_MODELS_INCLUDED
#define ITEM_MODELS_INCLUDED


#include "Common.hpp"

#include <QAbstractListModel>


//======================================================================================================================
/** abstract wrapper around list of arbitrary objects, mediating their names to UI view elements */

template< typename Object >
class AObjectListModel : public QAbstractListModel {

 protected:

	QList< Object > & objectList;

 public:

	AObjectListModel( QList< Object > & objectList ) : QAbstractListModel( nullptr ), objectList( objectList ) {}

	QList< Object > & list() const { return objectList; }

	int rowCount( const QModelIndex & ) const override
	{
		return objectList.size();
	}

	void updateUI( int changeBeginIdx )
	{
		emit dataChanged( createIndex( changeBeginIdx, 0 ), createIndex( objectList.size() - 1, 0 ), {Qt::DisplayRole} );
	}

};


//======================================================================================================================
/** wrapper around list of arbitrary objects, mediating their content to UI view elements with read-only access */

template< typename Object, QString (* makeUIText)( const Object & ) >
class ObjectListROModel : public AObjectListModel< Object > {

 public:

	using AObjectListModel< Object >::AObjectListModel;

	QVariant data( const QModelIndex & index, int role ) const override
	{
		if (!index.isValid() || index.row() >= AObjectListModel<Object>::objectList.size())
			return QVariant();
		if (role != Qt::DisplayRole)
			return QVariant();
		return makeUIText( AObjectListModel<Object>::objectList[ index.row() ] );
	}

};


//======================================================================================================================
/**  wrapper around list of arbitrary objects, mediating their names to UI view elements with editing capability */

template< typename Object >  // Object must contain attribute .name
class ObjectListRWModel : public AObjectListModel< Object > {

 public:

	using AObjectListModel< Object >::AObjectListModel;

	Qt::ItemFlags flags( const QModelIndex & index ) const override
	{
		if (!index.isValid())
			return Qt::ItemIsEnabled;
		return QAbstractItemModel::flags( index ) | Qt::ItemIsEditable;
	}

	QVariant data( const QModelIndex & index, int role ) const override
	{
		if (!index.isValid() || index.row() >= AObjectListModel<Object>::objectList.size())
			return QVariant();
		if (role != Qt::DisplayRole && role != Qt::EditRole)
			return QVariant();
		return AObjectListModel<Object>::objectList[ index.row() ].name;
	}

	bool setData( const QModelIndex & index, const QVariant & value, int role )
	{
		if (!index.isValid())
			return false;
		if (role != Qt::EditRole)
			return false;
		AObjectListModel<Object>::objectList[ index.row() ].name = value.toString();
		emit AObjectListModel<Object>::dataChanged( index, index, {role} );
		return true;
	}

};


#endif // ITEM_MODELS_INCLUDED
