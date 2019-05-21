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

#include "SharedData.hpp"

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

	explicit EngineDialog( QWidget * parent, PathHelper & pathHelper, QString & name, QString & path );
	~EngineDialog();

 private slots:

	void browseEngine();

	void confirm();
	void cancel();

 private:

	Ui::EngineDialog * ui;

	PathHelper & pathHelper;

	// dialog will return the user's input via these references
	QString & name;
	QString & path;

};


#endif // ENGINE_DIALOG_INCLUDED
