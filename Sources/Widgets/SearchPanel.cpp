//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: Panel allowing the user to enter search criteria
//======================================================================================================================

#include "SearchPanel.hpp"

#include <QString>
#include <QToolButton>
#include <QLineEdit>
#include <QCheckBox>


//======================================================================================================================

SearchPanel::SearchPanel(
	QToolButton * showBtn, QLineEdit * searchLine, QCheckBox * caseChkBox, QCheckBox * regexChkBox
) :
	showBtn( showBtn ), searchLine( searchLine ), caseChkBox( caseChkBox ), regexChkBox( regexChkBox )
{
	connect( showBtn, &QToolButton::clicked, this, &ThisClass::toggleExpanded );
	connect( searchLine, &QLineEdit::textChanged, this, &ThisClass::changeSearchPhrase );
	connect( caseChkBox, &QCheckBox::toggled, this, &ThisClass::toggleCaseSensitive );
	connect( regexChkBox, &QCheckBox::toggled, this, &ThisClass::toggleUseRegex );
}

void SearchPanel::setExpanded( bool expanded )
{
	if (!expanded)
	{
		searchLine->clear();
	}

	searchLine->setVisible( expanded );
	caseChkBox->setVisible( expanded );
	regexChkBox->setVisible( expanded );

	showBtn->setArrowType( expanded ? Qt::ArrowType::DownArrow : Qt::ArrowType::UpArrow );
}

void SearchPanel::expand()
{
	setExpanded( true );
	searchLine->setFocus();
}

void SearchPanel::collapse()
{
	setExpanded( false );
}

void SearchPanel::toggleExpanded()
{
	setExpanded( !searchLine->isVisible() );
}

void SearchPanel::changeSearchPhrase( const QString & phrase )
{
	emit searchParamsChanged( phrase, caseChkBox->isChecked(), regexChkBox->isChecked() );
}

void SearchPanel::toggleCaseSensitive( bool enable )
{
	emit searchParamsChanged( searchLine->text(), enable, regexChkBox->isChecked() );
}

void SearchPanel::toggleUseRegex( bool enable )
{
	emit searchParamsChanged( searchLine->text(), caseChkBox->isChecked(), enable );
}
