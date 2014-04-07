#include "HamlibTransceiver.hpp"

#include <cstring>

#include <QByteArray>
#include <QString>
#include <QDebug>

namespace
{
  // Unfortunately bandwidth is conflated  with mode, this is probably
  // because Icom do  the same. So we have to  care about bandwidth if
  // we want  to set  mode otherwise we  will end up  setting unwanted
  // bandwidths every time we change mode.  The best we can do via the
  // Hamlib API is to request the  normal option for the mode and hope
  // that an appropriate filter is selected.  Also ensure that mode is
  // only set is absolutely necessary.  On Icoms (and probably others)
  // the filter is  selected by number without checking  the actual BW
  // so unless the  "normal" defaults are set on the  rig we won't get
  // desirable results.
  //
  // As  an ultimate  workaround make  sure  the user  always has  the
  // option to skip mode setting altogether.

  // reroute Hamlib diagnostic messages to Qt
  int debug_callback (enum rig_debug_level_e level, rig_ptr_t /* arg */, char const * format, va_list ap)
  {
    QString message;
    message = message.vsprintf (format, ap).trimmed ();

    switch (level)
      {
      case RIG_DEBUG_BUG:
        qFatal ("%s", message.toLocal8Bit ().data ());
        break;

      case RIG_DEBUG_ERR:
        qCritical ("%s", message.toLocal8Bit ().data ());
        break;

      case RIG_DEBUG_WARN:
        qWarning ("%s", message.toLocal8Bit ().data ());
        break;

      default:
        qDebug ("%s", message.toLocal8Bit ().data ());
        break;
      }

    return 0;
  }

  // callback function that receives transceiver capabilities from the
  // hamlib libraries
  int rigCallback (rig_caps const * caps, void * callback_data)
  {
    TransceiverFactory::Transceivers * rigs = reinterpret_cast<TransceiverFactory::Transceivers *> (callback_data);

    QString key;
    if ("Hamlib" == QString::fromLatin1 (caps->mfg_name).trimmed ()
        && "Dummy" == QString::fromLatin1 (caps->model_name).trimmed ())
      {
        key = TransceiverFactory::basic_transceiver_name_;
      }
    else
      {
        key = QString::fromLatin1 (caps->mfg_name).trimmed ()
          + ' '+ QString::fromLatin1 (caps->model_name).trimmed ()
          // + ' '+ QString::fromLatin1 (caps->version).trimmed ()
          // + " (" + QString::fromLatin1 (rig_strstatus (caps->status)).trimmed () + ')'
          ;
      }

    auto port_type = TransceiverFactory::Capabilities::none;
    switch (caps->port_type)
      {
      case RIG_PORT_SERIAL:
        port_type = TransceiverFactory::Capabilities::serial;
        break;

      case RIG_PORT_NETWORK:
        port_type = TransceiverFactory::Capabilities::network;
        break;

      default: break;
      }
    (*rigs)[key] = TransceiverFactory::Capabilities (caps->rig_model
                                                     , port_type
                                                     , RIG_PTT_RIG == caps->ptt_type || RIG_PTT_RIG_MICDATA == caps->ptt_type
                                                     , RIG_PTT_RIG_MICDATA == caps->ptt_type);

    return 1;			// keep them coming
  }

  // int frequency_change_callback (RIG * /* rig */, vfo_t vfo, freq_t f, rig_ptr_t arg)
  // {
  //   (void)vfo;			// unused in release build

  //   Q_ASSERT (vfo == RIG_VFO_CURR); // G4WJS: at the time of writing only current VFO is signalled by hamlib

  //   HamlibTransceiver * transceiver (reinterpret_cast<HamlibTransceiver *> (arg));
  //   Q_EMIT transceiver->frequency_change (f, Transceiver::A);
  //   return RIG_OK;
  // }

  class hamlib_tx_vfo_fixup final
  {
  public:
    hamlib_tx_vfo_fixup (RIG * rig, vfo_t tx_vfo)
      : rig_ {rig}
    {
      original_vfo_ = rig_->state.tx_vfo;
      rig_->state.tx_vfo = tx_vfo;
    }

