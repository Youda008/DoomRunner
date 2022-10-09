//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of Initial Setup dialog
//======================================================================================================================

#ifndef SETUP_DIALOG_INCLUDED
#define SETUP_DIALOG_INCLUDED


#include "Common.hpp"

#include "UserData.hpp"  // Engine, IWAD
#include "ListModel.hpp"
#include "EventFilters.hpp"  // ConfirmationFilter
#include "FileSystemUtils.hpp"  // PathContext

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

	explicit SetupDialog(
		QWidget * parent,
		const QDir & baseDir,
		const QList< Engine > & engineList,
		const QList< IWAD > & iwadList, const IwadSettings & iwadSettings,
		const MapSettings & mapSettings, const ModSettings & modSettings,
		const LauncherSettings & settings
	);
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
	void editCurrentEngine();

	void updateIWADsFromDir();

	void toggleAbsolutePaths( bool checked );

	void toggleCloseOnLaunch( bool checked );
	void toggleShowEngineOutput( bool checked );

 private: // methods

	void setupEngineList();
	void setupIWADList();

	void toggleAutoIWADUpdate( bool enabled );
	void browseDir( const QString & dirPurpose, QLineEdit * targetLine );

 private: // members

	Ui::SetupDialog * ui;

	uint tickCount;

	ConfirmationFilter engineConfirmationFilter;

 public: // return values from this dialog

	PathContext pathContext;

	EditableListModel< Engine > engineModel;

	EditableListModel< IWAD > iwadModel;
	IwadSettings iwadSettings;

	MapSettings mapSettings;

	ModSettings modSettings;

	LauncherSettings settings;

};


#endif // SETUP_DIALOG_INCLUDED
