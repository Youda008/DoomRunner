//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  13.5.2019
// Description:
//======================================================================================================================

#ifndef ENGINE_DIALOG_INCLUDED
#define ENGINE_DIALOG_INCLUDED


#include "Common.hpp"

#include "SharedData.hpp"  // Engine
#include "FileSystemUtils.hpp"  // PathHelper

#include <QDialog>

namespace Ui {
	class EngineDialog;
}


//======================================================================================================================

class EngineDialog : public QDialog {

	Q_OBJECT

	using thisClass = EngineDialog;

 public:

	explicit EngineDialog( QWidget * parent, const PathHelper & pathHelper, const Engine & engine );
	virtual ~EngineDialog() override;

 private: //methods

	virtual void showEvent( QShowEvent * event ) override;

 private slots:

	void browseEngine();
	void browseConfigDir();

	void updateName( const QString & text );
	void updatePath( const QString & text );
	void updateConfigDir( const QString & text );

 private:

	Ui::EngineDialog * ui;

	PathHelper pathHelper;

 public: // return values from this dialog

	Engine engine;

};


#endif // ENGINE_DIALOG_INCLUDED
