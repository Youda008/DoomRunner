//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: asynchronous update checking tool
//======================================================================================================================

#include "UpdateChecker.hpp"

#include "Version.hpp"
#include "Themes.hpp"  // updateWindowBorder
#include "Utils/LangUtils.hpp"  // atScopeEndDo
#include "Utils/WidgetUtils.hpp"  // HYPERLINK

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegularExpression>

#include <QStringBuilder>
#include <QMessageBox>
#include <QGridLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QTextBrowser>


//======================================================================================================================
//  UpdateChecker

static const QString availableVersionUrl = "https://raw.githubusercontent.com/Youda008/DoomRunner/master/version.txt";
static const QString releasePageUrl = "https://github.com/Youda008/DoomRunner/releases";
static const QString changelogUrl = "https://raw.githubusercontent.com/Youda008/DoomRunner/master/changelog.txt";


UpdateChecker::UpdateChecker()
{
	QObject::connect( &manager, &QNetworkAccessManager::finished, this, &UpdateChecker::requestFinished );
}

UpdateChecker::~UpdateChecker() {}

void UpdateChecker::checkForUpdates_async( ResultCallback && callback )
{
	QNetworkRequest request;
	request.setUrl( availableVersionUrl );
	QNetworkReply * reply = manager.get( request );

	pendingRequests[ reply ] = { Phase::VersionRequest, {}, std::move(callback) };
}

void UpdateChecker::requestFinished( QNetworkReply * reply )
{
	auto requestIter = pendingRequests.find( reply );
	if (requestIter == pendingRequests.end())
	{
		logLogicError("UpdateChecker") << "This reply does not have a registered callback, wtf?";
		return;
	}
	RequestData & requestData = requestIter.value();

	auto guard = atScopeEndDo( [&](){ pendingRequests.erase( requestIter ); } );

	if (reply->error())
	{
		//logRuntimeError("UpdateChecker") << "HTTP request failed: " << reply->errorString();  // handled in callback when appropriate
		requestData.callback( ConnectionFailed, reply->errorString(), {} );
		return;
	}

	if (requestData.phase == Phase::VersionRequest)
	{
		versionReceived( reply, requestData );
	}
	else
	{
		changelogReceived( reply, requestData );
	}
}

void UpdateChecker::versionReceived( QNetworkReply * reply, RequestData & requestData )
{
	QString version( reply->readLine( 16 ) );

	static const QRegularExpression versionFileRegex("^\"([0-9\\.]+)\"$");
	auto match = versionFileRegex.match( version );
	if (!match.hasMatch())
	{
		logLogicError("UpdateChecker").noquote() << "Version number from github is in invalid format ("<<version<<"). Fix it!";
		requestData.callback( InvalidFormat, std::move(version), {} );
		return;
	}
	QString availableVersionStr = match.captured(1);
	Version availableVersion( availableVersionStr );

	bool updateAvailable = availableVersion > appVersion;
	if (!updateAvailable)
	{
		requestData.callback( UpdateNotAvailable, {}, { availableVersionStr } );
		return;
	}

	QNetworkRequest request;
	request.setUrl( changelogUrl );
	QNetworkReply * reply2 = manager.get( request );

	pendingRequests[ reply2 ] = { Phase::ChangelogRequest, std::move(availableVersionStr), std::move(requestData.callback) };
}

// fucking Qt, are you fucking kidding me?
static QString getLine( QNetworkReply * reply )
{
	QString line( reply->readLine() );  // what fucking sense does it make to include the '\n' char at every line
	line.chop(1);                       // so that the user has to chop it himself???
	return line;
}

void UpdateChecker::changelogReceived( QNetworkReply * reply, RequestData & requestData )
{
	/* changelog format
	1.5
	- tool-buttons got icons instead of symbols
	- added button to add a directory of mods
	- added button to create a new config from an existing one

	1.4
	- added new launch options for video, audio and save/screenshot directory
	- added tooltips for some of the launch options
	- map names are now extracted from IWAD file instead of guessing them from IWAD name

	1.3
	...
	*/

	QString line;
	QStringVec versionInfo;

	// find the line with the new version
	while ((line = getLine( reply )) != requestData.newVersion && !reply->atEnd()) {}
	versionInfo.append( line );

	// get all changes until our current version
	while ((line = getLine( reply )) != appVersion && !reply->atEnd())
		versionInfo.append( std::move(line) );

	// finally, call the user callback with all the data
	requestData.callback( UpdateAvailable, {}, std::move(versionInfo) );
}


//======================================================================================================================
//  common result reactions

struct NewElements
{
	QLabel * firstLabel = nullptr;
	QTextBrowser * textBrowser = nullptr;
	QLabel * secondLabel = nullptr;

