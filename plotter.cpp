#include "plotter.h"
#include <math.h>
#include <QDebug>

#define MAX_SCREENSIZE 2048


CPlotter::CPlotter(QWidget *parent) :                  //CPlotter Constructor
  QFrame(parent)
{
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  setFocusPolicy(Qt::StrongFocus);
  setAttribute(Qt::WA_PaintOnScreen,false);
  setAutoFillBackground(false);
  setAttribute(Qt::WA_OpaquePaintEvent, false);
  setAttribute(Qt::WA_NoSystemBackground, true);

  m_CenterFreq = 1500;
  m_StartFreq = 1000;
  m_nSpan=1000;                    //Units: Hz
  m_fSpan=(float)m_nSpan;
  m_hdivs = HORZ_DIVS;
  m_FreqUnits = 1;
  m_Running = false;
  m_paintEventBusy=false;
  m_WaterfallPixmap = QPixmap(0,0);
  m_ZoomWaterfallPixmap = QPixmap(0,0);
  m_2DPixmap = QPixmap(0,0);
  m_ScalePixmap = QPixmap(0,0);
  m_ZoomScalePixmap = QPixmap(0,0);
  m_Size = QSize(0,0);
  m_fQSO = 125;
  m_line = 0;
  m_fSample = 12000;
  m_nsps=6912;
  m_paintAllZoom = false;
}

CPlotter::~CPlotter() { }                                      // Destructor

QSize CPlotter::minimumSizeHint() const
{
  return QSize(50, 50);
}

QSize CPlotter::sizeHint() const
{
  return QSize(180, 180);
}

void CPlotter::resizeEvent(QResizeEvent* )                    //resizeEvent()
{
  if(!size().isValid()) return;
  if( m_Size != size() ) {
    //if changed, resize pixmaps to new screensize
    m_Size = size();
    int w = m_Size.width();
    int h = (m_Size.height()-60)/2;
    m_WaterfallPixmap = QPixmap(w,h);
    m_ZoomWaterfallPixmap = QPixmap(w,h);
    m_2DPixmap = QPixmap(w,h);
    m_WaterfallPixmap.fill(Qt::black);
    m_ZoomWaterfallPixmap.fill(Qt::black);
    m_2DPixmap.fill(Qt::black);
    memset(m_zwf,0,NSMAX*h);
    m_ScalePixmap = QPixmap(w,30);
    m_ZoomScalePixmap = QPixmap(w,30);    //(no change on resize...)
    m_ScalePixmap.fill(Qt::white);
    m_ZoomScalePixmap.fill(Qt::yellow);
  }
  SetCenterFreq(-1);
  DrawOverlay();
}

void CPlotter::paintEvent(QPaintEvent *)                    // paintEvent()
{
  static int x00=-99;

  if(m_paintEventBusy) return;
  m_paintEventBusy=true;
  QPainter painter(this);
  int w = m_Size.width();
  int h = (m_Size.height()-60)/2;
  painter.drawPixmap(0,0,m_ScalePixmap);
  painter.drawPixmap(0,30,m_WaterfallPixmap);
  painter.drawPixmap(0,h+30,m_ScalePixmap);
  painter.drawPixmap(0,h+60,m_2DPixmap);
  m_paintEventBusy=false;
}

