//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: custom QFileDialog wrapper, workaround for some issues with less common Linux graphical environments
//======================================================================================================================

#ifndef OWN_FILE_DIALOG
#define OWN_FILE_DIALOG


#include "Common.hpp"

#include <QFileDialog>


//======================================================================================================================

/// custom QFileDialog wrapper, workaround for some issues with less common Linux graphical environments
class OwnFileDialog : public QFileDialog {

 public:

	static QString getOpenFileName(
		QWidget * parent, const QString & caption, const QString & dir,
		const QString & filter = QString(), QString * selectedFilter = nullptr, Options options = Options()
	);

	static QStringList getOpenFileNames(
		QWidget * parent, const QString & caption, const QString & dir,
		const QString & filter = QString(), QString * selectedFilter = nullptr, Options options = Options()
	);

	static QString getSaveFileName(
		QWidget * parent, const QString & caption, const QString & dir,
		const QString & filter = QString(), QString * selectedFilter = nullptr, Options options = Options()
	);

	static QString getExistingDirectory(
		QWidget * parent, const QString & caption, const QString & dir, Options options = ShowDirsOnly
	);

};


#endif // OWN_FILE_DIALOG
