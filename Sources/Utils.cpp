//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  13.5.2019
// Description: miscellaneous utilities
//======================================================================================================================

#include "Utils.hpp"

#include <QListView>
#include <QMessageBox>


//======================================================================================================================
//  list view helpers

int getSelectedItemIdx( QListView * view )   // this function is for single selection lists
{
	QModelIndexList selectedIndexes = view->selectionModel()->selectedRows();
	if (selectedIndexes.empty()) {
		return -1;
	}
	if (selectedIndexes.size() > 1) {
		QMessageBox::critical( view->parentWidget(), "Multiple items selected",
			"Multiple items are selected. This shouldn't be happening and it is a bug. Please create an issue on Github page." );
		return -1;
	}
	return selectedIndexes[0].row();
}

void selectItemByIdx( QListView * view, int index )
{
	QModelIndex modelIndex = view->model()->index( index, 0 );
	view->selectionModel()->select( modelIndex, QItemSelectionModel::Select );
	view->selectionModel()->setCurrentIndex( modelIndex, QItemSelectionModel::NoUpdate );
}

void deselectItemByIdx( QListView * view, int index )
{
	QModelIndex modelIndex = view->model()->index( index, 0 );
	view->selectionModel()->select( modelIndex, QItemSelectionModel::Deselect );
}

void deselectSelectedItem( QListView * view )
{
	int selectedIdx = getSelectedItemIdx( view );
	if (selectedIdx >= 0)
		deselectItemByIdx( view, selectedIdx );
}

void changeSelectionTo( QListView * view, int index )
{
	deselectSelectedItem( view );
	selectItemByIdx( view, index );
}


//======================================================================================================================
//  list widget helpers

void appendItem( QListWidget * widget, const QString & text, bool checkable, Qt::CheckState initialState  )
{
	// create and add the item
	QListWidgetItem * item = new QListWidgetItem();
	item->setData( Qt::DisplayRole, text );
	if (checkable) {
		item->setFlags( item->flags() | Qt::ItemFlag::ItemIsUserCheckable );
		item->setCheckState( initialState );
	}
	widget->addItem( item );

	// change the selection to the new item
	int selectedIdx = getSelectedItemIdx( widget );
	if (selectedIdx >= 0)   // deselect currently selected item, if any
		deselectItemByIdx( widget, selectedIdx );
	selectItemByIdx( widget, widget->count() - 1 );
}

void deleteSelectedItem( QListWidget * widget )
{
	int selectedIdx = getSelectedItemIdx( widget );
	if (selectedIdx < 0) {  // if no item is selected
		if (widget->count() > 0)
			QMessageBox::warning( widget->parentWidget(), "No item selected", "No item is selected." );
		return;
	}

	// update selection
	if (selectedIdx == widget->count() - 1) {            // if item is the last one
		deselectItemByIdx( widget, selectedIdx );        // deselect it
		if (selectedIdx > 0) {                           // and if it's not the only one
			selectItemByIdx( widget, selectedIdx - 1 );  // select the previous
		}
	}

	delete widget->takeItem( selectedIdx );
}

void moveUpSelectedItem( QListWidget * widget )
{
	int selectedIdx = getSelectedItemIdx( widget );
	if (selectedIdx < 0) {  // if no item is selected
		QMessageBox::warning( widget->parentWidget(), "No item selected", "No item is selected." );
		return;
	}
	if (selectedIdx == 0) {  // if the selected item is the first one, do nothing
		return;
	}

	QListWidgetItem * item = widget->takeItem( selectedIdx );
	widget->insertItem( selectedIdx - 1, item );

	// update selection
	deselectSelectedItem( widget );  // the list widget, when item is removed, automatically selects some other one
	selectItemByIdx( widget, selectedIdx - 1 );
}

void moveDownSelectedItem( QListWidget * widget )
{
	int selectedIdx = getSelectedItemIdx( widget );
	if (selectedIdx < 0) {  // if no item is selected
		QMessageBox::warning( widget->parentWidget(), "No item selected", "No item is selected." );
		return;
	}
	if (selectedIdx == widget->count() - 1) {  // if the selected item is the last one, do nothing
		return;
	}

	QListWidgetItem * item = widget->takeItem( selectedIdx );
	widget->insertItem( selectedIdx + 1, item );

	// update selection
	deselectSelectedItem( widget );  // the list widget, when item is removed, automatically selects some other one
	selectItemByIdx( widget, selectedIdx + 1 );
}


//======================================================================================================================

QString getMapNumber( QString mapName )
{
	if (mapName.startsWith('E')) {
		return mapName[1]+QString(' ')+mapName[3];
	} else {
		return mapName.mid(3,2);
	}
}