void CPlotter::draw(float swide[], int i0, float splot[])             //draw()
{
  int i,j,w,h;
  float y;

  m_i0=i0;
  w = m_WaterfallPixmap.width();
  h = m_WaterfallPixmap.height();
  double gain = pow(10.0,0.05*(m_plotGain+7));

  //move current data down one line
  //(must do this before attaching a QPainter object)
  m_WaterfallPixmap.scroll(0,1,0,0,w,h);
  m_ZoomWaterfallPixmap.scroll(0,1,0,0, w, h);
  memmove(&m_zwf[32768],m_zwf,32768*(h-1));
  QPainter painter1(&m_WaterfallPixmap);
  QPainter painter2D(&m_2DPixmap);

  for(i=0; i<256; i++) {                     //Zero the histograms
    m_hist1[i]=0;
    m_hist2[i]=0;
  }

//  qDebug() << "A" << m_binsPerPixel << m_nSpan << m_fSpan;
  painter2D.setPen(Qt::green);
  QRect tmp(0,0,w,h);
  painter2D.fillRect(tmp,Qt::black);
  QPoint LineBuf[MAX_SCREENSIZE];
  j=0;
  bool strong0=false;
  bool strong=false;

  for(i=0; i<w; i++) {
    strong=false;
    if(swide[i]<0) {
      strong=true;
      swide[i]=-swide[i];
    }
    y = 10.0*log10(swide[i]);
    int y1 = 5.0*gain*(y + 29 -m_plotZero);
    if (y1<0) y1=0;
    if (y1>254) y1=254;
    if (swide[i]>1.e29) y1=255;
    m_hist1[y1]++;
    painter1.setPen(m_ColorTbl[y1]);
    painter1.drawPoint(i,0);
    int y2 = gain*(y + 34 -m_plotZero);
    if (y2<0) y2=0;
    if (y2>254) y2=254;
    if (swide[i]>1.e29) y2=255;
    if(strong != strong0 or i==w-1) {
      painter2D.drawPolyline(LineBuf,j);
      j=0;
      strong0=strong;
      if(strong0) painter2D.setPen(Qt::red);
      if(!strong0) painter2D.setPen(Qt::green);
    }
    LineBuf[j].setX(i);
    LineBuf[j].setY(h-y2);
    j++;
  }

  for(i=0; i<NSMAX; i++) {
    y = 10.0*log10(splot[i]);
    int y1 = 5.0*gain*(y + 29 -m_plotZero);
    if (y1<0) y1=0;
    if (y1>254) y1=254;
    if (splot[i]>1.e29) y1=255;
    m_hist2[y1]++;
    m_zwf[i]=y1;
  }

  if(swide[0]>1.0e29) m_line=0;
  m_line++;
  if(m_line == 13) {
    UTCstr();
    painter1.setPen(Qt::white);
    painter1.drawText(5,10,m_sutc);
    m_paintAllZoom=true;
  }
  update();                              //trigger a new paintEvent
}

void CPlotter::UTCstr()
{
  int ihr,imin,isec;
  if(jt8com_.ndiskdat != 0) {
    ihr=jt8com_.nutc/10000;
    imin=(jt8com_.nutc/100) % 100;
  } else {
    qint64 ms = QDateTime::currentMSecsSinceEpoch() % 86400000;
    imin=ms/60000;
    ihr=imin/60;
    imin=imin % 60;
  }
  if(isec<30) isec=0;
  if(isec>=30) isec=30;
  sprintf(m_sutc,"%2.2d:%2.2d",ihr,imin);
}