    ~hamlib_tx_vfo_fixup ()
    {
      rig_->state.tx_vfo = original_vfo_;
    }

  private:
    RIG * rig_;
    vfo_t original_vfo_;
  };
}

void HamlibTransceiver::register_transceivers (TransceiverFactory::Transceivers * registry)
{
  rig_set_debug_callback (debug_callback, nullptr);

#if WSJT_HAMLIB_TRACE
  rig_set_debug (RIG_DEBUG_TRACE);
#elif defined (NDEBUG)
  rig_set_debug (RIG_DEBUG_ERR);
#else
  rig_set_debug (RIG_DEBUG_VERBOSE);
#endif

  rig_load_all_backends ();
  rig_list_foreach (rigCallback, registry);
}

void HamlibTransceiver::RIGDeleter::cleanup (RIG * rig)
{
  if (rig)
    {
      // rig->state.obj = 0;
      rig_cleanup (rig);
    }
}

HamlibTransceiver::HamlibTransceiver (int model_number
                                      , QString const& cat_port
                                      , int cat_baud
                                      , TransceiverFactory::DataBits cat_data_bits
                                      , TransceiverFactory::StopBits cat_stop_bits
                                      , TransceiverFactory::Handshake cat_handshake
                                      , bool cat_dtr_always_on
                                      , bool cat_rts_always_on
                                      , TransceiverFactory::PTTMethod ptt_type
                                      , TransceiverFactory::TXAudioSource back_ptt_port
                                      , QString const& ptt_port
                                      , int poll_interval)
: PollingTransceiver {poll_interval}
, rig_ {rig_init (model_number)}
, back_ptt_port_ {TransceiverFactory::TX_audio_source_rear == back_ptt_port}
, is_dummy_ {RIG_MODEL_DUMMY == model_number}
, reversed_ {false}
{
  if (!rig_)
    {
      throw error {"Hamlib initialisation error"};
    }

  // rig_->state.obj = this;

  if (!cat_port.isEmpty ())
    {
      set_conf ("rig_pathname", cat_port.toLatin1 ().data ());
    }

  set_conf ("serial_speed", QByteArray::number (cat_baud).data ());
  set_conf ("data_bits", TransceiverFactory::seven_data_bits == cat_data_bits ? "7" : "8");
  set_conf ("stop_bits", TransceiverFactory::one_stop_bit == cat_stop_bits ? "1" : "2");

  switch (cat_handshake)
    {
    case TransceiverFactory::handshake_none: set_conf ("serial_handshake", "None"); break;
    case TransceiverFactory::handshake_XonXoff: set_conf ("serial_handshake", "XONXOFF"); break;
    case TransceiverFactory::handshake_hardware: set_conf ("serial_handshake", "Hardware"); break;
    }

  if (cat_dtr_always_on)
    {
      set_conf ("dtr_state", "ON");
    }
  if (TransceiverFactory::handshake_hardware != cat_handshake && cat_rts_always_on)
    {
      set_conf ("rts_state", "ON");
    }

  switch (ptt_type)
    {
    case TransceiverFactory::PTT_method_VOX:
      set_conf ("ptt_type", "None");
      break;

    case TransceiverFactory::PTT_method_CAT:
      set_conf ("ptt_type", "RIG");
      break;

    case TransceiverFactory::PTT_method_DTR:
    case TransceiverFactory::PTT_method_RTS:
      if (!ptt_port.isEmpty () && ptt_port != "None" && ptt_port != cat_port)
        {
#if defined (WIN32)
          set_conf ("ptt_pathname", ("\\\\.\\" + ptt_port).toLatin1 ().data ());
#else
          set_conf ("ptt_pathname", ptt_port.toLatin1 ().data ());
#endif
        }

      if (TransceiverFactory::PTT_method_DTR == ptt_type)
        {
          set_conf ("ptt_type", "DTR");
        }
      else
        {
          set_conf ("ptt_type", "RTS");
        }
    }

  // Make Icom CAT split commands less glitchy
  set_conf ("no_xchg", "1");

  // would be nice to get events but not supported on Windows and also not on a lot of rigs
  // rig_set_freq_callback (rig_.data (), &frequency_change_callback, this);
}

HamlibTransceiver::~HamlibTransceiver ()
{
}

