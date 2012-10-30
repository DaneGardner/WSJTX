#include "widegraph.h"
#include "ui_widegraph.h"
#include "commons.h"

#define NSMAX 22000

WideGraph::WideGraph(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::WideGraph)
{
  ui->setupUi(this);
  this->setWindowFlags(Qt::Dialog);
  this->installEventFilter(parent); //Installing the filter
  ui->widePlot->setCursor(Qt::CrossCursor);
  this->setMaximumWidth(2048);
  this->setMaximumHeight(880);
  ui->widePlot->setMaximumHeight(800);
  ui->widePlot->m_bCurrent=false;

  connect(ui->widePlot, SIGNAL(freezeDecode1(int)),this,
          SLOT(wideFreezeDecode(int)));

  //Restore user's settings
  QString inifile(QApplication::applicationDirPath());
  inifile += "/wsjtx.ini";
  QSettings settings(inifile, QSettings::IniFormat);

  settings.beginGroup("WideGraph");
  ui->widePlot->setPlotZero(settings.value("PlotZero", 0).toInt());
  ui->widePlot->setPlotGain(settings.value("PlotGain", 0).toInt());
  ui->zeroSpinBox->setValue(ui->widePlot->getPlotZero());
  ui->gainSpinBox->setValue(ui->widePlot->getPlotGain());
  int n = settings.value("FreqSpan",1).toInt();
  int w = settings.value("PlotWidth",1000).toInt();
  ui->widePlot->m_w=w;
  ui->freqSpanSpinBox->setValue(n);
  ui->widePlot->setNSpan(n);
//  int nbpp = n * 32768.0/(w*96.0) + 0.5;
  ui->widePlot->setBinsPerPixel(1);
  m_waterfallAvg = settings.value("WaterfallAvg",5).toInt();
  ui->waterfallAvgSpinBox->setValue(m_waterfallAvg);
  m_dialFreq=settings.value("DialFreqMHz",474.000).toDouble();
  ui->fDialLineEdit->setText(QString::number(m_dialFreq));
  ui->widePlot->m_bCurrent=settings.value("Current",true).toBool();
  ui->widePlot->m_bCumulative=settings.value("Cumulative",false).toBool();
  ui->widePlot->m_bJT9Sync=settings.value("JT9Sync",false).toBool();
  ui->rbCurrent->setChecked(ui->widePlot->m_bCurrent);
  ui->rbCumulative->setChecked(ui->widePlot->m_bCumulative);
  ui->rbJT9Sync->setChecked(ui->widePlot->m_bJT9Sync);
  int nbpp=settings.value("BinsPerPixel",1).toInt();
  ui->widePlot->setBinsPerPixel(nbpp);
  m_qsoFreq=settings.value("QSOfreq",1010).toInt();
  ui->widePlot->setFQSO(m_qsoFreq,true);
  settings.endGroup();
}

WideGraph::~WideGraph()
{
  saveSettings();
  delete ui;
}

void WideGraph::saveSettings()
{
  //Save user's settings
  QString inifile(QApplication::applicationDirPath());
  inifile += "/wsjtx.ini";
  QSettings settings(inifile, QSettings::IniFormat);

  settings.beginGroup("WideGraph");
  settings.setValue("PlotZero",ui->widePlot->m_plotZero);
  settings.setValue("PlotGain",ui->widePlot->m_plotGain);
  settings.setValue("PlotWidth",ui->widePlot->plotWidth());
  settings.setValue("FreqSpan",ui->freqSpanSpinBox->value());
  settings.setValue("WaterfallAvg",ui->waterfallAvgSpinBox->value());
  settings.setValue("DialFreqMHz",m_dialFreq);
  settings.setValue("Current",ui->widePlot->m_bCurrent);
  settings.setValue("Cumulative",ui->widePlot->m_bCumulative);
  settings.setValue("JT9Sync",ui->widePlot->m_bJT9Sync);
  settings.setValue("BinsPerPixel",ui->widePlot->binsPerPixel());
  settings.setValue("QSOfreq",ui->widePlot->fQSO());
  settings.endGroup();
}

