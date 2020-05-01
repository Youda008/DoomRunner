//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  15.2.2020
// Description:
//======================================================================================================================

#include "AboutDialog.hpp"
#include "ui_AboutDialog.h"

#include "Version.hpp"
#include "UpdateChecker.hpp"

#include <QMessageBox>
#include <QStringBuilder>


//======================================================================================================================

AboutDialog::AboutDialog( QWidget * parent, bool checkForUpdates )
:
	QDialog( parent ),
	checkForUpdates( checkForUpdates )
{
	ui = new Ui::AboutDialog;
	ui->setupUi( this );

	ui->appLabel->setText( ui->appLabel->text().arg( appVersion ) );
	ui->qtLabel->setText( ui->qtLabel->text().arg( qtVersion ) );

	ui->checkUpdatesChkBox->setChecked( checkForUpdates );

	connect( ui->checkUpdatesChkBox, &QCheckBox::toggled, this, &thisClass::toggleUpdateChecking );
	connect( ui->checkUpdateBtn, &QPushButton::clicked, this, &thisClass::checkForUpdate );
}

AboutDialog::~AboutDialog()
{
	delete ui;
}

void AboutDialog::toggleUpdateChecking( bool enabled )
{
	checkForUpdates = enabled;
}

void AboutDialog::checkForUpdate()
{
	// let the user know that the request is pending
	QString origText = ui->checkUpdateBtn->text();
	ui->checkUpdateBtn->setText("Checking...");
	ui->checkUpdateBtn->setEnabled( false );  // prevent him spamming the button and starting many requests simultaneously

	updateChecker.checkForUpdates(
		[ this, origText ]( UpdateChecker::Result result, const QString & detail )
		{
			// request finished, restore the button
			ui->checkUpdateBtn->setText( origText );
			ui->checkUpdateBtn->setEnabled( true );

			switch (result) {
			 case UpdateChecker::CONNECTION_FAILED:
				QMessageBox::warning( nullptr, "Update check failed",
					"Failed to connect to the project web page. Is your internet down?"
					"Details: "%detail
				);
				break;
			 case UpdateChecker::INVALID_FORMAT:
				QMessageBox::warning( nullptr, "Update check failed",
					"Version number from github is in invalid format: "%detail
				);
				break;
			 case UpdateChecker::UPDATE_NOT_AVAILABLE:
				QMessageBox::information( nullptr, "No update available",
					"No update is available, you have the newest version."
				);
				break;
			 case UpdateChecker::UPDATE_AVAILABLE:
				showUpdateNotification( this, detail, false );
				break;
			}
		}
	);
}
