//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  13.5.2019
// Description:
//======================================================================================================================

#include "CompatFlagsDialog.hpp"
#include "ui_CompatFlagsDialog.h"


//======================================================================================================================

CompatFlagsDialog::CompatFlagsDialog( QWidget * parent, uint32_t & compatflags1, uint32_t & compatflags2 )

	: QDialog( parent )
	, flags1( compatflags1 )
	, flags2( compatflags2 )
{
	ui = new Ui::CompatFlagsDialog;
    ui->setupUi(this);
}

CompatFlagsDialog::~CompatFlagsDialog()
{
    delete ui;
}
