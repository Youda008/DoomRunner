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
	connect( searchLine, &QLineEdit::textChanged, this, &ThisClass::onSearchPhraseChanged );
	connect( caseChkBox, &QCheckBox::toggled, this, &ThisClass::onCaseSensitiveToggled );
	connect( regexChkBox, &QCheckBox::toggled, this, &ThisClass::onUseRegexToggled );
}

bool SearchPanel::isExpanded() const
{
	return searchLine->isVisible();
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

	emit expandedToggled( expanded );
}

void SearchPanel::expand()
{
	setExpanded( true );
}

void SearchPanel::collapse()
{
	setExpanded( false );
}

void SearchPanel::toggleExpanded()
{
	setExpanded( !isExpanded() );
}

void SearchPanel::openSearch()
{
	setExpanded( true );
	searchLine->setFocus();
}

QString SearchPanel::searchPhrase() const
{
	return searchLine->text();
}

void SearchPanel::setSearchPhrase( const QString & phrase )
{
	searchLine->setText( phrase );
}

bool SearchPanel::caseSensitive() const
{
	return caseChkBox->isChecked();
}

void SearchPanel::toggleCaseSensitive( bool enable )
{
	caseChkBox->setChecked( enable );
}

bool SearchPanel::useRegex() const
{
	return regexChkBox->isChecked();
}

void SearchPanel::toggleUseRegex( bool enable )
{
	regexChkBox->setChecked( enable );
}

void SearchPanel::onSearchPhraseChanged( const QString & phrase )
{
	emit searchParamsChanged( phrase, caseSensitive(), useRegex() );
}

void SearchPanel::onCaseSensitiveToggled( bool enabled )
{
	emit searchParamsChanged( searchPhrase(), enabled, useRegex() );
}

void SearchPanel::onUseRegexToggled( bool enabled )
{
	emit searchParamsChanged( searchPhrase(), caseSensitive(), enabled );
}
