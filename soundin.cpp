#include "soundin.h"
#include <stdexcept>

#define FRAMES_PER_BUFFER 1024

extern "C" {
#include <portaudio.h>
extern struct {
  short int d2[1800*12000];         //This is "common/jt8com/..." in fortran
  float ss[184*4400];
  float savg[4400];
  double fcenter;                   //USB dial freq (kHz)
  int nutc;                         //UTC as integer, HHMM
  int ntrperiod;                    //TR period (seconds)
  int mousedf;                      //User-selected DF
  int mousefqso;                    //User-selected QSO freq (kHz)
  int nagain;                       //1 ==> decode only at fQSO +/- Tol
  int ndepth;                       //How much hinted decoding to do?
  int ndiskdat;                     //1 ==> data read from *.tf2 or *.iq file
  int newdat;                       //1 ==> new data, must do long FFT
  int nfa;                          //Low decode limit (kHz)
  int nfb;                          //High decode limit (kHz)
  int ntol;                         //+/- decoding range around fQSO (Hz)
  int map65RxLog;                   //Flags to control log files
  int nfsample;                     //Input sample rate
  int nsave;                        //Number of s3(64,63) spectra saved
  int kin;
  int kline;
  char datetime[20];
} jt8com_;
}

typedef struct
{
  int kin;          //Parameters sent to/from the portaudio callback function
  bool bzero;
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

//  if(framesToProcess != -99)   return paContinue;    //###

  if( (statusFlags&paInputOverflow) != 0) {
    qDebug() << "Input Overflow";
  }
  if(udata->bzero) {           //Start of a new Rx sequence
    udata->kin=0;              //Reset buffer pointer
    udata->bzero=false;
  }

  nbytes=2*framesToProcess;        //Bytes per frame
  k=udata->kin;
  memcpy(&jt8com_.d2[k],inputBuffer,nbytes);          //Copy all samples to d2
  udata->kin += framesToProcess;
  jt8com_.kin=udata->kin;

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
  udata.bzero=false;                        //Flag to request reset of kin

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
  paerr=Pa_OpenStream(&inStream,            //Input stream
        &inParam,                           //Input parameters
        NULL,                               //No output parameters
        12000.0,                            //Sample rate
        FRAMES_PER_BUFFER,                  //Frames per buffer
//        paClipOff+paDitherOff,            //No clipping or dithering
        paClipOff,                          //No clipping
        a2dCallback,                        //Input callbeck routine
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

//---------------------------------------------- Soundcard input loop
  while (!qe) {
    qe = quitExecution;
    if (qe) break;
    qint64 ms = QDateTime::currentMSecsSinceEpoch() % 86400000;
    nsec = ms/1000;             // Time according to this computer
    ntr = nsec % m_TRperiod;

// Reset buffer pointer and symbol number at start of minute
    if(ntr < ntr0 or !m_monitoring) {
      nstep0=0;
      udata.bzero=true;
    }
    k=udata.kin;
    if(m_monitoring) {
      int kstep=m_nsps/2;
      m_step=k/kstep;
      if(m_step != nstep0) {
        if(m_dataSinkBusy) {
          nBusy++;
        } else {
//          m_dataSinkBusy=true;
          emit readyForFFT(k);         //Signal to compute new FFTs
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
