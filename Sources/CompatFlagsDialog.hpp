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
	class CompatFlagsDialog;
}


//======================================================================================================================


class CompatFlagsDialog : public QDialog {

    Q_OBJECT

 public:

    explicit CompatFlagsDialog( QWidget * parent, uint32_t & compatflags1, uint32_t & compatflags2 );
    ~CompatFlagsDialog();

 protected:

    //virtual void closeEvent( QCloseEvent * event );

 private:

    Ui::CompatFlagsDialog * ui;

    uint32_t & flags1;
    uint32_t & flags2;

};


#endif // COMPAT_FLAGS_DIALOG_INCLUDED
