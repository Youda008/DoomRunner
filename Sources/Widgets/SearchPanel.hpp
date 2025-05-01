#include <QObject>

class QString;
class QToolButton;
class QLineEdit;
class QCheckBox;

class SearchPanel : public QObject {

	Q_OBJECT

	using ThisClass = SearchPanel;

 public:

	SearchPanel( QToolButton * showBtn, QLineEdit * searchPhraseLine, QCheckBox * caseChkBox, QCheckBox * regexChkBox );
	~SearchPanel() override {}

 public slots:

	void expand();
	void collapse();
	void toggleExpanded();
	void setExpanded( bool expanded );

	void changeSearchPhrase( const QString & phrase );
	void toggleCaseSensitive( bool enable );
	void toggleUseRegex( bool enable );

 signals:

	void searchParamsChanged( const QString & phrase, bool caseSensitive, bool useRegex );

 public:

	QToolButton * showBtn;
	QLineEdit * searchLine;
	QCheckBox * caseChkBox;
	QCheckBox * regexChkBox;

};
