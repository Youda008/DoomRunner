//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of the New Config dialog that appears when you click the Clone Config button
//======================================================================================================================

#include "NewConfigDialog.hpp"
#include "ui_NewConfigDialog.h"


//======================================================================================================================

NewConfigDialog::NewConfigDialog( QWidget * parent, const QFileInfo & origConfigFile )
:
	QDialog( parent ),
	DialogCommon( this )
{
	ui = new Ui::NewConfigDialog;
	ui->setupUi(this);

	ui->infoLabel->setText( ui->infoLabel->text().replace( "<orig_config_file_name>", origConfigFile.fileName() ) );
	ui->suffixLabel->setText( ui->suffixLabel->text().replace( "<config_suffix>", origConfigFile.suffix() ) );
	ui->configNameLine->setText( origConfigFile.completeBaseName() );

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
