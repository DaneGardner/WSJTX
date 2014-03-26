#ifndef DX_LAB_SUITE_COMMANDER_TRANSCEIVER_HPP__
#define DX_LAB_SUITE_COMMANDER_TRANSCEIVER_HPP__

#include <memory>

#include <QByteArray>

#include "TransceiverFactory.hpp"
#include "PollingTransceiver.hpp"

class QTcpSocket;

//
// DX Lab Suite Commander Interface
//
// Implemented as a Transceiver decorator  because we may want the PTT
// services of another Transceiver  type such as the HamlibTransceiver
// which can  be enabled by wrapping  a HamlibTransceiver instantiated
// as a "Hamlib Dummy" transceiver in the Transceiver factory method.
//
class DXLabSuiteCommanderTransceiver final
  : public PollingTransceiver
{
public:
  static void register_transceivers (TransceiverFactory::Transceivers *, int id);

  // takes ownership of wrapped Transceiver
  explicit DXLabSuiteCommanderTransceiver (std::unique_ptr<TransceiverBase> wrapped, QString const& address, bool use_for_ptt, int poll_interval);
  ~DXLabSuiteCommanderTransceiver ();

private:
  void do_start () override;
  void do_stop () override;
  void do_frequency (Frequency) override;
  void do_tx_frequency (Frequency, bool rationalise_mode) override;
  void do_mode (MODE, bool rationalise) override;
  void do_ptt (bool on) override;

  void poll () override;

  void send_command (QByteArray const&, bool no_debug = false);
  QByteArray read_reply (bool no_debug = false);

  std::unique_ptr<TransceiverBase> wrapped_;
  bool use_for_ptt_;
  QString server_;
  QTcpSocket * commander_;
};

#endif
