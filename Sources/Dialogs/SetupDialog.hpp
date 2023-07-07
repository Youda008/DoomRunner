//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of Initial Setup dialog
//======================================================================================================================

#ifndef SETUP_DIALOG_INCLUDED
#define SETUP_DIALOG_INCLUDED


#include "DialogCommon.hpp"

#include "UserData.hpp"  // Engine, IWAD
#include "Widgets/ListModel.hpp"
#include "Utils/EventFilters.hpp"  // ConfirmationFilter

#include <QDialog>

class QDir;
class QLineEdit;
class QAction;
class QItemSelection;

namespace Ui {
	class SetupDialog;
}


//======================================================================================================================

class SetupDialog : public QDialog, private DialogWithPaths {

	Q_OBJECT

	using thisClass = SetupDialog;

 public:

	explicit SetupDialog(
		QWidget * parent,
		const QDir & baseDir,
		const EngineSettings & engineSettings, const QList< Engine > & engineList,
		const IwadSettings & iwadSettings, const QList< IWAD > & iwadList,
		const MapSettings & mapSettings, const ModSettings & modSettings,
		const LauncherSettings & settings
	);
	virtual ~SetupDialog() override;

 private: // overridden methods

	virtual void timerEvent( QTimerEvent * event ) override;

 private slots:

	// engines

	void engineAdd();
	void engineDelete();
	void engineMoveUp();
	void engineMoveDown();

	void engineSelectionChanged( const QItemSelection & selected, const QItemSelection & deselected );
	void setEngineAsDefault();

	void editEngine( const QModelIndex & index );
	void editSelectedEngine();

	// IWADs

	void iwadAdd();
	void iwadDelete();
	void iwadMoveUp();
	void iwadMoveDown();

	void iwadSelectionChanged( const QItemSelection & selected, const QItemSelection & deselected );
	void setIWADAsDefault();

	void manageIWADsManually();
	void manageIWADsAutomatically();
	void toggleIWADSubdirs( bool checked );

	// game file directories

	void browseIWADDir();
	void browseMapDir();
	void browseModDir();

	void changeIWADDir( const QString & dir );
	void changeMapDir( const QString & dir );
	void changeModDir( const QString & dir );

	void updateIWADsFromDir();

	// theme options

	void selectAppStyle( int index );
	void setDefaultScheme();
	void setDarkScheme();
	void setLightScheme();

	// other

	void toggleAbsolutePaths( bool checked );

	void toggleShowEngineOutput( bool checked );
	void toggleCloseOnLaunch( bool checked );

 private: // methods

	void setupEngineList();
	void setupIWADList();

	void toggleAutoIWADUpdate( bool enabled );

 private: // members

	Ui::SetupDialog * ui;

	QAction * setDefaultEngineAction;
	QAction * setDefaultIWADAction;

	uint tickCount;

	ConfirmationFilter engineConfirmationFilter;

 public: // return values from this dialog

	EngineSettings engineSettings;
	EditableDirectListModel< Engine > engineModel;

	IwadSettings iwadSettings;
	EditableDirectListModel< IWAD > iwadModel;

	MapSettings mapSettings;

	ModSettings modSettings;

	LauncherSettings settings;

};


#endif // SETUP_DIALOG_INCLUDED
