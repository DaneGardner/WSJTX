#ifndef LogQSO_H
#define LogQSO_H

#ifdef QT5
#include <QtWidgets>
#else
#include <QtGui>
#endif

namespace Ui {
class LogQSO;
}

class LogQSO : public QDialog
{
  Q_OBJECT

public:
  explicit LogQSO(QWidget *parent = 0);
  ~LogQSO();
  void initLogQSO(QString hisCall, QString hisGrid, QString mode,
                  QString rptSent, QString rptRcvd, QString date,
                  QString qsoStart, QString qsoStop, double dialFreq,
                  QString myCall, QString myGrid, bool noSuffix,
                  bool toRTTY, bool dBtoComments);

  double m_dialFreq;
  QString m_myCall;
  QString m_myGrid;

public slots:
  void accept();
  void reject();

signals:
  void acceptQSO(bool accepted);

private:
  Ui::LogQSO *ui;
};

#endif // LogQSO_H
