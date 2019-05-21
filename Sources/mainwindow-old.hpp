#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <cstdint>

#include <QMainWindow>
#include <QDir>
#include <QString>
#include <QHash>
#include <QListWidgetItem>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {

    Q_OBJECT

 public:

    explicit MainWindow( QWidget * parent = 0 );
    ~MainWindow();

 protected:

    virtual void closeEvent( QCloseEvent * event );
    virtual void timerEvent( QTimerEvent * event );

 private slots:

    void addSourcePort();
    void delSourcePort();
    void selectSourcePort( QString port );

    void toggleUpdateIWADs( bool enabled );
    void changeUpdateIWADsDir( QString dir );
    void browseIWADs();
    void addIWAD();
    void delIWAD();
    void upIWAD();
    void downIWAD();
    void selectIWAD( QString text );

    void toggleUpdatePWADs( bool enabled );
    void changeUpdatePWADsDir( QString dir );
    void browsePWADs();
    void addPWAD();
    void delPWAD();
    void upPWAD();
    void downPWAD();
    void selectPWAD( QListWidgetItem * item );

    void toggleUpdateMods( bool enabled );
    void changeUpdateModsDir( QString dir );
    void browseMods();
    void addMod();
    void delMod();
    void upMod();
    void downMod();
    void selectMod( QListWidgetItem * item );

    void selectMultRole( int role );
    void changePlayerCount( int number );
    void selectGameMode( int index );
    void changeTeamDamage( double number );
    void changeTimeLimit( int number );
    void selectNetMode( int number );
    void changeIP( QString text );
    void changePort( int number );

    void toggleDirectStart( bool enabled );
    void toggleLoadGame( bool enabled );
    void changeSaveFile( QString text );
    void changeMap( QString text );
    void changeSkill( int number );
    void changeSkillNum( int number );
    void toggleNoMonsters( bool enabled );
    void toggleFastMonsters( bool enabled );
    void toggleMonstersRespawn( bool enabled );
    void showDMFlags();
    void showCompatFlags();
    void setDMFLags( uint32_t flags1, uint32_t flags2 );
    void setCompatFlags( uint32_t flags1, uint32_t flags2 );

    void toggleRelativePaths( bool relative );
    void changeCmdArgs( QString text );
    void saveOptions();
    void loadOptions();
    void exportOptions();
    void importOptions();

    void launch();

 private: //methods

    bool saveWindowProperties();
    bool loadWindowProperties();
    bool saveOptionsToFile( QString fileName );
    bool loadOptionsFromFile( QString fileName );
    void autoSaveOptions();

    void updateListFromDir( QListWidget * list, QString & prevDir, QString newDir, QHash<QString,QString> & paths, bool checkable );
    void reinsertListFromDir( QListWidget * list, QDir newDir, QHash<QString,QString> & paths, bool checkable );
    void correctListFromDir( QListWidget * list, QDir newDir, QHash<QString,QString> & paths, bool checkable );
    QListWidgetItem * addListItem( QListWidget * list, QString text, bool checkable );
    void moveUpListItem( QListWidget * list );
    void moveDownListItem( QListWidget * list );

    void updateMaps();
    void updateSaveFiles();

    void genLaunchCommand();
    void convertPathsToRelative();
    void convertPathsToAbsolute();
    QString getPath( QString path );
    QString getAbsPath( QString path );
    QString getRelPath( QString path );
    QString getMapNumber( QString mapName );

 private: //members

    Ui::MainWindow *ui;

    QDir currentDir;

    QHash<QString,QString> ports;
    QHash<QString,QString> IWADs;
    QString prevIWADsDir;
    QHash<QString,QString> PWADs;
    QString prevPWADsDir;
    QHash<QString,QString> mods;
    QString prevModsDir;

    uint32_t dmflags1;
    uint32_t dmflags2;
    uint32_t compatflags1;
    uint32_t compatflags2;

    int tickCount;

};

#endif // MAINWINDOW_H