void CPlotter::DrawOverlay()                                 //DrawOverlay()
{
  if(m_WaterfallPixmap.isNull()) return;
  int w = m_WaterfallPixmap.width();
  int x,y;
  int nHzDiv[11]={0,50,100,200,200,200,500,500,500,500,500};
  float pixperdiv;

  QRect rect0;
  QPainter painter0(&m_ScalePixmap);
  painter0.initFrom(this);

  //create Font to use for scales
  QFont Font("Arial");
  Font.setPointSize(12);
  QFontMetrics metrics(Font);
  Font.setWeight(QFont::Normal);
  painter0.setFont(Font);
  painter0.setPen(Qt::black);

  double fftBinWidth=12000.0/m_nsps;
  if(m_binsPerPixel < 1) m_binsPerPixel=1;
  double df = m_binsPerPixel*fftBinWidth;
  m_fSpan = w*df;
//  m_freqPerDiv=50*m_binsPerPixel;
//  m_freqPerDiv=nHzDiv[m_binsPerPixel];
  int n=m_fSpan/10;
  m_freqPerDiv=10;
  if(n>25) m_freqPerDiv=50;
  if(n>70) m_freqPerDiv=100;
  if(n>250) m_freqPerDiv=500;
  m_hdivs = w*df/m_freqPerDiv + 0.9999;
//  qDebug() << "B" << m_binsPerPixel << df << m_freqPerDiv << pixperdiv << m_hdivs << m_fSpan;
  m_ScalePixmap.fill(Qt::white);
  painter0.drawRect(0, 0, w, 30);

  //draw tick marks on wideband (upper) scale
//  pixperdiv = m_binsPerPixel*m_freqPerDiv/df;
  pixperdiv = m_freqPerDiv/df;
  for( int i=1; i<m_hdivs; i++) {     //major ticks
    x = (int)( (float)i*pixperdiv );
    painter0.drawLine(x,18,x,30);
  }
  for( int i=1; i<5*m_hdivs; i++) {   //minor ticks
    x = i*pixperdiv/5.0;
    painter0.drawLine(x,24,x,30);
  }

  //draw frequency values
  MakeFrequencyStrs();
  for( int i=0; i<=m_hdivs; i++) {
    if(0==i) {
      //left justify the leftmost text
      x = (int)( (float)i*pixperdiv);
      rect0.setRect(x,0, (int)pixperdiv, 20);
      painter0.drawText(rect0, Qt::AlignLeft|Qt::AlignVCenter,
                       m_HDivText[i]);
    }
    else if(m_hdivs == i) {
      //right justify the rightmost text
      x = (int)( (float)i*pixperdiv - pixperdiv);
      rect0.setRect(x,0, (int)pixperdiv, 20);
      painter0.drawText(rect0, Qt::AlignRight|Qt::AlignVCenter,
                       m_HDivText[i]);
    } else {
      //center justify the rest of the text
      x = (int)( (float)i*pixperdiv - pixperdiv/2);
      rect0.setRect(x,0, (int)pixperdiv, 20);
      painter0.drawText(rect0, Qt::AlignHCenter|Qt::AlignVCenter,
                       m_HDivText[i]);
    }
  }


  QPen pen0(Qt::green, 3);                 //Mark Cal Freq with green tick
  painter0.setPen(pen0);
  x = m_xClick;
  painter0.drawLine(x,15,x,30);
  int x0=(16384-m_i0)/m_binsPerPixel;
  m_fGreen=(x-x0)*df;
  x0 += (x0-x);
  QPen pen3(Qt::red, 3);
  painter0.setPen(pen3);
  if(x0>0 and x0<w) painter0.drawLine(x0,15,x0,30);

  // Now make the zoomed scale, using m_ZoomScalePixmap and painter3
  QRect rect1;
  QPainter painter3(&m_ZoomScalePixmap);
  painter3.initFrom(this);
  painter3.setFont(Font);
  painter3.setPen(Qt::black);

  df = 12000.0/m_nsps;
  m_hdivs = 4400*df/m_freqPerDiv + 0.9999;
  int nlabs=df*w/m_freqPerDiv + 1.0;
  m_ZoomScalePixmap.fill(Qt::white);
  painter3.drawRect(0, 0, w, 30);

  pixperdiv = m_freqPerDiv/df;
  for( int i=0; i<10*nlabs; i++) {
    x = i*pixperdiv/10;
    y=24;
    if ((i%10) == 0) y=18;
    painter3.drawLine(x,y,x,30);
  }

  //draw frequency values
  MakeFrequencyStrs();
  for( int i=0; i<=nlabs; i++) {
    x = (int)( (float)i*pixperdiv - pixperdiv/2);
    rect1.setRect(x,0, (int)pixperdiv, 20);
    painter3.drawText(rect1, Qt::AlignHCenter|Qt::AlignVCenter,
                      m_HDivText[i]);
  }
}

void CPlotter::MakeFrequencyStrs()                       //MakeFrequencyStrs
{
  float freq;
  int i,j;

  for(int i=0; i<=m_hdivs; i++) {
    freq = m_StartFreq + i*m_freqPerDiv;
    m_HDivText[i].setNum((int)freq);
    //      StartFreq += m_freqPerDiv;
  }
}

int CPlotter::XfromFreq(float f)                               //XfromFreq()
{
  float w = m_WaterfallPixmap.width();
  int x = (int) w * (f - m_StartFreq)/m_fSpan;
  if(x<0 ) return 0;
  if(x>(int)w) return m_WaterfallPixmap.width();
  return x;
}

