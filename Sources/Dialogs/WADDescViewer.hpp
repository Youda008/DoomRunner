//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: primitive viewer of WAD text description
//======================================================================================================================

#ifndef WAD_DESC_VIEWER_INCLUDED
#define WAD_DESC_VIEWER_INCLUDED


#include "DialogCommon.hpp"

#include <QDialog>
class QString;

namespace Ui
{
	class WADDescViewer;
}


//======================================================================================================================

class WADDescViewer : public QDialog, private DialogCommon {

	Q_OBJECT

	using ThisClass = WADDescViewer;

 public:

	explicit WADDescViewer( QWidget * parent, const QString & fileName, const QString & content );
	virtual ~WADDescViewer() override;

 private:

	void adjustUi( QWidget * parent );

 private:

	Ui::WADDescViewer * ui;

};


//----------------------------------------------------------------------------------------------------------------------

void showTxtDescriptionFor( QWidget * parent, const QString & filePath, const QString & contentType );


//======================================================================================================================


#endif // WAD_DESC_VIEWER_INCLUDED
