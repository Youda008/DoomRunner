//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of the New Config dialog that appears when you click the Clone Config button
//======================================================================================================================

#ifndef CONFIG_DIALOG_INCLUDED
#define CONFIG_DIALOG_INCLUDED


#include "Common.hpp"

#include <QDialog>

namespace Ui {
	class ConfigDialog;
}


//======================================================================================================================

class NewConfigDialog : public QDialog {

	Q_OBJECT

	using thisClass = NewConfigDialog;

 public:

	explicit NewConfigDialog( QWidget * parent, const QString & currentConfigName );
	virtual ~NewConfigDialog() override;

	void confirmed();

 private:

	Ui::ConfigDialog * ui;

 public: // return values from this dialog

	QString newConfigName;

};


#endif // CONFIG_DIALOG_INCLUDED