float CPlotter::FreqfromX(int x)                               //FreqfromX()
{
  float w = m_WaterfallPixmap.width();
  float f =m_CenterFreq - 0.5*m_fSpan + m_fSpan * x/w;
  return f;
}

void CPlotter::SetRunningState(bool running)              //SetRunningState()
{
  m_Running = running;
}

void CPlotter::setPlotZero(int plotZero)                  //setPlotZero()
{
  m_plotZero=plotZero;
}

int CPlotter::getPlotZero()                               //getPlotZero()
{
  return m_plotZero;
}

void CPlotter::setPlotGain(int plotGain)                  //setPlotGain()
{
  m_plotGain=plotGain;
}

int CPlotter::getPlotGain()                               //getPlotGain()
{
  return m_plotGain;
}

void CPlotter::SetCenterFreq(int f)                   //setCenterFreq()
{
}

qint64 CPlotter::centerFreq()                             //centerFreq()
{
  return m_CenterFreq;
}

void CPlotter::SetStartFreq(quint64 f)                    //SetStartFreq()
{
  m_StartFreq=f;
//    resizeEvent(NULL);
  DrawOverlay();
}

qint64 CPlotter::startFreq()                              //startFreq()
{
  return m_StartFreq;
}

int CPlotter::plotWidth(){return m_WaterfallPixmap.width();}
void CPlotter::UpdateOverlay() {DrawOverlay();}
void CPlotter::setDataFromDisk(bool b) {m_dataFromDisk=b;}

void CPlotter::setTol(int n)                                 //setTol()
{
  m_tol=n;
  DrawOverlay();
}

void CPlotter::setBinsPerPixel(int n)                       // set nbpp
{
  m_binsPerPixel = n;
  DrawOverlay();                         //Redraw scales and ticks
  update();                              //trigger a new paintEvent}
}

int CPlotter::binsPerPixel(){return m_binsPerPixel;}         //get nbpp

void CPlotter::setFQSO(int x, bool bf)                       //setFQSO()
{
  if(bf) {
    m_fQSO=x;         // x is freq in kHz
  } else {
    if(x<0) x=0;      // x is pixel number
    if(x>m_Size.width()) x=m_Size.width();
    m_fQSO = int(FreqfromX(x)+0.5);
    m_xClick=x;
  }
  DrawOverlay();
  update();
}

void CPlotter::setFcal(int n)                                  //setFcal()
{
  m_fCal=n;
}

void CPlotter::setNkhz(int n)                                  //setNkhz()
{
  m_nkhz=n;
}

int CPlotter::fQSO() {return m_fQSO;}                          //get fQSO

int CPlotter::DF() {return m_DF;}                              // get DF

void CPlotter::mousePressEvent(QMouseEvent *event)       //mousePressEvent
{
  int h = (m_Size.height()-60)/2;
  int x=event->x();
  int y=event->y();
  if(y < h+30) {
    setFQSO(x,false);                               // Wideband waterfall
  } else {
    m_DF=int(m_ZoomStartFreq + x*m_fSample/32768.0);  // Zoomed waterfall
    DrawOverlay();
    update();
  }
}

void CPlotter::mouseDoubleClickEvent(QMouseEvent *event)  //mouse2click
{
  int h = (m_Size.height()-60)/2;
  int x=event->x();
  int y=event->y();
  if(y < h+30) {
    m_DF=0;
    setFQSO(x,false);
    emit freezeDecode1(2);                  //### ???
  } else {
    float f = m_ZoomStartFreq + x*m_fSample/32768.0;
    m_DF=int(f);
    emit freezeDecode1(1);
    DrawOverlay();
  }
}

int CPlotter::autoZero()                                        //autoZero()
{
  m_z1=0;
  m_z2=0;
  int sum1=0;
  for(int i=0; i<256; i++) {
    sum1 += m_hist1[i];
    if(sum1 > m_Size.width()/2) {
      m_z1=i;
      break;
    }
  }
  int sum2=0;
  for(int i=0; i<256; i++) {
    sum2 += m_hist2[i];
    if(sum2 > 16384) {
      m_z2=i;
      break;
    }
  }
  double gain = pow(10.0,0.05*(m_plotGain+7));
//  double dz1 = (m_z1-38)/(5.0*gain);
  double dz2 = (m_z2-28)/(5.0*gain);
  if(m_z2 < 255) m_plotZero = int(m_plotZero + dz2 + 0.5);
  return m_plotZero;
}

