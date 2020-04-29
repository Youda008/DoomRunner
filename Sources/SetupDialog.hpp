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

#include "UserData.hpp"  // Engine, IWAD
#include "ListModel.hpp"
#include "FileSystemUtils.hpp"  // PathHelper

#include <QDialog>

class QDir;
class QLineEdit;

namespace Ui {
	class SetupDialog;
}


//======================================================================================================================

class SetupDialog : public QDialog {

	Q_OBJECT

	using thisClass = SetupDialog;

 public:

	explicit SetupDialog( QWidget * parent, bool useAbsolutePaths, const QDir & baseDir,
	                      const QList< Engine > & engineList,
	                      const QList< IWAD > & iwadList, const IwadSettings & iwadSettings,
	                      const MapSettings & mapSettings, const ModSettings & modSettings );
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

	EditableListModel< Engine > engineModel;

	EditableListModel< IWAD > iwadModel;
	IwadSettings iwadSettings;

	MapSettings mapSettings;

	ModSettings modSettings;

};


#endif // SETUP_DIALOG_INCLUDED
