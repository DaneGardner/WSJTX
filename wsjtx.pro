#-------------------------------------------------
#
# Project created by QtCreator 2011-07-07T08:39:24
#
#-------------------------------------------------

QT       += network multimedia
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG   += thread
#CONFIG   += console

TARGET = wsjtx
#DESTDIR = ../qt4_install
DESTDIR = ../wsjtx_install
VERSION = 1.2
TEMPLATE = app
DEFINES = QT5
HAMLIB_DIR = ../../hamlib-1.2.15.3
QMAKE_CXXFLAGS += -std=c++11

F90 = gfortran
gfortran.output = ${QMAKE_FILE_BASE}.o
gfortran.commands = $$F90 -c -O2 -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
gfortran.input = F90_SOURCES
QMAKE_EXTRA_COMPILERS += gfortran

win32 {
DEFINES += WIN32
#F90 = g95
#g95.output = ${QMAKE_FILE_BASE}.o
#g95.commands = $$F90 -c -O2 -o ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
#g95.input = F90_SOURCES
#QMAKE_EXTRA_COMPILERS += g95
QT += axcontainer
TYPELIBS = $$system(dumpcpp -getfile {4FE359C5-A58F-459D-BE95-CA559FB4F270})
}

unix {
DEFINES += UNIX
}

#
# Order matters here as the link is in this order so referrers need to be after referred
#
SOURCES += \
	logbook/adif.cpp \
	logbook/countrydat.cpp \
	logbook/countriesworked.cpp \
	logbook/logbook.cpp \
        astro.cpp \
        NetworkServerLookup.cpp \
        Transceiver.cpp \
        TransceiverBase.cpp \
        TransceiverFactory.cpp \
        PollingTransceiver.cpp \
        EmulateSplitTransceiver.cpp \
        HRDTransceiver.cpp \
        DXLabSuiteCommanderTransceiver.cpp \
        HamlibTransceiver.cpp \
        BandData.cpp \
        Configuration.cpp \
	psk_reporter.cpp \
	Modulator.cpp \
	Detector.cpp \
	logqso.cpp \
	displaytext.cpp \
	getfile.cpp \
	soundout.cpp \
	soundin.cpp \
	meterwidget.cpp \
	signalmeter.cpp \
	plotter.cpp \
	widegraph.cpp \
	about.cpp \
	mainwindow.cpp \
	main.cpp \
        decodedtext.cpp

HEADERS  += qt_helpers.hpp \
	    pimpl_h.hpp pimpl_impl.hpp \
            NetworkServerLookup.hpp \
	    mainwindow.h plotter.h soundin.h soundout.h astro.h \
            about.h widegraph.h getfile.h \
            commons.h sleep.h displaytext.h logqso.h \
            AudioDevice.hpp Detector.hpp Modulator.hpp psk_reporter.h \
            Transceiver.hpp TransceiverBase.hpp TransceiverFactory.hpp PollingTransceiver.hpp \
            EmulateSplitTransceiver.hpp DXLabSuiteCommanderTransceiver.hpp HamlibTransceiver.hpp \
            BandData.hpp \
            Configuration.hpp \
    signalmeter.h \
    meterwidget.h \
    logbook/logbook.h \
    logbook/countrydat.h \
    logbook/countriesworked.h \
    logbook/adif.h

win32 {
SOURCES += killbyname.cpp OmniRigTransceiver.cpp
HEADERS += OmniRigTransceiver.hpp
}

FORMS    += mainwindow.ui about.ui Configuration.ui widegraph.ui astro.ui \
    logqso.ui

RC_FILE = wsjtx.rc

unix {
LIBS += -L lib -ljt9
LIBS += -lhamlib
LIBS += -lfftw3f $$system($$F90 -print-file-name=libgfortran.so)
}

win32 {
INCLUDEPATH += ${HAMLIB_DIR}/include
LIBS += -L${HAMLIB_DIR}/lib -lhamlib
#LIBS += -L${HAMLIB_DIR}/lib -lhamlib
LIBS += -L./lib -ljt9
LIBS += -L. -lfftw3f_win
LIBS += -lwsock32
LIBS += $$system($$F90 -print-file-name=libgfortran.a)
}
