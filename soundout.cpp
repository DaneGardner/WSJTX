#include "soundout.h"

//#define FRAMES_PER_BUFFER 1024

extern "C" {
#include <portaudio.h>
}

extern float gran();                  //Noise generator (for tests only)
extern int itone[85];                 //Tx audio tones for 85 symbols
extern int icw[250];                  //Dits for CW ID
extern int outBufSize;
extern bool btxok;
extern bool btxMute;
extern double outputLatency;

typedef struct   //Parameters sent to or received from callback function
{
  double txsnrdb;
  int    nsps;
  int    ntrperiod;
  int    ntxfreq;
  int    ncall;
  bool   txMute;
  bool   bRestart;
  bool   btune;
} paUserData;

//--------------------------------------------------------------- d2aCallback
extern "C" int d2aCallback(const void *inputBuffer, void *outputBuffer,
                           unsigned long framesToProcess,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData )
{
  paUserData *udata=(paUserData*)userData;
  short *wptr = (short*)outputBuffer;

  static double twopi=2.0*3.141592653589793238462;
  static double baud;
  static double phi=0.0;
  static double dphi;
  static double freq;
  static double snr;
  static double fac;
  static double amp;
  static int ic=0,j=0;
  static short int i2;
  int isym,nspd;

  udata->ncall++;
  if(udata->bRestart) {
 // Time according to this computer
    qint64 ms = QDateTime::currentMSecsSinceEpoch() % 86400000;
    int mstr = ms % (1000*udata->ntrperiod );
    if(mstr<1000) return paContinue;
    ic=(mstr-1000)*48;
    udata->bRestart=false;
    srand(mstr);                                //Initialize random seed
  }
  isym=ic/(4*udata->nsps);                      //Actual fsample=48000
  if(udata->btune) isym=0;                      //If tuning, send pure tone
  if(udata->txsnrdb < 0.0) {
    snr=pow(10.0,0.05*(udata->txsnrdb-6.0));
    fac=3000.0;
    if(snr>1.0) fac=3000.0/snr;
  }

  if(isym>=85 and icw[0]>0) {              //Output the CW ID
    freq=udata->ntxfreq;
    dphi=twopi*freq/48000.0;
//    float wpm=20.0;
//    int nspd=1.2*48000.0/wpm;
//    nspd=3072;                           //18.75 WPM
    nspd=2048 + 512;                       //22.5 WPM
    int ic0=85*4*udata->nsps;
    for(uint i=0 ; i<framesToProcess; i++ )  {
      phi += dphi;
      if(phi>twopi) phi -= twopi;
      i2=32767.0*sin(phi);
      j=(ic-ic0)/nspd + 1;
      if(icw[j]==0) i2=0;
      if(udata->txsnrdb < 0.0) {
        int i4=fac*(gran() + i2*snr/32768.0);
        if(i4>32767) i4=32767;
        if(i4<-32767) i4=-32767;
        i2=i4;
      }
      if(!btxok or btxMute)  i2=0;
      *wptr++ = i2;                   //left
#ifdef UNIX
      *wptr++ = i2;                   //right
#endif
      ic++;
    }
    if(j>icw[0]) return paComplete;
    if(statusFlags==999999 and timeInfo==NULL and
       inputBuffer==NULL) return paContinue;   //Silence compiler warning:
    return paContinue;
  }

  baud=12000.0/udata->nsps;
  freq=udata->ntxfreq + itone[isym]*baud;
  dphi=twopi*freq/48000.0;
  amp=32767.0;
  int i0=84.983*4.0*udata->nsps;
  int i1=85*4*udata->nsps;
  if(udata->btune) {                           //If tuning, no ramp down
    i0=999*udata->nsps;
    i1=i0;
  }
  for(uint i=0 ; i<framesToProcess; i++ )  {
    phi += dphi;
    if(phi>twopi) phi -= twopi;
    if(ic>i0) amp=0.98*amp;
    if(ic>i1) amp=0.0;
    i2=amp*sin(phi);
    if(udata->txsnrdb < 0.0) {
      int i4=fac*(gran() + i2*snr/32768.0);
      if(i4>32767) i4=32767;
      if(i4<-32767) i4=-32767;
      i2=i4;
    }
    if(!btxok or btxMute)  i2=0;
    *wptr++ = i2;                   //left
#ifdef UNIX
    *wptr++ = i2;                   //right
#endif
    ic++;
  }
  if(amp==0.0) {
    if(icw[0]==0) return paComplete;
    phi=0.0;
  }
  return paContinue;
}

