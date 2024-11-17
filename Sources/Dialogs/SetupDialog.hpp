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
#include "Widgets/EditableListView.hpp"  // DnDType
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

class SetupDialog : public QDialog, public DialogWithPaths {

	Q_OBJECT

	using thisClass = SetupDialog;

 public:

	explicit SetupDialog(
		QWidget * parent,
		const PathConvertor & pathConvertor,
		const EngineSettings & engineSettings, const QList< EngineInfo > & engineList,
		const IwadSettings & iwadSettings, const QList< IWAD > & iwadList,
		const MapSettings & mapSettings, const ModSettings & modSettings,
		const LauncherSettings & settings,
		const AppearanceSettings & appearance
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
	void onEnginesDropped( int row, int count, DnDType type );

	void onEngineSelectionChanged( const QItemSelection & selected, const QItemSelection & deselected );
	void setEngineAsDefault();

	void editEngine( const QModelIndex & index );
	void editSelectedEngine();

	// IWADs

	void iwadAdd();
	void iwadDelete();
	void iwadMoveUp();
	void iwadMoveDown();

	void onIWADSelectionChanged( const QItemSelection & selected, const QItemSelection & deselected );
	void setIWADAsDefault();

	void manageIWADsManually();
	void manageIWADsAutomatically();
	void onIWADSubdirsToggled( bool checked );

	// game file directories

	void browseIWADDir();
	void browseMapDir();
	void browseModDir();

	void onIWADDirChanged( const QString & dir );
	void onMapDirChanged( const QString & dir );
	void onModDirChanged( const QString & dir );

	void updateIWADsFromDir();

	// theme options

	void onAppStyleSelected( int index );
	void onDefaultSchemeChosen();
	void onDarkSchemeChosen();
	void onLightSchemeChosen();

	// other

	void onAbsolutePathsToggled( bool checked );

	void onShowEngineOutputToggled( bool checked );
	void onCloseOnLaunchToggled( bool checked );

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
	EditableDirectListModel< EngineInfo > engineModel;

	IwadSettings iwadSettings;
	EditableDirectListModel< IWAD > iwadModel;

	MapSettings mapSettings;

	ModSettings modSettings;

	LauncherSettings settings;

	AppearanceSettings appearance;

};


#endif // SETUP_DIALOG_INCLUDED
