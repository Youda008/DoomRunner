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
class PathConvertor;

#include <QDialog>

namespace Ui
{
	class EngineDialog;
}


//======================================================================================================================

class EngineDialog : public QDialog, public DialogWithPaths {

	Q_OBJECT

	using ThisClass = EngineDialog;
	using SuperClass = QDialog;

 public:

	explicit EngineDialog( QWidget * parent, const PathConvertor & pathConvertor, const EngineInfo & engine, QString lastUsedDir );
	virtual ~EngineDialog() override;

	/// Attempts to auto-detect the engine properties based on its currently set executablePath.
	static void autofillEngineInfo( EngineInfo & engine, const QString & executablePath );

 private: // overridden methods

	virtual void showEvent( QShowEvent * event ) override;

 private: // own methods

	void adjustUi();
	void onWindowShown();

	void autofillEngineFields();

 private slots:

	void selectExecutable();
	void selectConfigDir();
	void selectDataDir();

	void onNameChanged( const QString & text );
	void onExecutableChanged( const QString & text );
	void onConfigDirChanged( const QString & text );
	void onDataDirChanged( const QString & text );
	void onFamilySelected( int familyIdx );

	void onAutoDetectBtnClicked();

	void onAcceptBtnClicked();

 private: // internal members

	Ui::EngineDialog * ui;

	bool windowAlreadyShown = false;  ///< whether the main window already appeared at least once

	QString suggestedConfigDir;
	QString suggestedDataDir;

 public: // return values from this dialog

	EngineInfo engine;

};


#endif // ENGINE_DIALOG_INCLUDED
