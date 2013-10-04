#ifdef QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QApplication>
#include <QObject>
#include <QSettings>
#include <QSysInfo>

#include "mainwindow.h"


// Multiple instances:
QSharedMemory mem_jt9;
QUuid         my_uuid;
QString       my_key;

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);

  qRegisterMetaType<AudioDevice::Channel> ("AudioDevice::Channel");

  QSettings settings(a.applicationDirPath() + "/wsjtx.ini", QSettings::IniFormat);

  QFile f("fonts.txt");
  qint32 fontSize,fontWeight,fontSize2,fontWeight2;   // Defaults 8 50 10 50
  fontSize2=10;
  fontWeight2=50;
  if(f.open(QIODevice::ReadOnly)) {
     QTextStream in(&f);
     in >> fontSize >> fontWeight >> fontSize2 >> fontWeight2;
     f.close();
     QFont font=a.font();
     if(fontSize!=8) font.setPointSize(fontSize);
     font.setWeight(fontWeight);                       //Set the GUI fonts
     a.setFont(font);
  }

  // Create and initialize shared memory segment
  // Multiple instances: generate shared memory keys with UUID
  my_uuid = QUuid::createUuid();
  my_key = my_uuid.toString();
  mem_jt9.setKey(my_key);

  if(!mem_jt9.attach()) {
    if (!mem_jt9.create(sizeof(jt9com_))) {
      QMessageBox::critical( 0, "Error", "Unable to create shared memory segment.");
      exit(1);
    }
  }
  char *to = (char*)mem_jt9.data();
  int size=sizeof(jt9com_);
  if(jt9com_.newdat==0) {
  }
  memset(to,0,size);         //Zero all decoding params in shared memory

  settings.beginGroup ("Tune");

  // deal with Windows Vista input audio rate converter problems
  unsigned downSampleFactor = settings.value ("Audio/DisableInputResampling",
#if defined (Q_OS_WIN)
					      QSysInfo::WV_VISTA == QSysInfo::WindowsVersion ? true : false
#else
					      false
#endif
					      ).toBool () ? 1u : 4u;
  settings.endGroup ();

// Multiple instances:  Call MainWindow() with the UUID key
  MainWindow w(&settings, &mem_jt9, &my_key, fontSize2, fontWeight2, downSampleFactor);
  w.show();

  QObject::connect (&a, SIGNAL (lastWindowClosed()), &a, SLOT (quit()));
  return a.exec();
}
