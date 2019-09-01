//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  13.5.2019
// Description:
//======================================================================================================================

#ifndef DM_FLAGS_DIALOG_INCLUDED
#define DM_FLAGS_DIALOG_INCLUDED


#include "Common.hpp"

#include <QDialog>


namespace Ui {
	class GameOptsDialog;
}


//======================================================================================================================

typedef uint8_t flagsIdx;
const flagsIdx dmflags1 = 0;
const flagsIdx dmflags2 = 1;
struct Flag {
    flagsIdx flags;
    uint32_t bit;
    bool defaultVal;
};


//======================================================================================================================

class GameOptsDialog : public QDialog {

    Q_OBJECT

    using thisClass = GameOptsDialog;

 public:

    explicit GameOptsDialog( QWidget * parent, uint32_t & dmflags1, uint32_t & dmflags2 );
    ~GameOptsDialog();

 protected:

    //virtual void closeEvent( QCloseEvent * event );

 private slots:

	// slots named like this don't need to be connected, they are connected automatically
    void on_fallingDamage_toggled( bool checked );
    void on_dropWeapon_toggled( bool checked );
    void on_doubleAmmo_toggled( bool checked );
    void on_infAmmo_toggled( bool checked );
    void on_infInventory_toggled( bool checked );
    void on_noMonsters_toggled( bool checked );
    void on_noMonstersToExit_toggled( bool checked );
    void on_monstersRespawn_toggled( bool checked );
    void on_noRespawn_toggled( bool checked );
    void on_itemsRespawn_toggled( bool checked );
    void on_bigPowerupsRespawn_toggled( bool checked );
    void on_fastMonsters_toggled( bool checked );
    void on_degeneration_toggled( bool checked );
    void on_allowAutoAim_toggled( bool checked );
    void on_allowSuicide_toggled( bool checked );
    void on_allowJump_stateChanged( int state );
    void on_allowCrouch_stateChanged( int state );
    void on_allowFreelook_stateChanged( int state );
    void on_allowFOV_toggled( bool checked );
    void on_allowBFGAiming_toggled( bool checked );
    void on_allowAutomap_toggled( bool checked );
    void on_automapAllies_toggled( bool checked );
    void on_allowSpying_toggled( bool checked );
    void on_chasecamCheat_toggled( bool checked );
    void on_checkAmmoForWeaponSwitch_toggled( bool checked );
    void on_iconsDeathKillsItsSpawns_toggled( bool checked );
    void on_endSectorCountsForKill_toggled( bool checked );

    void on_weaponsStay_toggled( bool checked );
    void on_allowPowerups_toggled( bool checked );
    void on_allowHealth_toggled( bool checked );
    void on_allowArmor_toggled( bool checked );
    void on_spawnFarthest_toggled( bool checked );
    void on_sameMap_toggled( bool checked );
    void on_forceRespawn_toggled( bool checked );
    void on_allowExit_toggled( bool checked );
    void on_barrelsRespawn_toggled( bool checked );
    void on_respawnProtection_toggled( bool checked );
    void on_loseFragIfFragged_toggled( bool checked );
    void on_keepFragsGained_toggled( bool checked );
    void on_noTeamSwitching_toggled( bool checked );

    void on_spawnMultiWeapons_toggled( bool checked );
    void on_loseEntireInventory_toggled( bool checked );
    void on_keepKeys_toggled( bool checked );
    void on_keepWeapons_toggled( bool checked );
    void on_keepArmor_toggled( bool checked );
    void on_keepPowerups_toggled( bool checked );
    void on_keepAmmo_toggled( bool checked );
    void on_loseHalfAmmo_toggled( bool checked );
    void on_spawnWhereDied_toggled( bool checked );

    void on_dmflags1_line_textEdited( const QString & arg1 );
    void on_dmflags2_line_textEdited( const QString & arg1 );

	void confirm();
	void cancel();

 private: // methods

    void setFlag( Flag flag, bool enabled );
    bool isEnabled( Flag flag );
    void updateCheckboxes();

 private: // members

    Ui::GameOptsDialog * ui;

    // dialog-local flags - need to be separate from the caller's flags, because the user might click Cancel
    uint32_t flags1;
    uint32_t flags2;

	// caller's flags (return values from this dialog) - will be updated only when user clicks Ok
    uint32_t & retFlags1;
    uint32_t & retFlags2;

};


#endif // DM_FLAGS_DIALOG_INCLUDED
