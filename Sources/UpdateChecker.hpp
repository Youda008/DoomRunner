//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  1.5.2020
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
/** Asynchronous update checking tool.
  * The object must live until a response is received, i.e. it can't be local in a function. */

class UpdateChecker : public QObject {

	Q_OBJECT

 public:

	UpdateChecker();
	virtual ~UpdateChecker();

	enum Result {
		CONNECTION_FAILED,
		INVALID_FORMAT,
		UPDATE_AVAILABLE,
		UPDATE_NOT_AVAILABLE
	};

	using ResultCallback = std::function< void ( Result result, const QString & detail ) >;

	void checkForUpdates( const ResultCallback & callback );

 private:

	void requestFinished( QNetworkReply * reply );

 private:

	QNetworkAccessManager manager;
	QHash< QNetworkReply *, ResultCallback > registeredCallbacks;

};


//----------------------------------------------------------------------------------------------------------------------

bool showUpdateNotification( QWidget * parent, const QString & newVersion, bool includeCheckbox );


#endif // UPDATE_CHECKER_INCLUDED
