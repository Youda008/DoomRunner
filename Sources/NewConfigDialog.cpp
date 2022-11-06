//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of the New Config dialog that appears when you click the Clone Config button
//======================================================================================================================

#include "NewConfigDialog.hpp"
#include "ui_NewConfigDialog.h"

#include "ColorThemes.hpp"


//======================================================================================================================

NewConfigDialog::NewConfigDialog( QWidget * parent, const QString & currentConfigName )
:
	QDialog( parent )
{
	ui = new Ui::NewConfigDialog;
	ui->setupUi(this);

	updateWindowBorder( this );  // on Windows we need to manually make title bar of every new window dark, if dark theme is used

	ui->configNameLine->setText( currentConfigName );

	connect( this, &QDialog::accepted, this, &thisClass::confirmed );
}

void NewConfigDialog::confirmed()
{
	newConfigName = ui->configNameLine->text();
}

NewConfigDialog::~NewConfigDialog()
{
	delete ui;
}