void WideGraph::dataSink2(float s[], float red[], float df3, int ihsym,
                          int ndiskdata, uchar lstrong[])
{
  static float splot[NSMAX];
  static float swide[2048];
  float smax;
//  double df;
  int nbpp = ui->widePlot->binsPerPixel();
  static int n=0;
  static int nkhz0=-999;

//  df = 12000.0/m_nsps;

  //Average spectra over specified number, m_waterfallAvg
  if (n==0) {
    for (int i=0; i<NSMAX; i++)
      splot[i]=s[i];
  } else {
    for (int i=0; i<NSMAX; i++)
      splot[i] += s[i];
  }
  n++;

  if (n>=m_waterfallAvg) {
    for (int i=0; i<NSMAX; i++)
        splot[i] /= n;                       //Normalize the average
    n=0;

    int w=ui->widePlot->plotWidth();
    int i0=0;                            //###
    int i=i0;
    int jz=1000.0/df3;
    for (int j=0; j<jz; j++) {
      /*
      smax=0;
      for (int k=0; k<nbpp; k++) {
        if(splot[i]>smax) smax=splot[i];
        i++;
      }
      swide[j]=smax;
      */

      float sum=0;
      for (int k=0; k<nbpp; k++) {
        i++;
        sum += splot[i];
      }
      swide[j]=sum;
//      if(lstrong[1 + i/32]!=0) swide[j]=-smax;   //Tag strong signals
    }

// Time according to this computer
    qint64 ms = QDateTime::currentMSecsSinceEpoch() % 86400000;
    int ntr = (ms/1000) % m_TRperiod;
    if((ndiskdata && ihsym <= m_waterfallAvg) || (!ndiskdata && ntr<m_ntr0)) {
      for (int i=0; i<2048; i++) {
        swide[i] = 1.e30;
      }
    }
    m_ntr0=ntr;
    ui->widePlot->draw(swide,red,i0);
  }
}

void WideGraph::on_freqSpanSpinBox_valueChanged(int n)
{
  ui->widePlot->setBinsPerPixel(n);
}

void WideGraph::on_waterfallAvgSpinBox_valueChanged(int n)
{
  m_waterfallAvg = n;
}

void WideGraph::on_zeroSpinBox_valueChanged(int value)
{
  ui->widePlot->setPlotZero(value);
}

void WideGraph::on_gainSpinBox_valueChanged(int value)
{
  ui->widePlot->setPlotGain(value);
}

void WideGraph::keyPressEvent(QKeyEvent *e)
{  
  switch(e->key())
  {
  case Qt::Key_F11:
    emit f11f12(11);
    break;
  case Qt::Key_F12:
    emit f11f12(12);
    break;
  default:
    e->ignore();
  }
}

void WideGraph::setQSOfreq(int n)
{
  m_qsoFreq=n;
  ui->widePlot->setFQSO(m_qsoFreq,true);
}

int WideGraph::QSOfreq()
{
  return ui->widePlot->fQSO();
}

int WideGraph::nSpan()
{
  return ui->widePlot->m_nSpan;
}

float WideGraph::fSpan()
{
  return ui->widePlot->m_fSpan;
}

int WideGraph::nStartFreq()
{
  return ui->widePlot->startFreq();
}

void WideGraph::wideFreezeDecode(int n)
{
  emit freezeDecode2(n);
}

void WideGraph::setTol(int n)
{
  ui->widePlot->m_tol=n;
  ui->widePlot->DrawOverlay();
  ui->widePlot->update();
}

int WideGraph::Tol()
{
  return ui->widePlot->m_tol;
}

void WideGraph::setFcal(int n)
{
  m_fCal=n;
  ui->widePlot->setFcal(n);
}

void WideGraph::on_autoZeroPushButton_clicked()
{
   int nzero=ui->widePlot->autoZero();
   ui->zeroSpinBox->setValue(nzero);
}

void WideGraph::setPalette(QString palette)
{
  ui->widePlot->setPalette(palette);
}

void WideGraph::on_fDialLineEdit_editingFinished()
{
  m_dialFreq=ui->fDialLineEdit->text().toDouble();
}

void WideGraph::initIQplus()
{
}

double WideGraph::fGreen()
{
  return ui->widePlot->fGreen();
}

void WideGraph::setPeriod(int ntrperiod, int nsps)
{
  m_TRperiod=ntrperiod;
  m_nsps=nsps;
  ui->widePlot->setNsps(nsps);
}

void WideGraph::on_rbCurrent_clicked()
{
  ui->widePlot->m_bCurrent=true;
  ui->widePlot->m_bCumulative=false;
  ui->widePlot->m_bJT9Sync=false;
}

void WideGraph::on_rbCumulative_clicked()
{
  ui->widePlot->m_bCurrent=false;
  ui->widePlot->m_bCumulative=true;
  ui->widePlot->m_bJT9Sync=false;
}

void WideGraph::on_rbJT9Sync_clicked()
{
  ui->widePlot->m_bCurrent=false;
  ui->widePlot->m_bCumulative=false;
  ui->widePlot->m_bJT9Sync=true;
}