void HamlibTransceiver::do_start ()
{
#if WSJT_TRACE_CAT
  qDebug () << "HamlibTransceiver::do_start rig:" << QString::fromLatin1 (rig_->caps->mfg_name).trimmed () + ' '
    + QString::fromLatin1 (rig_->caps->model_name).trimmed ();
#endif

  error_check (rig_open (rig_.data ()));

  init_rig ();
}

void HamlibTransceiver::do_stop ()
{
  if (rig_)
    {
      rig_close (rig_.data ());
    }

#if WSJT_TRACE_CAT
  qDebug () << "HamlibTransceiver::do_stop: state:" << state () << "reversed =" << reversed_;
#endif
}

void HamlibTransceiver::init_rig ()
{
  if (!is_dummy_)
    {
      freq_t f1;
      freq_t f2;
      rmode_t m {RIG_MODE_USB};
      rmode_t mb;
      pbwidth_t w {rig_passband_wide (rig_.data (), m)};
      pbwidth_t wb;
      if (!rig_->caps->get_vfo)
        {
          // Icom have deficient CAT protocol with no way of reading which
          // VFO is selected or if SPLIT is selected so we have to simply
          // assume it is as when we started by setting at open time right
          // here. We also gather/set other initial state.
          error_check (rig_get_freq (rig_.data (), RIG_VFO_CURR, &f1));

#if WSJT_TRACE_CAT
          qDebug () << "HamlibTransceiver::init_rig rig_get_freq =" << f1;
#endif

          error_check (rig_get_mode (rig_.data (), RIG_VFO_CURR, &m, &w));

#if WSJT_TRACE_CAT
          qDebug () << "HamlibTransceiver::init_rig rig_get_mode =" << m << "bw =" << w;
#endif

          if (!rig_->caps->set_vfo)
            {
              if (rig_has_vfo_op (rig_.data (), RIG_OP_TOGGLE))
                {

#if WSJT_TRACE_CAT
                  qDebug () << "HamlibTransceiver::init_rig rig_vfo_op TOGGLE";
#endif

                  error_check (rig_vfo_op (rig_.data (), RIG_VFO_CURR, RIG_OP_TOGGLE));
                }
              else
                {
                  throw error {"Hamlib: unable to initialise rig"};
                }
            }
          else
            {

#if WSJT_TRACE_CAT
              qDebug () << "HamlibTransceiver::init_rig rig_set_vfo";
#endif

              error_check (rig_set_vfo (rig_.data (), rig_->state.vfo_list & RIG_VFO_B ? RIG_VFO_B : RIG_VFO_SUB));
            }

          error_check (rig_get_freq (rig_.data (), RIG_VFO_CURR, &f2));

#if WSJT_TRACE_CAT
          qDebug () << "HamlibTransceiver::init_rig rig_get_freq =" << f2;
#endif

          error_check (rig_get_mode (rig_.data (), RIG_VFO_CURR, &mb, &wb));

#if WSJT_TRACE_CAT
          qDebug () << "HamlibTransceiver::init_rig rig_get_mode =" << mb << "bw =" << wb;
#endif

          update_other_frequency (f2);

          if (!rig_->caps->set_vfo)
            {

#if WSJT_TRACE_CAT
              qDebug () << "HamlibTransceiver::init_rig rig_vfo_op TOGGLE";
#endif

              error_check (rig_vfo_op (rig_.data (), RIG_VFO_CURR, RIG_OP_TOGGLE));
            }
          else
            {
#if WSJT_TRACE_CAT
              qDebug () << "HamlibTransceiver::init_rig rig_set_vfo";
#endif

              error_check (rig_set_vfo (rig_.data (), rig_->state.vfo_list & RIG_VFO_A ? RIG_VFO_A : RIG_VFO_MAIN));
            }

          if (f1 != f2 || m != mb || w != wb)	// we must have started with MAIN/A
            {
              update_rx_frequency (f1);
            }
          else
            {
              error_check (rig_get_freq (rig_.data (), RIG_VFO_CURR, &f1));

#if WSJT_TRACE_CAT
              qDebug () << "HamlibTransceiver::init_rig rig_get_freq =" << f1;
#endif

              error_check (rig_get_mode (rig_.data (), RIG_VFO_CURR, &m, &w));

#if WSJT_TRACE_CAT
              qDebug () << "HamlibTransceiver::init_rig rig_get_mode =" << m << "bw =" << w;
#endif

              update_rx_frequency (f1);
            }

#if WSJT_TRACE_CAT
          qDebug () << "HamlibTransceiver::init_rig rig_set_split_vfo";
#endif

          // error_check (rig_set_split_vfo (rig_.data (), RIG_VFO_CURR, RIG_SPLIT_OFF, RIG_VFO_CURR));
          // update_split (false);
        }
      else
        {
          vfo_t v;
          error_check (rig_get_vfo (rig_.data (), &v)); // has side effect of establishing current VFO inside hamlib

#if WSJT_TRACE_CAT
          qDebug ().nospace () << "HamlibTransceiver::init_rig rig_get_vfo = 0x" << hex << v;
#endif

          reversed_ = RIG_VFO_B == v;

          if (!(rig_->caps->targetable_vfo & (RIG_TARGETABLE_MODE | RIG_TARGETABLE_PURE)))
            {
              error_check (rig_get_mode (rig_.data (), RIG_VFO_CURR, &m, &w));

#if WSJT_TRACE_CAT
              qDebug () << "HamlibTransceiver::init_rig rig_get_mode =" << m << "bw =" << w;
#endif
            }
        }
      update_mode (map_mode (m));
    }

  poll ();

#if WSJT_TRACE_CAT
  qDebug () << "HamlibTransceiver::init_rig exit" << state () << "reversed =" << reversed_;
#endif
}

