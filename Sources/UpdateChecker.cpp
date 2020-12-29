//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  1.5.2020
// Description: asynchronous update checking tool
//======================================================================================================================

#include "UpdateChecker.hpp"

#include "Version.hpp"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegularExpression>

#include <QStringBuilder>
#include <QDebug>
#include <QMessageBox>
#include <QCheckBox>


//======================================================================================================================
//  UpdateChecker

static const QString availableVersionUrl = "https://raw.githubusercontent.com/Youda008/DoomRunner/master/version.txt";
static const QString releasePageUrl = "https://github.com/Youda008/DoomRunner/releases";

UpdateChecker::UpdateChecker()
{
	QObject::connect( &manager, &QNetworkAccessManager::finished, this, &UpdateChecker::requestFinished );
}

UpdateChecker::~UpdateChecker() {}

void UpdateChecker::checkForUpdates( const ResultCallback & callback )
{
	QNetworkRequest request;
	request.setUrl( availableVersionUrl );

	QNetworkReply * reply = manager.get( request );

	registeredCallbacks[ reply ] = callback;
}

void UpdateChecker::requestFinished( QNetworkReply * reply )
{
	auto callbackIter = registeredCallbacks.find( reply );
	if (callbackIter == registeredCallbacks.end())
	{
		qWarning() << "This reply does not have a registered callback, wtf?";
		return;
	}

	ResultCallback & callback = callbackIter.value();

	if (reply->error())
	{
		qWarning() << "HTTP request failed: " << reply->errorString();
		callback( CONNECTION_FAILED, reply->errorString() );
		return;
	}

	QString version( reply->readLine( 16 ) );

	QRegularExpressionMatch match = QRegularExpression("^\"([0-9\\.]+)\"$").match( version );
	if (!match.hasMatch())
	{
		qWarning() << "Version number from github is in invalid format ("%version%"). Fix it!";
		callback( INVALID_FORMAT, version );
		return;
	}
	QString availableVersion = match.captured(1);

	bool updateAvailable = compareVersions( availableVersion, appVersion ) > 0;

	callback( updateAvailable ? UPDATE_AVAILABLE : UPDATE_NOT_AVAILABLE, availableVersion );
}

bool showUpdateNotification( QWidget * parent, const QString & newVersion, bool includeCheckbox )
{
	QMessageBox msgBox( QMessageBox::Information, "Update available",
		"<html><head/><body>"
		"<p>"
			"Version "%newVersion%" is available. You can download it here:<br>"
			"<a href=\""%releasePageUrl%"\">"
			"<span style=\" text-decoration: underline; color:#0000ff;\">"
				%releasePageUrl%
			"</span>"
			"</a>"
		"</p>"
		"</body></html>",
		QMessageBox::Ok,
		parent
	);

	QCheckBox chkBox( "Check for updates on every start" );
	chkBox.setChecked( true );
	if (includeCheckbox)
		msgBox.setCheckBox( &chkBox );

	msgBox.exec();

	return chkBox.isChecked();
}
