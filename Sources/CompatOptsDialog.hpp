//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  13.5.2019
// Description:
//======================================================================================================================

#ifndef COMPAT_OPTS_DIALOG_INCLUDED
#define COMPAT_OPTS_DIALOG_INCLUDED


#include "Common.hpp"

#include "SharedData.hpp"

#include <QDialog>


namespace Ui {
	class CompatOptsDialog;
}

struct CompatFlag;


//======================================================================================================================

class CompatOptsDialog : public QDialog {

	Q_OBJECT

	using thisClass = CompatOptsDialog;

 public:

	explicit CompatOptsDialog( QWidget * parent, CompatibilityOptions & compatOpts );
	~CompatOptsDialog();

	static QString getCmdArgsFromOptions( const CompatibilityOptions & compatOpts );

 protected:

	//virtual void closeEvent( QCloseEvent * event );

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

	void confirm();
	void cancel();

 private: // methods

	void setFlag( const CompatFlag & flag, bool enabled );
	bool isEnabled( const CompatFlag & flag ) const;
	void updateCheckboxes();

 private:

	Ui::CompatOptsDialog * ui;

	// dialog-local options - need to be separate from the caller's options, because the user might click Cancel
	CompatibilityOptions compatOpts;

	// caller's options (return value from this dialog) - will be updated only when user clicks Ok
	CompatibilityOptions & retCompatOpts;

};


#endif // COMPAT_OPTS_DIALOG_INCLUDED
