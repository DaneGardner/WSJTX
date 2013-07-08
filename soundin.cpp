#include "soundin.h"
#include <stdexcept>

#define FRAMES_PER_BUFFER 1024
//#define NSMAX 1365
#define NSMAX 6827
#define NTMAX 120

extern "C" {
#include <portaudio.h>
extern struct {
  float ss[184*NSMAX];              //This is "common/jt9com/..." in fortran
  float savg[NSMAX];
//  float c0[2*NTMAX*1500];
  short int d2[NTMAX*12000];
  int nutc;                         //UTC as integer, HHMM
  int ndiskdat;                     //1 ==> data read from *.wav file
  int ntrperiod;                    //TR period (seconds)
  int mousefqso;                    //User-selected QSO freq (kHz)
  int newdat;                       //1 ==> new data, must do long FFT
  int npts8;                        //npts in c0() array
  int nfa;                          //Low decode limit (Hz)
  int nfb;                          //High decode limit (Hz)
  int ntol;                         //+/- decoding range around fQSO (Hz)
  int kin;
  int nzhsym;
  int nsave;
  int nagain;
  int ndepth;
  int ntxmode;
  int nmode;
  char datetime[20];
} jt9com_;
}

typedef struct
{
  int kin;          //Parameters sent to/from the portaudio callback function
  int ncall;
  bool bzero;
  bool monitoring;
} paUserData;

//--------------------------------------------------------------- a2dCallback
extern "C" int a2dCallback( const void *inputBuffer, void *outputBuffer,
                         unsigned long framesToProcess,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData )

// This routine called by the PortAudio engine when samples are available.
// It may be called at interrupt level, so don't do anything
// that could mess up the system like calling malloc() or free().

{
  paUserData *udata=(paUserData*)userData;
  (void) outputBuffer;          //Prevent unused variable warnings.
  (void) timeInfo;
  (void) userData;
  int nbytes,k;

  udata->ncall++;
  if( (statusFlags&paInputOverflow) != 0) {
    qDebug() << "Input Overflow in a2dCallback";
  }
  if(udata->bzero) {           //Start of a new Rx sequence
    udata->kin=0;              //Reset buffer pointer
    udata->bzero=false;
  }

  nbytes=2*framesToProcess;        //Bytes per frame
  k=udata->kin;
  if(udata->monitoring) {
    memcpy(&jt9com_.d2[k],inputBuffer,nbytes);      //Copy all samples to d2
  }
  udata->kin += framesToProcess;
  jt9com_.kin=udata->kin;

  return paContinue;
}

void SoundInThread::run()                           //SoundInThread::run()
{
  quitExecution = false;

//---------------------------------------------------- Soundcard Setup
  PaError paerr;
  PaStreamParameters inParam;
  PaStream *inStream;
  paUserData udata;

  udata.kin=0;                              //Buffer pointer
  udata.ncall=0;                            //Number of callbacks
  udata.bzero=false;                        //Flag to request reset of kin
  udata.monitoring=m_monitoring;

  inParam.device=m_nDevIn;                  //### Input Device Number ###
  inParam.channelCount=1;                   //Number of analog channels
  inParam.sampleFormat=paInt16;             //Get i*2 from Portaudio
  inParam.suggestedLatency=0.05;
  inParam.hostApiSpecificStreamInfo=NULL;

  paerr=Pa_IsFormatSupported(&inParam,NULL,12000.0);
  if(paerr<0) {
    emit error("PortAudio says requested soundcard format not supported.");
//    return;
  }
  qDebug() << "";
  paerr=Pa_OpenStream(&inStream,            //Input stream
        &inParam,                           //Input parameters
        NULL,                               //No output parameters
        12000.0,                            //Sample rate
        FRAMES_PER_BUFFER,                  //Frames per buffer
//        paClipOff+paDitherOff,            //No clipping or dithering
        paClipOff,                          //No clipping
        a2dCallback,                        //Input callback routine
        &udata);                            //userdata

  paerr=Pa_StartStream(inStream);
  if(paerr<0) {
    emit error("Failed to start audio input stream.");
    return;
  }

  bool qe = quitExecution;
  static int ntr0=99;
  int k=0;
  int nsec;
  int ntr;
  int nBusy=0;
  int nstep0=0;
  int nsps0=0;
  qint64 ms0 = QDateTime::currentMSecsSinceEpoch();

//---------------------------------------------- Soundcard input loop
  while (!qe) {
    qe = quitExecution;
    if (qe) break;
    udata.monitoring=m_monitoring;
    qint64 ms = QDateTime::currentMSecsSinceEpoch();
    m_SamFacIn=1.0;
    if(udata.ncall>100) {
      m_SamFacIn=udata.ncall*FRAMES_PER_BUFFER*1000.0/(12000.0*(ms-ms0-50));
    }
    ms=ms % 86400000;
    nsec = ms/1000;             // Time according to this computer
    ntr = nsec % m_TRperiod;

// Reset buffer pointer and symbol number at start of minute
    if(ntr < ntr0 or !m_monitoring or m_nsps!=nsps0) {
      nstep0=0;
      nsps0=m_nsps;
      udata.bzero=true;
    }
    k=udata.kin;
    if(m_monitoring) {
      int kstep=m_nsps/2;
//      m_step=k/kstep;
      m_step=(k-1)/kstep;
      if(m_step != nstep0) {
        if(m_dataSinkBusy) {
          nBusy++;
        } else {
//          m_dataSinkBusy=true;
//          emit readyForFFT(k);         //Signal to compute new FFTs
          emit readyForFFT(k-1);         //Signal to compute new FFTs
        }
        nstep0=m_step;
      }
    }
    msleep(100);
    ntr0=ntr;
  }
  Pa_StopStream(inStream);
  Pa_CloseStream(inStream);
}

void SoundInThread::setInputDevice(int n)                  //setInputDevice()
{
  if (isRunning()) return;
  this->m_nDevIn=n;
}

void SoundInThread::quit()                                       //quit()
{
  quitExecution = true;
}

void SoundInThread::setMonitoring(bool b)                    //setMonitoring()
{
  m_monitoring = b;
}

void SoundInThread::setPeriod(int ntrperiod, int nsps)
{
  m_TRperiod=ntrperiod;
  m_nsps=nsps;
}

int SoundInThread::mstep()
{
  return m_step;
}

double SoundInThread::samFacIn()
{
  return m_SamFacIn;
}