auto HamlibTransceiver::get_vfos () const -> std::tuple<vfo_t, vfo_t>
{
  if (rig_->caps->get_vfo)
    {
      vfo_t v;
      error_check (rig_get_vfo (rig_.data (), &v)); // has side effect of establishing current VFO inside hamlib

#if WSJT_TRACE_CAT
      qDebug ().nospace () << "HamlibTransceiver::get_vfos rig_get_vfo = 0x" << hex << v;
#endif

      reversed_ = RIG_VFO_B == v;
    }
  else if (rig_->caps->set_vfo)
    {
      // use VFO A/MAIN for main frequency and B/SUB for Tx
      // frequency if split since these type of radios can only
      // support this way around

#if WSJT_TRACE_CAT
      qDebug () << "HamlibTransceiver::get_vfos rig_set_vfo";
#endif

      error_check (rig_set_vfo (rig_.data (), rig_->state.vfo_list & RIG_VFO_A ? RIG_VFO_A : RIG_VFO_MAIN));
    }
  // else only toggle available but both VFOs should be substitutable 

  auto rx_vfo = rig_->state.vfo_list & RIG_VFO_A ? RIG_VFO_A : RIG_VFO_MAIN;
  auto tx_vfo = state ().split () ? (rig_->state.vfo_list & RIG_VFO_B ? RIG_VFO_B : RIG_VFO_SUB) : rx_vfo;
  if (reversed_)
    {
#if WSJT_TRACE_CAT
      qDebug () << "HamlibTransceiver::get_vfos reversing VFOs";
#endif

      std::swap (rx_vfo, tx_vfo);
    }

#if WSJT_TRACE_CAT
  qDebug ().nospace () << "HamlibTransceiver::get_vfos RX VFO = 0x" << hex << rx_vfo << " TX VFO = 0x" << hex << tx_vfo;
#endif

  return std::make_tuple (rx_vfo, tx_vfo);
}

void HamlibTransceiver::do_frequency (Frequency f)
{
#if WSJT_TRACE_CAT
  qDebug () << "HamlibTransceiver::do_frequency:" << f << "reversed:" << reversed_;
#endif

  if (!is_dummy_)
    {
      error_check (rig_set_freq (rig_.data (), RIG_VFO_CURR, f));
    }

  update_rx_frequency (f);
}

