//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: Extended QLabel adapting its hyperlink color based on selected theme
//======================================================================================================================

#ifndef LABEL_WITH_HYPERLINK_INCLUDED
#define LABEL_WITH_HYPERLINK_INCLUDED


#include "Common.hpp"

#include <QLabel>


//======================================================================================================================
/// Extended QLabel adapting its hyperlink color based on selected theme

class LabelWithHyperlink : public QLabel {

	Q_OBJECT

	using thisClass = LabelWithHyperlink;
	using superClass = QLabel;

 public:

	LabelWithHyperlink( QWidget * parent );
	virtual ~LabelWithHyperlink() override;

 public slots:

    void setText( const QString & );

};


#endif // LABEL_WITH_HYPERLINK_INCLUDED
