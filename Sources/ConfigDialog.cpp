//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  25.1.2021
// Description:
//======================================================================================================================

#include "ConfigDialog.hpp"
#include "ui_ConfigDialog.h"


//======================================================================================================================

ConfigDialog::ConfigDialog( QWidget * parent, const QString & currentConfigName )
:
	QDialog( parent )
{
	ui = new Ui::ConfigDialog;
	ui->setupUi(this);

	ui->configNameLine->setText( currentConfigName );

	connect( this, &QDialog::accepted, this, &thisClass::confirmed );
}

void ConfigDialog::confirmed()
{
	newConfigName = ui->configNameLine->text();
}

ConfigDialog::~ConfigDialog()
{
	delete ui;
}
