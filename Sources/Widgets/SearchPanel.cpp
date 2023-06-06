#include "SearchPanel.hpp"

#include <QString>
#include <QToolButton>
#include <QLineEdit>
#include <QCheckBox>


SearchPanel::SearchPanel(
	QToolButton * showBtn, QLineEdit * searchLine, QCheckBox * caseChkBox, QCheckBox * regexChkBox
) :
	showBtn( showBtn ), searchLine( searchLine ), caseChkBox( caseChkBox ), regexChkBox( regexChkBox )
{
	connect( showBtn, &QToolButton::clicked, this, &thisClass::toggleExpanded );
	connect( searchLine, &QLineEdit::textChanged, this, &thisClass::changeSearchPhrase );
	connect( caseChkBox, &QCheckBox::toggled, this, &thisClass::toggleCaseSensitive );
	connect( regexChkBox, &QCheckBox::toggled, this, &thisClass::toggleUseRegex );
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
