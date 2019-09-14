//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  13.5.2019
// Description:
//======================================================================================================================

#include "EngineDialog.hpp"
#include "ui_EngineDialog.h"

#include "Utils.hpp"

#include <QFileDialog>


//======================================================================================================================

EngineDialog::EngineDialog( QWidget * parent, PathHelper & pathHelper, QString & name, QString & path )

	: QDialog( parent )
	, pathHelper( pathHelper )
	, name( name )
	, path( path )
{
	ui = new Ui::EngineDialog;
	ui->setupUi(this);

	ui->nameLine->setText( name );
	ui->pathLine->setText( path );

	connect( ui->browseBtn, &QPushButton::clicked, this, &thisClass::browseEngine );

	connect( ui->buttonBox, &QDialogButtonBox::accepted, this, &thisClass::confirm );
	connect( ui->buttonBox, &QDialogButtonBox::rejected, this, &thisClass::cancel );
}

EngineDialog::~EngineDialog()
{
	delete ui;
}

void EngineDialog::browseEngine()
{
	QString path = QFileDialog::getOpenFileName( this, "Locate engine's executable", QString(),
 #ifdef _WIN32
		"Executable files (*.exe);;"
 #endif
		"All files (*)"
	);
	if (path.length() == 0)  // user probably clicked cancel
		return;

	// the path comming out of the file dialog is always absolute
	if (pathHelper.useRelativePaths())
		path = pathHelper.getRelativePath( path );

	ui->pathLine->setText( path );
	ui->nameLine->setText( QFileInfo( path ).dir().dirName() );
	// we will write those values to data storage when dialog is confirmed because user might want to cancel it
}

void EngineDialog::confirm()
{
	name = ui->nameLine->text();
	path = ui->pathLine->text();
}

void EngineDialog::cancel()
{

}