void CPlotter::setNSpan(int n)                                  //setNSpan()
{
  m_nSpan=n;
}

void CPlotter::setPalette(QString palette)                      //setPalette()
{
  if(palette=="Linrad") {
    float twopi=6.2831853;
    float r,g,b,phi,x;
    for(int i=0; i<256; i++) {
      r=0.0;
      if(i>105 and i<=198) {
        phi=(twopi/4.0) * (i-105.0)/(198.0-105.0);
        r=sin(phi);
      } else if(i>=198) {
          r=1.0;
      }

      g=0.0;
      if(i>35 and i<198) {
        phi=(twopi/4.0) * (i-35.0)/(122.5-35.0);
        g=0.625*sin(phi);
      } else if(i>=198) {
        x=(i-186.0);
        g=-0.014 + 0.0144*x -0.00007*x*x +0.000002*x*x*x;
        if(g>1.0) g=1.0;
      }

      b=0.0;
      if(i<=117) {
        phi=(twopi/2.0) * i/117.0;
        b=0.4531*sin(phi);
      } else if(i>186) {
        x=(i-186.0);
        b=-0.014 + 0.0144*x -0.00007*x*x +0.000002*x*x*x;
        if(b>1.0) b=1.0;
      }
      m_ColorTbl[i].setRgb(int(255.0*r),int(255.0*g),int(255.0*b));
    }
    m_ColorTbl[255].setRgb(255,255,100);

  }

  if(palette=="CuteSDR") {
      for( int i=0; i<256; i++) {
      if( (i<43) )
        m_ColorTbl[i].setRgb( 0,0, 255*(i)/43);
      if( (i>=43) && (i<87) )
        m_ColorTbl[i].setRgb( 0, 255*(i-43)/43, 255 );
      if( (i>=87) && (i<120) )
        m_ColorTbl[i].setRgb( 0,255, 255-(255*(i-87)/32));
      if( (i>=120) && (i<154) )
        m_ColorTbl[i].setRgb( (255*(i-120)/33), 255, 0);
      if( (i>=154) && (i<217) )
        m_ColorTbl[i].setRgb( 255, 255 - (255*(i-154)/62), 0);
      if( (i>=217)  )
        m_ColorTbl[i].setRgb( 255, 0, 128*(i-217)/38);
    }
    m_ColorTbl[255].setRgb(255,255,100);
  }

  if(palette=="Blue") {
    FILE* fp=fopen("blue.dat","r");
    int n,r,g,b;
    float xr,xg,xb;
    for(int i=0; i<256; i++) {
      fscanf(fp,"%d%f%f%f",&n,&xr,&xg,&xb);
      r=255.0*xr + 0.5;
      g=255.0*xg + 0.5;
      b=255.0*xb + 0.5;
      m_ColorTbl[i].setRgb(r,g,b);
    }
  }

  if(palette=="AFMHot") {
    FILE* fp=fopen("afmhot.dat","r");
    int n,r,g,b;
    float xr,xg,xb;
    for(int i=0; i<256; i++) {
      fscanf(fp,"%d%f%f%f",&n,&xr,&xg,&xb);
      r=255.0*xr + 0.5;
      g=255.0*xg + 0.5;
      b=255.0*xb + 0.5;
      m_ColorTbl[i].setRgb(r,g,b);
    }
  }

}

void CPlotter::set2Dspec(bool b)
{
//  m_2Dspec=b;
  m_paintAllZoom=!b;
  DrawOverlay();                         //Redraw scales and ticks
  update();                              //trigger a new paintEvent}
}

double CPlotter::fGreen()
{
  return m_fGreen;
}

void CPlotter::setNsps(int n)                                  //setNSpan()
{
  m_nsps=n;
  DrawOverlay();                         //Redraw scales and ticks
  update();                              //trigger a new paintEvent}
}
