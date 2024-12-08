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

#include <QDialog>

class PathConvertor;

namespace Ui {
	class EngineDialog;
}


//======================================================================================================================

class EngineDialog : public QDialog, public DialogWithPaths {

	Q_OBJECT

	using thisClass = EngineDialog;
	using superClass = QDialog;

 public:

	explicit EngineDialog( QWidget * parent, const PathConvertor & pathConvertor, const EngineInfo & engine, QString lastUsedDir );
	virtual ~EngineDialog() override;

	/// Attempts to auto-detect the engine properties based on its currently set executablePath.
	static void autofillEngineInfo( EngineInfo & engine, const QString & executablePath );

 private: // methods

	void adjustUi();
	void onWindowShown();

 private slots:

	void browseExecutable();
	void browseConfigDir();
	void browseDataDir();

	void autofillEngineFields();

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

	QString suggestedConfigDir;
	QString suggestedDataDir;

 public: // return values from this dialog

	EngineInfo engine;

};


#endif // ENGINE_DIALOG_INCLUDED