void SoundOutThread::run()
{
  PaError paerr;
  PaStreamParameters outParam;
  PaStream *outStream;
  paUserData udata;
  quitExecution = false;

  outParam.device=m_nDevOut;                 //Output device number
  outParam.channelCount=1;                   //Number of analog channels
#ifdef UNIX
  outParam.channelCount=2;                   //Number of analog channels
#endif
  outParam.sampleFormat=paInt16;             //Send short ints to PortAudio
  outParam.suggestedLatency=0.05;
  outParam.hostApiSpecificStreamInfo=NULL;

  paerr=Pa_IsFormatSupported(NULL,&outParam,48000.0);
  if(paerr<0) {
    qDebug() << "PortAudio says requested output format not supported.";
    qDebug() << paerr << m_nDevOut;
    return;
  }

  udata.txsnrdb=99.0;
  udata.nsps=m_nsps;
  udata.ntrperiod=m_TRperiod;
  udata.ntxfreq=m_txFreq;
  udata.ncall=0;
  udata.txMute=m_txMute;
  udata.bRestart=true;
  udata.btune=m_tune;

  paerr=Pa_OpenStream(&outStream,           //Output stream
        NULL,                               //No input parameters
        &outParam,                          //Output parameters
        48000.0,                            //Sample rate
        outBufSize,                         //Frames per buffer
        paClipOff,                          //No clipping
        d2aCallback,                        //output callbeck routine
        &udata);                            //userdata

  paerr=Pa_StartStream(outStream);
  if(paerr<0) {
    qDebug() << "Failed to start audio output stream.";
    return;
  }
  const PaStreamInfo* p=Pa_GetStreamInfo(outStream);
  outputLatency = p->outputLatency;
  bool qe = quitExecution;
  qint64 ms0 = QDateTime::currentMSecsSinceEpoch();

//---------------------------------------------- Soundcard output loop
  while (!qe) {
    qe = quitExecution;
    if (qe) break;

    udata.txsnrdb=m_txsnrdb;
    udata.nsps=m_nsps;
    udata.ntrperiod=m_TRperiod;
    udata.ntxfreq=m_txFreq;
    udata.txMute=m_txMute;
    udata.btune=m_tune;

    m_SamFacOut=1.0;
    if(udata.ncall>400) {
      qint64 ms = QDateTime::currentMSecsSinceEpoch();
      m_SamFacOut=udata.ncall*outBufSize*1000.0/(48000.0*(ms-ms0-50));
    }
    msleep(100);
  }
  Pa_StopStream(outStream);
  Pa_CloseStream(outStream);
}

void SoundOutThread::setOutputDevice(int n)      //setOutputDevice()
{
  if (isRunning()) return;
  this->m_nDevOut=n;
}

void SoundOutThread::setPeriod(int ntrperiod, int nsps)
{
  m_TRperiod=ntrperiod;
  m_nsps=nsps;
}

void SoundOutThread::setTxFreq(int n)
{
  m_txFreq=n;
}


void SoundOutThread::setTxSNR(double snr)
{
  m_txsnrdb=snr;
}

void SoundOutThread::setTune(bool b)
{
  m_tune=b;
}

double SoundOutThread::samFacOut()
{
  return m_SamFacOut;
}
