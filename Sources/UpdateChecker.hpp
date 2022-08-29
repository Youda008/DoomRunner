//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: asynchronous update checking tool
//======================================================================================================================

#ifndef UPDATE_CHECKER_INCLUDED
#define UPDATE_CHECKER_INCLUDED


#include "Common.hpp"

#include <QObject>
#include <QString>
#include <QHash>
#include <QNetworkAccessManager>

#include <functional>


//======================================================================================================================
/// Asynchronous update checking tool.
/** The object must live until a response is received, i.e. it can't be local in a function. */

class UpdateChecker : public QObject {

	Q_OBJECT

 public:

	UpdateChecker();
	virtual ~UpdateChecker() override;

	enum Result
	{
		CONNECTION_FAILED,
		INVALID_FORMAT,
		UPDATE_AVAILABLE,
		UPDATE_NOT_AVAILABLE
	};

	using ResultCallback = std::function< void ( Result result, QString errorDetail, QStringList versionInfo ) >;

	/// Asynchronously checks for updates via HTTP connection and calls your callback when it's ready.
	void checkForUpdates_async( ResultCallback && callback );

 private:

	void requestFinished( QNetworkReply * reply );

	// one update check consists of 2 phases - request to version file and request to changelog
	enum class Phase
	{
		VERSION_REQUEST,
		CHANGELOG_REQUEST,
	};
	struct RequestData
	{
		Phase phase;
		QString newVersion;
		ResultCallback callback;
	};

	void versionReceived( QNetworkReply * reply, RequestData & requestData );
	void changelogReceived( QNetworkReply * reply, RequestData & requestData );

 private:

	QNetworkAccessManager manager;

	QHash< QNetworkReply *, RequestData > pendingRequests;

};


//======================================================================================================================
//  common result reactions

bool showUpdateNotification( QWidget * parent, QStringList versionInfo, bool includeCheckbox );


#endif // UPDATE_CHECKER_INCLUDED
