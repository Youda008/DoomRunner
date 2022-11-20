//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: Extended QLabel adapting its hyperlink color based on selected theme
//======================================================================================================================

#include "LabelWithHyperlink.hpp"

#include "Themes.hpp"


//======================================================================================================================

LabelWithHyperlink::LabelWithHyperlink( QWidget * parent ) : QLabel( parent ) {}

LabelWithHyperlink::~LabelWithHyperlink() {}

void LabelWithHyperlink::setText( const QString & text )
{
	superClass::setText( themes::updateHyperlinkColor( text ) );
}
