//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  25.1.2021
// Description:
//======================================================================================================================

#ifndef CONFIG_DIALOG_INCLUDED
#define CONFIG_DIALOG_INCLUDED


#include "Common.hpp"

#include <QDialog>

namespace Ui {
	class ConfigDialog;
}


//======================================================================================================================

class ConfigDialog : public QDialog {

	Q_OBJECT

	using thisClass = ConfigDialog;

 public:

	explicit ConfigDialog( QWidget * parent, const QString & currentConfigName );
	virtual ~ConfigDialog() override;

	void confirmed();

 private:

	Ui::ConfigDialog * ui;

 public: // return values from this dialog

	QString newConfigName;

};


#endif // CONFIG_DIALOG_INCLUDED
