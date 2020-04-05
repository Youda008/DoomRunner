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

#include <QDialog>

class PathHelper;

namespace Ui {
	class EngineDialog;
}


//======================================================================================================================

class EngineDialog : public QDialog {

	Q_OBJECT

	using thisClass = EngineDialog;

 public:

	explicit EngineDialog( QWidget * parent, const PathHelper & pathHelper, const Engine & engine );
	~EngineDialog();

 private slots:

	void browseEngine();

	void updateName( QString text );
	void updatePath( QString text );

 private:

	Ui::EngineDialog * ui;

	PathHelper pathHelper;

 public: // return values from this dialog

	Engine engine;

};


#endif // ENGINE_DIALOG_INCLUDED
