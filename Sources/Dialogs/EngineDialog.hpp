//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of Engine Properties dialog that appears when you try to add of modify an engine
//======================================================================================================================

#ifndef ENGINE_DIALOG_INCLUDED
#define ENGINE_DIALOG_INCLUDED


#include "DialogCommon.hpp"

#include "UserData.hpp"  // EngineInfo
#include "Utils/FileSystemUtils.hpp"  // PathConvertor

#include <QDialog>

namespace Ui {
	class EngineDialog;
}


//======================================================================================================================

class EngineDialog : public QDialog, private DialogWithPaths {

	Q_OBJECT

	using thisClass = EngineDialog;
	using superClass = QDialog;

 public:

	explicit EngineDialog( QWidget * parent, const PathConvertor & pathConvertor, const EngineInfo & engine );
	virtual ~EngineDialog() override;

 private: // methods

	void adjustUi();
	void onWindowShown();

 private slots:

	void browseExecutable();
	void browseConfigDir();
	void browseDataDir();

	void onNameChanged( const QString & text );
	void onExecutableChanged( const QString & text );
	void onConfigDirChanged( const QString & text );
	void onDataDirChanged( const QString & text );
	void onFamilySelected( int familyIdx );

 public slots: // overridden methods

	virtual void showEvent( QShowEvent * event ) override;
	virtual void accept() override;

 private:

	Ui::EngineDialog * ui;

	QString suggestedName;
	QString suggestedConfigDir;
	QString suggestedDataDir;

 public: // return values from this dialog

	EngineInfo engine;

};


#endif // ENGINE_DIALOG_INCLUDED
