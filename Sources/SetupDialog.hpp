//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  13.5.2019
// Description:
//======================================================================================================================

#ifndef SETUP_DIALOG_INCLUDED
#define SETUP_DIALOG_INCLUDED


#include "Common.hpp"

#include "SharedData.hpp"  // Engine, IWAD
#include "ListModel.hpp"
#include "FileSystemUtils.hpp"  // PathHelper

#include <QDialog>

class QDir;
class QLineEdit;
class PathHelper;

namespace Ui {
	class SetupDialog;
}


//======================================================================================================================

class SetupDialog : public QDialog {

	Q_OBJECT

	using thisClass = SetupDialog;

 public:

	explicit SetupDialog( QWidget * parent, bool useAbsolutePaths, const QDir & baseDir, const QList< Engine > & engines,
	                      const QList< IWAD > & iwads, bool iwadListFromDir, const QString & iwadDir, bool iwadSubdirs,
	                      const QString & mapDir, const QString & modDir );
	virtual ~SetupDialog() override;

 private:

	virtual void timerEvent( QTimerEvent * event ) override;

 private slots:

	void manageIWADsManually();
	void manageIWADsAutomatically();
	void toggleIWADSubdirs( bool checked );

	void browseIWADDir();
	void browseMapDir();
	void browseModDir();

	void changeIWADDir( const QString & text );
	void changeMapDir( const QString & text );
	void changeModDir( const QString & text );

	void iwadAdd();
	void iwadDelete();
	void iwadMoveUp();
	void iwadMoveDown();

	void engineAdd();
	void engineDelete();
	void engineMoveUp();
	void engineMoveDown();

	void editEngine( const QModelIndex & index );

	void updateIWADsFromDir();

	void toggleAbsolutePaths( bool checked );

	void closeDialog();

 private: // methods

	void toggleAutoIWADUpdate( bool enabled );
	void browseDir( const QString & dirPurpose, QLineEdit * targetLine );

 private: // members

	Ui::SetupDialog * ui;

	uint tickCount;

 public: // return values from this dialog

	PathHelper pathHelper;

	// engine info
	EditableListModel< Engine > engineModel;

	// IWAD info
	EditableListModel< IWAD > iwadModel;
	bool iwadListFromDir;
	QString iwadDir;
	bool iwadSubdirs;

	// additional paths
	QString mapDir;
	QString modDir;

};


#endif // SETUP_DIALOG_INCLUDED
