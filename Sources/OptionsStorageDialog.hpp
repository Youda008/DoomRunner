//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of Options Storage dialog
//======================================================================================================================

#ifndef OPTIONS_STORAGE_DIALOG_INCLUDED
#define OPTIONS_STORAGE_DIALOG_INCLUDED


#include "Common.hpp"

#include "UserData.hpp"  // OptionsStorage

#include <QDialog>

class QRadioButton;

namespace Ui {
	class OptionsStorageDialog;
}


//======================================================================================================================

class OptionsStorageDialog : public QDialog {

	Q_OBJECT

	using thisClass = OptionsStorageDialog;

 public:

	explicit OptionsStorageDialog( QWidget * parent, const StorageSettings & settings );
	virtual ~OptionsStorageDialog() override;

 private: // methods

	void restoreStorage( OptionsStorage storage, QRadioButton * noneBtn, QRadioButton * globalBtn, QRadioButton * presetBtn );

 private slots:

	void launchStorage_none();
	void launchStorage_global();
	void launchStorage_preset();

	void gameplayStorage_none();
	void gameplayStorage_global();
	void gameplayStorage_preset();

	void compatStorage_none();
	void compatStorage_global();
	void compatStorage_preset();

 private: // members

	Ui::OptionsStorageDialog * ui;

 public: // return values from this dialog

	StorageSettings storageSettings;

};


#endif // OPTIONS_STORAGE_DIALOG_INCLUDED
