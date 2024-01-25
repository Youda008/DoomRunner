//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: asynchronous update checking tool
//======================================================================================================================

#ifndef UPDATE_CHECKER_INCLUDED
#define UPDATE_CHECKER_INCLUDED


#include "Essential.hpp"
#include "CommonTypes.hpp"
#include "Utils/ErrorHandling.hpp"  // LoggingComponent

#include <QObject>
#include <QString>
#include <QHash>
#include <QNetworkAccessManager>

#include <functional>


//======================================================================================================================
/// Asynchronous update checking tool.
/** The object must live until a response is received, i.e. it can't be local in a function. */

class UpdateChecker : public QObject, protected LoggingComponent {

	Q_OBJECT

 public:

	UpdateChecker();
	virtual ~UpdateChecker() override;

	enum Result
	{
		ConnectionFailed,
		InvalidFormat,
		UpdateAvailable,
		UpdateNotAvailable
	};

	using ResultCallback = std::function< void ( Result result, QString errorDetail, QStringVec versionInfo ) >;

	/// Asynchronously checks for updates via HTTP connection and calls your callback when it's ready.
	void checkForUpdates_async( ResultCallback && callback );

 private:

	void requestFinished( QNetworkReply * reply );

	// one update check consists of 2 phases - request to version file and request to changelog
	enum class Phase
	{
		VersionRequest,
		ChangelogRequest,
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

bool showUpdateNotification( QWidget * parent, const QStringVec & versionInfo, bool includeCheckbox );


#endif // UPDATE_CHECKER_INCLUDED
