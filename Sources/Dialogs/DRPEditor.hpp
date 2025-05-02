//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: internal editor of DoomRunner Packs
//======================================================================================================================

#ifndef DRP_EDITOR_INCLUDED
#define DRP_EDITOR_INCLUDED


#include "DialogCommon.hpp"

#include "UserData.hpp"  // Mod
#include "DataModels/GenericListModel.hpp"  // for modModel
#include "Widgets/ExtendedListView.hpp"  // DnDSources

#include <QDialog>

class PathConvertor;

namespace Ui
{
	class DRPEditor;
}


//======================================================================================================================

class DRPEditor : public QDialog, public DialogWithPaths {

	Q_OBJECT

	using ThisClass = DRPEditor;
	using SuperClass = QDialog;

 public:

	explicit DRPEditor(
		QWidget * parentWidget, const PathConvertor & pathConvertor, QString lastUsedDir, bool showIcons, QString filePath
	);
	virtual ~DRPEditor() override;

	bool areIconsEnabled() const;

 private slots:

	void selectDestinationFile();

	void modAdd();
	void modAddDir();
	void modAddExistingDRP();
	void modCreateNewDRP();
	void modDelete();
	void modMoveUp();
	void modMoveDown();
	void modMoveToTop();
	void modMoveToBottom();
	void onModDoubleClicked( const QModelIndex & index );

 private slots: // overridden slots

	virtual void accept() override;

 private: // internal methods

	void setupModList( bool showIcons );
	void loadModsFromDRP( const QString & filePath );
	bool saveModsToDRP( const QString & filePath );

 private: // internal members

	Ui::DRPEditor * ui;
	bool windowAlreadyShown = false;  ///< whether the main window already appeared at least once

	QString origFilePath;
	EditableDirectListModel< Mod > modModel;

	QAction * addExistingDRP = nullptr;
	QAction * createNewDRP = nullptr;

 public: // return values from this dialog

	QString savedFilePath;

};


#endif // DRP_EDITOR_INCLUDED
