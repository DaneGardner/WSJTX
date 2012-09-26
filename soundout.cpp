#include "soundout.h"

#define FRAMES_PER_BUFFER 256

extern "C" {
#include <portaudio.h>
}

extern float gran();                  //Noise generator (for tests only)
extern short int iwave[120*12000];     //Wave file for Tx audio
extern int nwave;
extern bool btxok;
extern double outputLatency;

typedef struct   //Parameters sent to or received from callback function
{
  int dummy;
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
  unsigned int i;
  static int ic=0;

  for(i=0 ; i<framesToProcess; i++ )  {
    short int i2=iwave[ic];
    if(ic > nwave) i2=0;
//      i2 = 500.0*(i2/32767.0 + 5.0*gran());      //Add noise (tests only!)
    if(!btxok) i2=0;
    *wptr++ = i2;                   //left
    ic++;
  }
  if(ic >= nwave) {
    ic=0;
  }
  return 0;
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
  outParam.sampleFormat=paInt16;             //Send short ints to PortAudio
  outParam.suggestedLatency=0.05;
  outParam.hostApiSpecificStreamInfo=NULL;

  paerr=Pa_IsFormatSupported(NULL,&outParam,12000.0);
  if(paerr<0) {
    qDebug() << "PortAudio says requested output format not supported.";
    qDebug() << paerr << m_nDevOut;
    return;
  }

//  udata.nwave=m_nwave;
//  udata.btxok=false;

  paerr=Pa_OpenStream(&outStream,           //Output stream
        NULL,                               //No input parameters
        &outParam,                          //Output parameters
        12000.0,                            //Sample rate
        FRAMES_PER_BUFFER,                  //Frames per buffer
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

//---------------------------------------------- Soundcard output loop
  while (!qe) {
    qe = quitExecution;
    if (qe) break;
//    udata.nwave=m_nwave;
//    if(m_txOK) udata.btxok=1;
//    if(!m_txOK) udata.btxok=0;
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
