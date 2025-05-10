//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of Options Storage dialog
//======================================================================================================================

#ifndef OPTIONS_STORAGE_DIALOG_INCLUDED
#define OPTIONS_STORAGE_DIALOG_INCLUDED


#include "DialogCommon.hpp"

#include "UserData.hpp"  // StorageSettings

#include <QDialog>
class QRadioButton;

namespace Ui
{
	class OptionsStorageDialog;
}


//======================================================================================================================

class OptionsStorageDialog : public QDialog, private DialogCommon {

	Q_OBJECT

	using ThisClass = OptionsStorageDialog;

 public:

	explicit OptionsStorageDialog( QWidget * parent, const StorageSettings & settings );
	virtual ~OptionsStorageDialog() override;

 private: // methods

	void restoreStorage( OptionsStorage storage, QRadioButton * noneBtn, QRadioButton * globalBtn, QRadioButton * presetBtn );

 private slots:

	void onLaunchStorageChosen_none();
	void onLaunchStorageChosen_global();
	void onLaunchStorageChosen_preset();

	void onGameplayStorageChosen_none();
	void onGameplayStorageChosen_global();
	void onGameplayStorageChosen_preset();

	void onCompatStorageChosen_none();
	void onCompatStorageChosen_global();
	void onCompatStorageChosen_preset();

	void onVideoStorageChosen_none();
	void onVideoStorageChosen_global();
	void onVideoStorageChosen_preset();

	void onAudioStorageChosen_none();
	void onAudioStorageChosen_global();
	void onAudioStorageChosen_preset();

 private: // members

	Ui::OptionsStorageDialog * ui;

 public: // return values from this dialog

	StorageSettings storageSettings;

};


//======================================================================================================================


#endif // OPTIONS_STORAGE_DIALOG_INCLUDED
