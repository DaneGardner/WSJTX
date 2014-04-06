#include "EmulateSplitTransceiver.hpp"

EmulateSplitTransceiver::EmulateSplitTransceiver (std::unique_ptr<Transceiver> wrapped)
  : wrapped_ {std::move (wrapped)}
  , frequency_ {0, 0}
  , tx_ {false}
{
  // Connect update signal of wrapped Transceiver object instance to ours.
  connect (wrapped_.get (), &Transceiver::update, this, &EmulateSplitTransceiver::handle_update);

  // Connect failure signal of wrapped Transceiver object to our
  // parent failure signal.
  connect (wrapped_.get (), &Transceiver::failure, this, &Transceiver::failure);
}

void EmulateSplitTransceiver::start () noexcept
{
  wrapped_->start ();
  wrapped_->tx_frequency (0, false);
}

void EmulateSplitTransceiver::frequency (Frequency rx) noexcept
{
#if WSJT_TRACE_CAT
  qDebug () << "EmulateSplitTransceiver::frequency:" << rx;
#endif

  // Save frequency parameters.
  frequency_[0] = rx;

  // Set active frequency.
  wrapped_->frequency (rx);
}

void EmulateSplitTransceiver::tx_frequency (Frequency tx, bool /* rationalise_mode */) noexcept
{
#if WSJT_TRACE_CAT
  qDebug () << "EmulateSplitTransceiver::tx_frequency:" << tx;
#endif

  // Save frequency parameter.
  frequency_[1] = tx;

  // Set active frequency.
  wrapped_->frequency (frequency_[(tx_ && frequency_[1]) ? 1 : 0]);
}

void EmulateSplitTransceiver::ptt (bool on) noexcept
{
#if WSJT_TRACE_CAT
  qDebug () << "EmulateSplitTransceiver::ptt:" << on;
#endif

  // Save TX state for future frequency change requests.
  tx_ = on;

  // Switch to other frequency if we have one i.e. client wants split
  // operation).
  wrapped_->frequency (frequency_[(on && frequency_[1]) ? 1 : 0]);

  // Change TX state.
  wrapped_->ptt (on);
}

void EmulateSplitTransceiver::handle_update (TransceiverState state)
{
#if WSJT_TRACE_CAT
  qDebug () << "EmulateSplitTransceiver::handle_update: from wrapped:" << state;
#endif

  // Change to reflect emulated state, we don't want to report the
  // shifted frequency when transmitting.
  if (tx_)
    {
      state.frequency (frequency_[0]);
    }
  else
    {
      // Follow the rig if in RX mode.
      frequency_[0] = state.frequency ();
    }

  // Always report the other frequency as the Tx frequency we will use.
  state.tx_frequency (frequency_[1]);

  if (state.split ())
    {
      Q_EMIT failure (tr ("Emulated split mode requires rig to in simplex mode"));
    }
  else
    {
      state.split (true);       // override rig state
  
#if WSJT_TRACE_CAT
      qDebug () << "EmulateSplitTransceiver::handle_update: signalling:" << state;
#endif

      // signal emulated state
      Q_EMIT update (state);
    }
}
