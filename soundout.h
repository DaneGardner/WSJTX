#ifndef SOUNDOUT_H__
#define SOUNDOUT_H__

#include <QObject>
#include <QString>
#include <QAudioOutput>
#include <QAudioDeviceInfo>

class QAudioDeviceInfo;

class QAudioDeviceInfo;

// An instance of this sends audio data to a specified soundcard.

class SoundOutput : public QObject
{
  Q_OBJECT;

  Q_PROPERTY(bool running READ isRunning);
  Q_PROPERTY(unsigned attenuation READ attenuation WRITE setAttenuation RESET resetAttenuation);

 private:
  Q_DISABLE_COPY (SoundOutput);

 public:
  SoundOutput (QIODevice * source);
  ~SoundOutput ();

  bool isRunning() const {return m_active;}
  qreal attenuation () const;

 private Q_SLOTS:
  /* private because we expect to run in a thread and don't want direct
     C++ calls made, instead they must be invoked via the Qt
     signal/slot mechanism which is thread safe */
  void startStream (QAudioDeviceInfo const& device, unsigned channels);
  void suspend ();
  void resume ();
  void stopStream ();
  void setAttenuation (qreal);	/* unsigned */
  void resetAttenuation ();	/* to zero */

 Q_SIGNALS:
  void error (QString message) const;
  void status (QString message) const;

private:
  bool audioError () const;

 private Q_SLOTS:
  void handleStateChanged (QAudio::State);

 private:
  QScopedPointer<QAudioOutput> m_stream;

  QIODevice * m_source;
  bool volatile m_active;
  QAudioDeviceInfo m_currentDevice;
  qreal m_volume;
};

#endif
