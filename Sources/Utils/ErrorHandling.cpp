//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: common error-handling routines
//======================================================================================================================

#include "ErrorHandling.hpp"

#include "Utils/WidgetUtils.hpp"  // HYPERLINK

#include <QMessageBox>
#include <QStringBuilder>


//----------------------------------------------------------------------------------------------------------------------

static const QString issuePageUrl = "https://github.com/Youda008/DoomRunner/issues";

void reportBugToUser( QWidget * parent, QString title, QString message )
{
	QMessageBox::critical( parent, title,
		"<html><head/><body>"
		"<p>"
			%message%" This is a bug, please create an issue at " HYPERLINK( issuePageUrl, issuePageUrl )
		"</p>"
		"</body></html>"
	);
}
