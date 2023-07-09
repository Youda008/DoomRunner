//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of Engine Properties dialog that appears when you try to add of modify an engine
//======================================================================================================================

#ifndef ENGINE_DIALOG_INCLUDED
#define ENGINE_DIALOG_INCLUDED


#include "DialogCommon.hpp"

#include "UserData.hpp"  // Engine
#include "Utils/FileSystemUtils.hpp"  // PathContext

#include <QDialog>

namespace Ui {
	class EngineDialog;
}


//======================================================================================================================

class EngineDialog : public QDialog, private DialogCommon {

	Q_OBJECT

	using thisClass = EngineDialog;
	using superClass = QDialog;

 public:

	explicit EngineDialog( QWidget * parent, const PathContext & pathContext, const Engine & engine );
	virtual ~EngineDialog() override;

 private: // methods

	void adjustUi();
	void onWindowShown();

 private slots:

	void browseEngine();
	void browseConfigDir();

	void onNameChanged( const QString & text );
	void onPathChanged( const QString & text );
	void onConfigDirChanged( const QString & text );
	void onFamilySelected( int familyIdx );

 public slots: // overridden methods

	virtual void showEvent( QShowEvent * event ) override;
	virtual void accept() override;

 private:

	Ui::EngineDialog * ui;

	PathContext pathContext;

 public: // return values from this dialog

	Engine engine;

};


#endif // ENGINE_DIALOG_INCLUDED
