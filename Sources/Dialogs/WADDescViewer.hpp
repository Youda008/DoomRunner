//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: primitive viewer of WAD text description
//======================================================================================================================

#ifndef WAD_DESC_VIEWER_INCLUDED
#define WAD_DESC_VIEWER_INCLUDED

class QWidget;
class QString;

void showTxtDescriptionFor( QWidget * parent, const QString & filePath, const QString & contentType );

#endif // WAD_DESC_VIEWER_INCLUDED