void HamlibTransceiver::do_tx_frequency (Frequency tx, bool rationalise_mode)
{
#if WSJT_TRACE_CAT
  qDebug () << "HamlibTransceiver::do_tx_frequency:" << tx << "rationalise mode:" << rationalise_mode << "reversed:" << reversed_;
#endif

  if (!is_dummy_)
    {
      auto vfos = get_vfos ();
      // auto rx_vfo = std::get<0> (vfos);
      auto tx_vfo = std::get<1> (vfos);

      if (tx)
        {
#if WSJT_TRACE_CAT
          qDebug () << "HamlibTransceiver::do_tx_frequency rig_set_split_freq";
#endif

          hamlib_tx_vfo_fixup fixup (rig_.data (), tx_vfo);
          error_check (rig_set_split_freq (rig_.data (), RIG_VFO_CURR, tx));

          if (rationalise_mode)
            {
              rmode_t current_mode;
              pbwidth_t current_width;

#if WSJT_TRACE_CAT
              qDebug () << "HamlibTransceiver::mode rig_get_split_mode";
#endif
              auto new_mode = map_mode (state ().mode ());
              error_check (rig_get_split_mode (rig_.data (), RIG_VFO_CURR, &current_mode, &current_width));

              if (new_mode != current_mode)
                {
#if WSJT_TRACE_CAT
                  qDebug () << "HamlibTransceiver::do_tx_frequency rig_set_split_mode";
#endif

                  error_check (rig_set_split_mode (rig_.data (), RIG_VFO_CURR, new_mode, rig_passband_wide (rig_.data (), new_mode)));
                }
            }
        }

      // enable split last since some rigs (Kenwood for one) come out
      // of split when you switch RX VFO (to set split mode above for
      // example)

#if WSJT_TRACE_CAT
      qDebug () << "HamlibTransceiver::do_tx_frequency rig_set_split_vfo";
#endif

      error_check (rig_set_split_vfo (rig_.data (), RIG_VFO_CURR, tx ? RIG_SPLIT_ON : RIG_SPLIT_OFF, tx_vfo));
    }

  update_split (tx);
  update_other_frequency (tx);
}

void HamlibTransceiver::do_mode (MODE mode, bool rationalise)
{
#if WSJT_TRACE_CAT
  qDebug () << "HamlibTransceiver::do_mode:" << mode << "rationalise:" << rationalise;
#endif

  if (!is_dummy_)
    {
      auto vfos = get_vfos ();
      // auto rx_vfo = std::get<0> (vfos);
      auto tx_vfo = std::get<1> (vfos);

      rmode_t current_mode;
      pbwidth_t current_width;

#if WSJT_TRACE_CAT
      qDebug () << "HamlibTransceiver::mode rig_get_mode";
#endif
      error_check (rig_get_mode (rig_.data (), RIG_VFO_CURR, &current_mode, &current_width));

      auto new_mode = map_mode (mode);
      if (new_mode != current_mode)
        {
#if WSJT_TRACE_CAT
          qDebug () << "HamlibTransceiver::mode rig_set_mode";
#endif
          error_check (rig_set_mode (rig_.data (), RIG_VFO_CURR, new_mode, rig_passband_wide (rig_.data (), new_mode)));
        }
      
      if (state ().split () && rationalise)
        {
#if WSJT_TRACE_CAT
          qDebug () << "HamlibTransceiver::mode rig_get_split_mode";
#endif
          error_check (rig_get_split_mode (rig_.data (), RIG_VFO_CURR, &current_mode, &current_width));

          if (new_mode != current_mode)
            {
#if WSJT_TRACE_CAT
              qDebug () << "HamlibTransceiver::mode rig_set_split_mode";
#endif
              hamlib_tx_vfo_fixup fixup (rig_.data (), tx_vfo);
              error_check (rig_set_split_mode (rig_.data (), RIG_VFO_CURR, new_mode, rig_passband_wide (rig_.data (), new_mode)));
            }
        }
    }

  update_mode (mode);
}

