#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QtGui>
#include <QLabel>
#include <QTimer>
#include <QDateTime>
#include <QHash>
#include "soundin.h"
#include "soundout.h"
#include "commons.h"
#include "sleep.h"

#define NFFT 32768
#define NSMAX 5760000

//--------------------------------------------------------------- MainWindow
namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();
  bool m_network;

public slots:
  void showSoundInError(const QString& errorMsg);
  void showStatusMessage(const QString& statusMsg);
  void dataSink(int k);
  void diskDat();
  void diskWriteFinished();
  void freezeDecode(int n);
  void guiUpdate();
  void doubleClickOnCall(QString hiscall, bool ctrl);

protected:
  virtual void keyPressEvent( QKeyEvent *e );
  void  closeEvent(QCloseEvent*);
  virtual bool eventFilter(QObject *object, QEvent *event);

private slots:
  void on_tx1_editingFinished();
  void on_tx2_editingFinished();
  void on_tx3_editingFinished();
  void on_tx4_editingFinished();
  void on_tx5_editingFinished();
  void on_tx6_editingFinished();
  void on_actionDeviceSetup_triggered();
  void on_monitorButton_clicked();
  void on_actionExit_triggered();
  void on_actionAbout_triggered();
  void OnExit();
  void on_actionLinrad_triggered();
  void on_actionCuteSDR_triggered();
  void on_autoButton_clicked();
  void on_stopTxButton_clicked();
  void on_tolSpinBox_valueChanged(int arg1);
  void on_stopButton_clicked();
  void on_actionOnline_Users_Guide_triggered();
  void on_actionWide_Waterfall_triggered();
  void on_actionOpen_triggered();
  void on_actionOpen_next_in_directory_triggered();
  void on_actionDecode_remaining_files_in_directory_triggered();
  void on_actionDelete_all_wav_files_in_SaveDir_triggered();
  void on_actionFind_Delta_Phi_triggered();
  void on_actionF4_sets_Tx6_triggered();
  void on_actionOnly_EME_calls_triggered();
  void on_actionNo_shorthands_if_Tx1_triggered();
  void on_actionNo_Deep_Search_triggered();
  void on_actionNormal_Deep_Search_triggered();
  void on_actionAggressive_Deep_Search_triggered();
  void on_actionNone_triggered();
  void on_actionSave_all_triggered();
  void on_actionKeyboard_shortcuts_triggered();
  void on_actionSpecial_mouse_commands_triggered();
  void on_actionAvailable_suffixes_and_add_on_prefixes_triggered();
  void on_DecodeButton_clicked();
  void decode();
  void decodeBusy(bool b);
  void on_EraseButton_clicked();
  void on_txb1_clicked();
  void on_txFirstCheckBox_stateChanged(int arg1);
  void set_ntx(int n);
  void on_txb2_clicked();
  void on_txb3_clicked();
  void on_txb4_clicked();
  void on_txb5_clicked();
  void on_txb6_clicked();
  void on_lookupButton_clicked();
  void on_addButton_clicked();
  void on_dxCallEntry_textChanged(const QString &arg1);
  void on_dxGridEntry_textChanged(const QString &arg1);
  void selectCall2(bool ctrl);
  void on_genStdMsgsPushButton_clicked();
  void bumpDF(int n);
  void on_logQSOButton_clicked();
//  void on_actionErase_wsjt-x_rx_log_triggered();
  void on_actionErase_wsjtx_tx_log_triggered();
  void on_actionAFMHot_triggered();
  void on_actionBlue_triggered();

