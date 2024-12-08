//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of the Compatibility Options dialog
//======================================================================================================================

#ifndef COMPAT_OPTS_DIALOG_INCLUDED
#define COMPAT_OPTS_DIALOG_INCLUDED


#include "DialogCommon.hpp"

#include "CommonTypes.hpp"
#include "UserData.hpp"  // CompatibilityOptions

#include <QDialog>


namespace Ui {
	class CompatOptsDialog;
}

struct CompatFlag;


//======================================================================================================================

class CompatOptsDialog : public QDialog, private DialogCommon {

	Q_OBJECT

	using thisClass = CompatOptsDialog;

 public:

	explicit CompatOptsDialog( QWidget * parent, const CompatibilityDetails & compatDetails );
	virtual ~CompatOptsDialog() override;

	static QStringVec getCmdArgsFromOptions( const CompatibilityDetails & compatDetails );

 private slots:

	// slots named like this don't need to be connected, they are connected automatically
	void on_crushedMonsters_toggled( bool checked );
	void on_friendlyMonsters_toggled( bool checked );
	void on_limitPainElem_toggled( bool checked );
	void on_monsterMovement_toggled( bool checked );
	void on_monstersCannotCross_toggled( bool checked );
	void on_monstersGetStuck_toggled( bool checked );
	void on_monstersSeeInvisible_toggled( bool checked );
	void on_noMinotaurFloor_toggled( bool checked );
	void on_spawnItemDrops_toggled( bool checked );

	void on_dehHealthSettings_toggled( bool checked );
	void on_originalAMushroom_toggled( bool checked );

	void on_allSpecialLines_toggled( bool checked );
	void on_allowAnyBossdeath_toggled( bool checked );
	void on_disableBoomDoor_toggled( bool checked );
	void on_findNeighboringLight_toggled( bool checked );
	void on_findShortestTextures_toggled( bool checked );
	void on_useBuggierStair_toggled( bool checked );
	void on_useDoomsFloor_toggled( bool checked );
	void on_useDoomsPointOnLine_toggled( bool checked );
	void on_levelExit_toggled( bool checked );

	void on_actorsAreInfinite_toggled( bool checked );
	void on_boomScrollers_toggled( bool checked );
	void on_cannotTravelStraight_toggled( bool checked );
	void on_enableWallRunning_toggled( bool checked );
	void on_ravenScrollers_toggled( bool checked );
	void on_selfRefSectors_toggled( bool checked );
	void on_useDoomHitscan_toggled( bool checked );
	void on_useDoomHeights_toggled( bool checked );

	void on_drawPolyobjects_toggled( bool checked );
	void on_ignoreYoffsets_toggled( bool checked );
	void on_invertSpriteSorting_toggled( bool checked );

	void on_crippleSound_toggled( bool checked );
	void on_dontLetOthersHear_toggled( bool checked );
	void on_instantMovingFloors_toggled( bool checked );
	void on_sectorSounds_toggled( bool checked );
	void on_soundsStop_toggled( bool checked );
	void on_useOriginalSound_toggled( bool checked );

	void on_compatflags1_line_textEdited( const QString & arg1 );
	void on_compatflags2_line_textEdited( const QString & arg1 );

 private: // methods

	void setFlag( const CompatFlag & flag, bool enabled );
	bool isEnabled( const CompatFlag & flag ) const;
	void updateCheckboxes();

 private:

	Ui::CompatOptsDialog * ui;

 public: // return value from this dialog

	CompatibilityDetails compatDetails;

};


#endif // COMPAT_OPTS_DIALOG_INCLUDED
