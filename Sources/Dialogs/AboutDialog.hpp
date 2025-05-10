//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of the About dialog that appears when you click Menu -> About
//======================================================================================================================

#ifndef ABOUT_DIALOG_INCLUDED
#define ABOUT_DIALOG_INCLUDED


#include "DialogCommon.hpp"

#include "UpdateChecker.hpp"

#include <QDialog>

namespace Ui
{
	class AboutDialog;
}


//======================================================================================================================

class AboutDialog : public QDialog, private DialogCommon {

	Q_OBJECT

	using ThisClass = AboutDialog;

 public:

	explicit AboutDialog( QWidget * parent, bool checkForUpdates );
	virtual ~AboutDialog() override;

 private slots:

	void onUpdateCheckingToggled( bool enabled );
	void checkForUpdate();

 private:

	Ui::AboutDialog * ui;

	UpdateChecker updateChecker;

 public: // return value from this dialog

	bool checkForUpdates;

};


//======================================================================================================================


#endif // ABOUT_DIALOG_INCLUDED
