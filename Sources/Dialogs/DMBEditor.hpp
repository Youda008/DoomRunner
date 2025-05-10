//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: internal editor of Doom Mod Bundles
//======================================================================================================================

#ifndef DMB_EDITOR_INCLUDED
#define DMB_EDITOR_INCLUDED


#include "DialogCommon.hpp"

#include "UserData.hpp"  // Mod
#include "DataModels/GenericListModel.hpp"  // for modModel
class PathConvertor;

#include <QDialog>

namespace Ui
{
	class DMBEditor;
}


//======================================================================================================================

class DMBEditor : public QDialog, public DialogWithPaths {

	Q_OBJECT

	using ThisClass = DMBEditor;
	using SuperClass = QDialog;

 public:

	explicit DMBEditor(
		QWidget * parentWidget, const PathConvertor & pathConvertor, QString lastUsedDir, bool showIcons, QString filePath
	);
	virtual ~DMBEditor() override;

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
	void modCreateNewDMB();
	void modAddExistingDMB();
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

	void loadModsFromDMB( const QString & filePath );
	bool saveModsToDMB( const QString & filePath );

	QString createNewDMB();
	QStringList addExistingDMB();
	Result editDMB( const QString & filePath );

 private: // internal members

	Ui::DMBEditor * ui;
	bool windowAlreadyShown = false;  ///< whether the main window already appeared at least once

	EditableDirectListModel< Mod > modModel;

	QAction * createNewDMBAction = nullptr;
	QAction * addExistingDMBAction = nullptr;

 public: // return values from this dialog

	QString origFilePath;
	QString savedFilePath;

	Outcome outcome;

};


//======================================================================================================================


#endif // DMB_EDITOR_INCLUDED
