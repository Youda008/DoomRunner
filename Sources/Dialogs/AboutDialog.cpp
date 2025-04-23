//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of the About dialog that appears when you click Menu -> About
//======================================================================================================================

#include "AboutDialog.hpp"
#include "ui_AboutDialog.h"

#include "Version.hpp"
#include "UpdateChecker.hpp"
#include "Utils/ErrorHandling.hpp"

#include <QStringBuilder>


//======================================================================================================================

AboutDialog::AboutDialog( QWidget * parent, bool checkForUpdates )
:
	QDialog( parent ),
	DialogCommon( this, u"AboutDialog" ),
	checkForUpdates( checkForUpdates )
{
	ui = new Ui::AboutDialog;
	ui->setupUi( this );

	ui->appLabel->setText( ui->appLabel->text().arg( appVersion ) );
	ui->qtLabel->setText( ui->qtLabel->text().arg( qtVersion ) );

	ui->checkUpdatesChkBox->setChecked( checkForUpdates );

	connect( ui->checkUpdatesChkBox, &QCheckBox::toggled, this, &thisClass::onUpdateCheckingToggled );
	connect( ui->checkUpdateBtn, &QPushButton::clicked, this, &thisClass::checkForUpdate );
}

AboutDialog::~AboutDialog()
{
	delete ui;
}

void AboutDialog::onUpdateCheckingToggled( bool enabled )
{
	checkForUpdates = enabled;
}

void AboutDialog::checkForUpdate()
{
	// let the user know that the request is pending
	QString origText = ui->checkUpdateBtn->text();
	ui->checkUpdateBtn->setText("Checking...");
	ui->checkUpdateBtn->setEnabled( false );  // prevent him spamming the button and starting many requests simultaneously

	updateChecker.checkForUpdates_async(
		[ this, origText ]( UpdateChecker::Result result, QString errorDetail, QStringList versionInfo )
		{
			// request finished, restore the button
			ui->checkUpdateBtn->setText( origText );
			ui->checkUpdateBtn->setEnabled( true );

			switch (result) {
			 case UpdateChecker::ConnectionFailed:
				reportRuntimeError( "Update check failed",
					"Failed to connect to the project web page. Is your internet down?"
					"Details: "%errorDetail
				);
				break;
			 case UpdateChecker::InvalidFormat:
				reportLogicError( u"checkForUpdate", "Update check failed",
					"Version number from github is in invalid format: "%errorDetail
				);
				break;
			 case UpdateChecker::UpdateNotAvailable:
				reportInformation( "No update available",
					"No update is available, you have the newest version."
				);
				break;
			 case UpdateChecker::UpdateAvailable:
				showUpdateNotification( this, versionInfo, /*checkbox*/false );
				break;
			 default:
				reportLogicError( u"checkForUpdate", "Update check failed",
					"UpdateChecker::Result is out of bounds: "%QString::number( fut::to_underlying(result) )
				);
				break;
			}
		}
	);
}
