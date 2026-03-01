//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: Panel allowing the user to enter search criteria
//======================================================================================================================

#ifndef SEARCH_PANEL_INCLUDED
#define SEARCH_PANEL_INCLUDED


#include "Essential.hpp"

#include <QObject>
class QString;
class QToolButton;
class QLineEdit;
class QCheckBox;


//======================================================================================================================
/// Panel allowing the user to enter search criteria

class SearchPanel : public QObject {

	Q_OBJECT

	using ThisClass = SearchPanel;

 public:

	SearchPanel( QToolButton * showBtn, QLineEdit * searchPhraseLine, QCheckBox * caseChkBox, QCheckBox * regexChkBox );
	~SearchPanel() override = default;

	bool isExpanded() const;

	QString searchPhrase() const;
	void setSearchPhrase( const QString & phrase );

	bool caseSensitive() const;
	void toggleCaseSensitive( bool enable );
	bool useRegex() const;
	void toggleUseRegex( bool enable );

 public slots:

	void expand();
	void collapse();
	void toggleExpanded();
	void setExpanded( bool expanded );

	void openSearch();

 private slots:

	void onSearchPhraseChanged( const QString & phrase );
	void onCaseSensitiveToggled( bool enabled );
	void onUseRegexToggled( bool enabled );

 signals:

	void expandedToggled( bool expanded );
	void searchParamsChanged( const QString & phrase, bool caseSensitive, bool useRegex );

 public:

	QToolButton * showBtn;
	QLineEdit * searchLine;
	QCheckBox * caseChkBox;
	QCheckBox * regexChkBox;

};


//======================================================================================================================


#endif // SEARCH_PANEL_INCLUDED
