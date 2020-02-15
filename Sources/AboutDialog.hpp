//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  15.2.2020
// Description:
//======================================================================================================================

#ifndef ABOUT_DIALOG_INCLUDED
#define ABOUT_DIALOG_INCLUDED

#include <QDialog>

namespace Ui {
	class AboutDialog;
}


//======================================================================================================================

class AboutDialog : public QDialog {

	Q_OBJECT

 public:

	explicit AboutDialog( QWidget * parent = nullptr );
	~AboutDialog();

 private:

	Ui::AboutDialog * ui;

};


#endif // ABOUT_DIALOG_INCLUDED
