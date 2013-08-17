#ifndef SOUNDIN_H__
#define SOUNDIN_H__

#include <QObject>
#include <QString>
#include <QScopedPointer>
#include <QAudioInput>

class QAudioDeviceInfo;
class QAudioInput;
class QIODevice;

// Gets audio data from sound sample source and passes it to a sink device
class SoundInput : public QObject
{
  Q_OBJECT;

 private:
  Q_DISABLE_COPY (SoundInput);

 public:
  SoundInput (QObject * parent = 0)
    : QObject (parent)
  {
  }

  ~SoundInput ();

 private:
  Q_SIGNAL void error (QString message) const;
  Q_SIGNAL void status (QString message) const;

  // sink must exist from the start call to any following stop () call
  Q_SLOT void start(QAudioDeviceInfo const&, unsigned channels, int framesPerBuffer, QIODevice * sink);
  Q_SLOT void stop();

  // used internally
  Q_SLOT void handleStateChanged (QAudio::State) const;

  bool audioError () const;

  QScopedPointer<QAudioInput> m_stream;
};

#endif
