//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of Engine Properties dialog that appears when you try to add of modify an engine
//======================================================================================================================

#ifndef ENGINE_DIALOG_INCLUDED
#define ENGINE_DIALOG_INCLUDED


#include "Common.hpp"

#include "UserData.hpp"  // Engine
#include "FileSystemUtils.hpp"  // PathContext

#include <QDialog>

namespace Ui {
	class EngineDialog;
}


//======================================================================================================================

class EngineDialog : public QDialog {

	Q_OBJECT

	using thisClass = EngineDialog;

 public:

	explicit EngineDialog( QWidget * parent, const PathContext & pathContext, const Engine & engine );
	virtual ~EngineDialog() override;

 private: // methods

	void onWindowShown();

 private slots:

	void browseEngine();
	void browseConfigDir();

	void updateName( const QString & text );
	void updatePath( const QString & text );
	void updateConfigDir( const QString & text );
	void selectFamily( int familyIdx );

 private:

	Ui::EngineDialog * ui;

	PathContext pathContext;

 public: // return values from this dialog

	Engine engine;

};


#endif // ENGINE_DIALOG_INCLUDED