	operator bool() const { return firstLabel && textBrowser && secondLabel; }
};
static NewElements reworkLayout( QMessageBox & msgBox )
{
	NewElements newElements;

	// We need to do all of this mess just to customize the content of the message box and add a text field.
	// Beware: This code is kinda fragile since it depends on the exact implementation of QMessageBox and its layout.

	QGridLayout * layout = qobject_cast< QGridLayout * >( msgBox.layout() );
	if (!layout)
	{
		logLogicError("UpdateChecker") << "MessageBox doesn't use grid layout, wtf?";
		return newElements;
	}

	/* the original layout looks like this
	 QIcon             QSpacerItem       QLabel
	 QIcon             QSpacerItem      (QCheckBox)
	(QSpacerItem       nullptr           nullptr)
	 QDialogButtonBox  QDialogButtonBox  QDialogButtonBox
	*/
	/* but we want it like this
	 QIcon             QSpacerItem       QLabel
	 QIcon             QSpacerItem       QTextBrowser
	 nullptr           nullptr           QLabel
	 nullptr           nullptr          (QCheckBox)
	(QSpacerItem       nullptr           nullptr)
	 QDialogButtonBox  QDialogButtonBox  QDialogButtonBox
	*/

	int origLastRow = layout->rowCount() - 1;

	// move button box 2 rows down
	QDialogButtonBox * btnBox = msgBox.findChild< QDialogButtonBox * >();
	if (!btnBox)
	{
		logLogicError("UpdateChecker") << "MessageBox doesn't have button box, wtf?";
		return newElements;
	}
	layout->removeWidget( btnBox );
	layout->addWidget( btnBox, origLastRow + 2, 0, 1, layout->columnCount() );

	// move checkbox and its related layout items 2 rows down
	QCheckBox * chkBox = msgBox.findChild< QCheckBox * >();
	if (chkBox)
	{
		int boxRow, boxColumn, boxRowSpan, boxColumnSpan;
		layout->getItemPosition( layout->indexOf( chkBox ), &boxRow, &boxColumn, &boxRowSpan, &boxColumnSpan );
		layout->removeWidget( chkBox );
		layout->addWidget( chkBox, boxRow + 2, boxColumn, boxRowSpan, boxColumnSpan, Qt::AlignLeft );

		for (int itemColumn = 0; itemColumn < layout->columnCount(); ++itemColumn)
		{
			QLayoutItem * item = layout->itemAtPosition( boxRow + 1, itemColumn );
			if (item)
			{
				layout->removeItem( item );
				layout->addItem( item, boxRow + 3, itemColumn, 1, 1 );  // we asume only 1x1 items, hopefully this is not gonna change
			}
		}
	}

	// find the original label
	newElements.firstLabel = msgBox.findChild< QLabel * >( "qt_msgbox_label" );
	if (!newElements.firstLabel)
	{
		logLogicError("UpdateChecker") << "MessageBox doesn't have this label, incorrect name?";
		return newElements;
	}
	int labelRow, labelColumn, labelRowSpan, labelColumnSpan;
	layout->getItemPosition( layout->indexOf( newElements.firstLabel ), &labelRow, &labelColumn, &labelRowSpan, &labelColumnSpan );

	// add new elements under the original label
	newElements.textBrowser = new QTextBrowser;
	newElements.textBrowser->setMinimumSize( 500, 200 );
	newElements.textBrowser->setLineWrapMode( QTextBrowser::LineWrapMode::WidgetWidth );
	layout->addWidget( newElements.textBrowser, labelRow + 1, labelColumn, 1, 1 );

	newElements.secondLabel = new QLabel;
	newElements.secondLabel->setOpenExternalLinks( true );
	layout->addWidget( newElements.secondLabel, labelRow + 2, labelColumn, 1, 1 );

	return newElements;
}

bool showUpdateNotification( QWidget * parent, const QStringVec & versionInfo, bool includeCheckbox )
{
	QString newVersion = versionInfo.first();

	QMessageBox msgBox( QMessageBox::Information, "Update available", {}, QMessageBox::Ok, parent );

	// On Windows we need to manually make title bar of every new window dark, if dark theme is used.
	themes::updateWindowBorder( &msgBox );

	// add checkbox for automatic update checks
	QCheckBox chkBox( "Check for updates on every start" );
	if (includeCheckbox)
	{
		chkBox.setChecked( true );  // if this was called with includeCheckbox, it must be true
		msgBox.setCheckBox( &chkBox );
	}

	NewElements newElements = reworkLayout( msgBox );

	if (newElements)
	{
		newElements.firstLabel->setText(
			"<html><head/><body>"
			"<p>"
				"Version "%newVersion%" is available."
			"</p><p>"
				"Here is what's new."
			"</p>"
			"</body></html>"
		);

		newElements.textBrowser->setText( versionInfo.join('\n') );

		newElements.secondLabel->setText(
			"<html><head/><body>"
			"<p>"
				"You can download it at " HYPERLINK( releasePageUrl, releasePageUrl ) "."
			"</p>"
			"</body></html>"
		);
	}
	else  // layout rework failed
	{
		msgBox.setText(
			"<html><head/><body>"
			"<p>"
				"Version "%newVersion%" is available."
			"</p><p>"
				"You can download it at<br>"
				HYPERLINK( releasePageUrl, releasePageUrl ) "."
			"</p><p>"
				"Bellow you can see what's new."
			"</p>"
			"</body></html>"
		);

		// show changelog at least in the message box details
		msgBox.setDetailedText( versionInfo.join('\n') );

		// automatically expand the details section
		const QList< QAbstractButton * > buttons = msgBox.buttons();
		for (QAbstractButton * button : buttons)
		{
			if (button->text().startsWith("Show Details"))
			{
				button->click();
				break;
			}
		}
	}

	msgBox.exec();

	return chkBox.isChecked();
}