void HamlibTransceiver::poll ()
{
  if (is_dummy_)
    {
      // split with dummy is never reported since there is no rig
      if (state ().split ())
        {
          update_split (false);
        }
    }
  else
    {
#if !WSJT_TRACE_CAT_POLLS
#if defined (NDEBUG)
      rig_set_debug (RIG_DEBUG_ERR);
#else
      rig_set_debug (RIG_DEBUG_VERBOSE);
#endif
#endif

      freq_t f;
      rmode_t m;
      pbwidth_t w;
      split_t s;

      if (rig_->caps->get_vfo)
        {
          vfo_t v;
          error_check (rig_get_vfo (rig_.data (), &v)); // has side effect of establishing current VFO inside hamlib

#if WSJT_TRACE_CAT && WSJT_TRACE_CAT_POLLS
          qDebug ().nospace () << "HamlibTransceiver::state rig_get_vfo = 0x" << hex << v;
#endif

          reversed_ = RIG_VFO_B == v;
        }

      error_check (rig_get_freq (rig_.data (), RIG_VFO_CURR, &f));

#if WSJT_TRACE_CAT && WSJT_TRACE_CAT_POLLS
      qDebug () << "HamlibTransceiver::state rig_get_freq =" << f;
#endif

      update_rx_frequency (f);

      if (rig_->caps->targetable_vfo & (RIG_TARGETABLE_FREQ | RIG_TARGETABLE_PURE))
        {
          // we can only probe current VFO unless rig supports reading the other one directly
          error_check (rig_get_freq (rig_.data ()
                                     , reversed_
                                     ? (rig_->state.vfo_list & RIG_VFO_A ? RIG_VFO_A : RIG_VFO_MAIN)
                                     : (rig_->state.vfo_list & RIG_VFO_B ? RIG_VFO_B : RIG_VFO_SUB)
                                     , &f));

#if WSJT_TRACE_CAT && WSJT_TRACE_CAT_POLLS
          qDebug () << "HamlibTransceiver::state rig_get_freq other =" << f;
#endif

          update_other_frequency (f);
        }

      error_check (rig_get_mode (rig_.data (), RIG_VFO_CURR, &m, &w));

#if WSJT_TRACE_CAT && WSJT_TRACE_CAT_POLLS
      qDebug () << "HamlibTransceiver::state rig_get_mode =" << m << "bw =" << w;
#endif

      update_mode (map_mode (m));

      vfo_t v {RIG_VFO_NONE};		// so we can tell if it doesn't get updated :(
      auto rc = rig_get_split_vfo (rig_.data (), RIG_VFO_CURR, &s, &v);
      if (RIG_OK == rc && RIG_SPLIT_ON == s)
        {

#if WSJT_TRACE_CAT && WSJT_TRACE_CAT_POLLS
          qDebug ().nospace () << "HamlibTransceiver::state rig_get_split_vfo split = " << s << " VFO = 0x" << hex << v;
#endif

          update_split (true);
          // if (RIG_VFO_A == v)
          // 	{
          // 	  reversed_ = true;	// not sure if this helps us here
          // 	}
        }
      else if (RIG_OK == rc)	// not split
        {
#if WSJT_TRACE_CAT && WSJT_TRACE_CAT_POLLS
          qDebug ().nospace () << "HamlibTransceiver::state rig_get_split_vfo split = " << s << " VFO = 0x" << hex << v;
#endif

          update_split (false);
        }
      else if (-RIG_ENAVAIL == rc) // Some rigs (Icom) don't have a way of reporting SPLIT mode
        {
#if WSJT_TRACE_CAT && WSJT_TRACE_CAT_POLLS
          qDebug ().nospace () << "HamlibTransceiver::state rig_get_split_vfo can't do on this rig";
#endif

          // just report how we see it based on prior commands
        }
      else
        {
          error_check (rc);
        }

      if (RIG_PTT_NONE != rig_->state.pttport.type.ptt && rig_->caps->get_ptt)
        {
          ptt_t p;
          error_check (rig_get_ptt (rig_.data (), RIG_VFO_CURR, &p));

#if WSJT_TRACE_CAT && WSJT_TRACE_CAT_POLLS
          qDebug () << "HamlibTransceiver::state rig_get_ptt =" << p;
#endif

          update_PTT (!(RIG_PTT_OFF == p));
        }

#if !WSJT_TRACE_CAT_POLLS
#if WSJT_HAMLIB_TRACE
      rig_set_debug (RIG_DEBUG_TRACE);
#elif defined (NDEBUG)
      rig_set_debug (RIG_DEBUG_ERR);
#else
      rig_set_debug (RIG_DEBUG_VERBOSE);
#endif
#endif
    }
}

