//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  13.5.2019
// Description:
//======================================================================================================================

#ifndef COMPAT_FLAGS_DIALOG_INCLUDED
#define COMPAT_FLAGS_DIALOG_INCLUDED


#include "Common.hpp"

#include <QDialog>


namespace Ui {
	class CompatOptsDialog;
}


//======================================================================================================================


class CompatOptsDialog : public QDialog {

    Q_OBJECT

 public:

    explicit CompatOptsDialog( QWidget * parent, uint32_t & compatflags1, uint32_t & compatflags2 );
    ~CompatOptsDialog();

 protected:

    //virtual void closeEvent( QCloseEvent * event );

 private:

    Ui::CompatOptsDialog * ui;

    uint32_t & flags1;
    uint32_t & flags2;

};


#endif // COMPAT_FLAGS_DIALOG_INCLUDED