private:
    Ui::MainWindow *ui;
    qint32  m_nDevIn;
    qint32  m_nDevOut;
    qint32  m_idInt;
    qint32  m_waterfallAvg;
    qint32  m_DF;
    qint32  m_tol;
    qint32  m_QSOfreq0;
    qint32  m_ntx;
    qint32  m_pttPort;
    qint32  m_timeout;
    qint32  m_dPhi;
    qint32  m_fCal;
    qint32  m_txFreq;
    qint32  m_setftx;
    qint32  m_ndepth;
    qint32  m_sec0;
    qint32  m_RxLog;
    qint32  m_nutc0;
    qint32  m_mode65;
    qint32  m_nrx;
    qint32  m_hsym0;
    qint32  m_paInDevice;
    qint32  m_paOutDevice;
    qint32  m_udpPort;
    qint32  m_NBslider;
    qint32  m_adjustIQ;
    qint32  m_applyIQcal;
    qint32  m_mult570;

    double  m_fAdd;
    double  m_IQamp;
    double  m_IQphase;
    double  m_cal570;

    bool    m_monitoring;
    bool    m_transmitting;
    bool    m_diskData;
    bool    m_loopall;
    bool    m_decoderBusy;
    bool    m_txFirst;
    bool    m_auto;
    bool    m_txMute;
    bool    m_restart;
    bool    m_killAll;
    bool    m_xpol;
    bool    m_xpolx;
    bool    m_call3Modified;
    bool    m_startAnother;
    bool    m_saveAll;
    bool    m_onlyEME;
    bool    m_widebandDecode;
    bool    m_kb8rq;
    bool    m_NB;
    bool    m_fs96000;
    bool    m_IQswap;
    bool    m_10db;
    bool    m_initIQplus;

    float   m_gainx;
    float   m_gainy;
    float   m_phasex;
    float   m_phasey;
    float   m_pctZap;

    QRect   m_wideGraphGeom;

    QLabel* lab1;                            // labels in status bar
    QLabel* lab2;
    QLabel* lab3;
    QLabel* lab4;
    QLabel* lab5;
    QLabel* lab6;

    QMessageBox msgBox0;

    QFuture<void>* future1;
    QFuture<void>* future2;
    QFutureWatcher<void>* watcher1;
    QFutureWatcher<void>* watcher2;

    QProcess proc_m65;
    QProcess proc_qthid;

    QString m_path;
    QString m_pbdecoding_style1;
    QString m_pbmonitor_style;
    QString m_pbAutoOn_style;
    QString m_myCall;
    QString m_myGrid;
    QString m_hisCall;
    QString m_hisGrid;
    QString m_appDir;
    QString m_saveDir;
    QString m_azelDir;
    QString m_dxccPfx;
    QString m_palette;
    QString m_dateTime;
    QString m_mode;
    QString m_colors;

    QHash<QString,bool> m_worked;

    SoundInThread soundInThread;             //Instantiate the audio threads
    SoundOutThread soundOutThread;

//---------------------------------------------------- private functions
    void readSettings();
    void writeSettings();
    void createStatusBar();
    void updateStatusBar();
    void msgBox(QString t);
    void genStdMsgs(QString rpt);
    void lookup();
    void ba2msg(QByteArray ba, char* message);
    void msgtype(QString t, QLineEdit* tx);
    void stub();
};

extern void getfile(QString fname, bool xpol, int idInt);
extern void savewav(QString fname);
extern int killbyname(const char* progName);
extern void getDev(int* numDevices,char hostAPI_DeviceName[][50],
                   int minChan[], int maxChan[],
                   int minSpeed[], int maxSpeed[]);

extern "C" {
//----------------------------------------------------- C and Fortran routines
  void specjtms_(int* k, float* px, float* pxsmo, float* spk0, float* f0);
  void genjtms3_(char* message, short iwave[], int* nwave, int len1);
  void genmsk_(char* message, short iwave[], int* nwave, int len1);
  void gen65_(char* msg, int* mode65, double* samfac, int* nsendingsh,
              char* msgsent, short iwave[], int* nwave, int len1, int len2);
  int ptt_(int* nport, int* itx, int* iptt);
  }

#endif // MAINWINDOW_H