void HamlibTransceiver::do_ptt (bool on)
{
#if WSJT_TRACE_CAT
  qDebug () << "HamlibTransceiver::do_ptt:" << on << state () << "reversed =" << reversed_;
#endif

  if (on)
    {
      if (RIG_PTT_NONE != rig_->state.pttport.type.ptt)
        {
#if WSJT_TRACE_CAT
          qDebug () << "HamlibTransceiver::ptt rig_set_ptt";
#endif

          error_check (rig_set_ptt (rig_.data (), RIG_VFO_CURR, back_ptt_port_ ? RIG_PTT_ON_DATA : RIG_PTT_ON));
        }
    }
  else
    {
      if (RIG_PTT_NONE != rig_->state.pttport.type.ptt)
        {
#if WSJT_TRACE_CAT
          qDebug () << "HamlibTransceiver::ptt rig_set_ptt";
#endif

          error_check (rig_set_ptt (rig_.data (), RIG_VFO_CURR, RIG_PTT_OFF));
        }
    }

  update_PTT (on);
}

void HamlibTransceiver::error_check (int ret_code) const
{
  if (RIG_OK != ret_code)
    {
#if WSJT_TRACE_CAT && WSJT_TRACE_CAT_POLLS
      qDebug () << "HamlibTransceiver::error_check: error:" << rigerror (ret_code);
#endif

      throw error {QByteArray ("Hamlib error: ") + rigerror (ret_code)};
    }
}

void HamlibTransceiver::set_conf (char const * item, char const * value)
{
  token_t token = rig_token_lookup (rig_.data (), item);
  if (RIG_CONF_END != token)	// only set if valid for rig model
    {
      error_check (rig_set_conf (rig_.data (), token, value));
    }
}

QByteArray HamlibTransceiver::get_conf (char const * item)
{
  token_t token = rig_token_lookup (rig_.data (), item);
  QByteArray value {128, '\0'};
  if (RIG_CONF_END != token)	// only get if valid for rig model
    {
      error_check (rig_get_conf (rig_.data (), token, value.data ()));
    }
  return value;
}

auto HamlibTransceiver::map_mode (rmode_t m) const -> MODE
{
  switch (m)
    {
    case RIG_MODE_AM:
    case RIG_MODE_SAM:
    case RIG_MODE_AMS:
    case RIG_MODE_DSB:
      return AM;

    case RIG_MODE_CW:
      return CW;

    case RIG_MODE_CWR:
      return CW_R;

    case RIG_MODE_USB:
    case RIG_MODE_ECSSUSB:
    case RIG_MODE_SAH:
    case RIG_MODE_FAX:
      return USB;

    case RIG_MODE_LSB:
    case RIG_MODE_ECSSLSB:
    case RIG_MODE_SAL:
      return LSB;

    case RIG_MODE_RTTY:
      return FSK;

    case RIG_MODE_RTTYR:
      return FSK_R;

    case RIG_MODE_PKTLSB:
      return DIG_L;

    case RIG_MODE_PKTUSB:
      return DIG_U;

    case RIG_MODE_FM:
    case RIG_MODE_WFM:
      return FM;

    case RIG_MODE_PKTFM:
      return DIG_FM;

    default:
      return UNK;
    }
}

rmode_t HamlibTransceiver::map_mode (MODE mode) const
{
  switch (mode)
    {
    case AM: return RIG_MODE_AM;
    case CW: return RIG_MODE_CW;
    case CW_R: return RIG_MODE_CWR;
    case USB: return RIG_MODE_USB;
    case LSB: return RIG_MODE_LSB;
    case FSK: return RIG_MODE_RTTY;
    case FSK_R: return RIG_MODE_RTTYR;
    case DIG_L: return RIG_MODE_PKTLSB;
    case DIG_U: return RIG_MODE_PKTUSB;
    case FM: return RIG_MODE_FM;
    case DIG_FM: return RIG_MODE_PKTFM;
    default: break;
    }
  return RIG_MODE_USB;	// quieten compiler grumble
}
