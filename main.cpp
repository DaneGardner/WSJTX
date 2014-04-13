#include <iostream>
#include <exception>
#include <stdexcept>
#include <string>

#include <locale.h>

#ifdef QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif
#include <QApplication>
#include <QRegularExpression>
#include <QObject>
#include <QSettings>
#include <QLibraryInfo>
#include <QSysInfo>
#include <QDir>
#include <QStandardPaths>

#include "revision_utils.hpp"

#include "SettingsGroup.hpp"
#include "TraceFile.hpp"
#include "mainwindow.h"

// Multiple instances:
QSharedMemory mem_jt9;
QString       my_key;

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  try
    {
      setlocale (LC_NUMERIC, "C"); // ensure number forms are in
      // consistent format, do this after
      // instantiating QApplication so
      // that GUI has correct l18n

      // Override programs executable basename as application name.
      a.setApplicationName ("WSJT-X");

      bool multiple {false};

#if WSJT_STANDARD_FILE_LOCATIONS
      // support for multiple instances running from a single installation
      auto args = a.arguments ();
      auto rig_arg_index = args.lastIndexOf (QRegularExpression {R"((-r)|(--r(i?(g?))))"});
      if (rig_arg_index > 0	// not interested if somehow the exe is called -r or --rig
          && rig_arg_index + 1 < args.size ())
        {
          auto temp_name = args.at (rig_arg_index + 1);
          if ('-' != temp_name[0]
              && !temp_name.isEmpty ())
            {
              if (temp_name.contains (QRegularExpression {R"([\\/])"}))
                {
                  throw std::runtime_error (QObject::tr ("Invalid rig name - \\ & / not allowed").toLocal8Bit ().data ());
                }
                
              a.setApplicationName (a.applicationName () + " - " + temp_name);
            }
          multiple = true;
        }
#endif

      auto config_directory = QStandardPaths::writableLocation (QStandardPaths::ConfigLocation);
      QDir config_path {config_directory}; // will be "." if config_directory is empty
      if (!config_path.mkpath ("."))
        {
          throw std::runtime_error {"Cannot find a usable configuration path \"" + config_path.path ().toStdString () + '"'};
        }
      QSettings settings(config_path.absoluteFilePath (a.applicationName () + ".ini"), QSettings::IniFormat);

      // // open a trace file
      // TraceFile trace_file {QDir {QApplication::applicationDirPath ()}.absoluteFilePath ("wsjtx_trace.log")};

      // // announce to log file
      // qDebug () << program_title (revision ()) + " - Program startup";

      // Create and initialize shared memory segment
      // Multiple instances: use rig_name as shared memory key
      my_key = a.applicationName ();
      mem_jt9.setKey(my_key);

      if(!mem_jt9.attach()) {
        if (!mem_jt9.create(sizeof(jt9com_))) {
          QMessageBox::critical (nullptr, "Error", "Unable to create shared memory segment.");
          exit(1);
        }
      }
      char *to = (char*)mem_jt9.data();
      int size=sizeof(jt9com_);
      if(jt9com_.newdat==0) {
      }
      memset(to,0,size);         //Zero all decoding params in shared memory

      unsigned downSampleFactor;
      {
        SettingsGroup {&settings, "Tune"};

        // deal with Windows Vista and earlier input audio rate
        // converter problems
        downSampleFactor = settings.value ("Audio/DisableInputResampling",
#if defined (Q_OS_WIN)
                                           // default to true for
                                           // Windows Vista and older
                                           QSysInfo::WV_VISTA >= QSysInfo::WindowsVersion ? true : false
#else
                                           false
#endif
                                           ).toBool () ? 1u : 4u;
      }

      MainWindow w(multiple, &settings, &mem_jt9, my_key, downSampleFactor);
      w.show();

      QObject::connect (&a, SIGNAL (lastWindowClosed()), &a, SLOT (quit()));
      return a.exec();
    }
  catch (std::exception const& e)
    {
      QMessageBox::critical (nullptr, QObject::tr ("Error"), e.what ());
      std::cerr << "Error: " << e.what () << '\n';
    }
  catch (...)
    {
      QMessageBox::critical (nullptr, QObject::tr ("Unexpected"), QObject::tr ("Error"));
      std::cerr << "Unexpected error\n";
      throw;			// hoping the runtime might tell us more about the exception
    }
  return -1;
}
