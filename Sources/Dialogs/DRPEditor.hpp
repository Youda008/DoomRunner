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

	enum class Outcome
	{
		Cancelled,        ///< user cancelled this dialog, no change has been made
		Failed,           ///< saving or deleting the file failed, no change has been made
		SavedToExisting,  ///< the content has been successfully saved to the original file
		SavedAsNew,       ///< the content has been successfully saved to a new file
		Deleted,          ///< the existing file has been successfully deleted
	};

	struct Result
	{
		Outcome outcome;
		QString savedFilePath;
	};

 private slots:

	void modAdd();
	void modAddDir();
	void modCreateNewDRP();
	void modAddExistingDRP();
	void modDelete();
	void modMoveUp();
	void modMoveDown();
	void modMoveToTop();
	void modMoveToBottom();
	void onModDoubleClicked( const QModelIndex & index );

	void onSaveBtnClicked();
	void onSaveAsBtnClicked();
	void onDeleteBtnClicked();

 private: // internal methods

	void setupModList( bool showIcons );

	void loadModsFromDRP( const QString & filePath );
	bool saveModsToDRP( const QString & filePath );

	QString createNewDRP();
	QStringList addExistingDRP();
	Result editDRP( const QString & filePath );

 private: // internal members

	Ui::DRPEditor * ui;
	bool windowAlreadyShown = false;  ///< whether the main window already appeared at least once

	EditableDirectListModel< Mod > modModel;

	QAction * createNewDRPAction = nullptr;
	QAction * addExistingDRPAction = nullptr;

 public: // return values from this dialog

	QString origFilePath;
	QString savedFilePath;

	Outcome outcome;

};


#endif // DRP_EDITOR_INCLUDED
