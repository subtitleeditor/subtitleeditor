// subtitleeditor -- a tool to create or edit subtitle
//
// https://kitone.github.io/subtitleeditor/
// https://github.com/kitone/subtitleeditor/
//
// Copyright @ 2005-2018, kitone
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <gstreamermm/buffer.h>
#include <gstreamermm/bus.h>
#include <gstreamermm/caps.h>
#include <gstreamermm/clock.h>
#include <gstreamermm/event.h>
#include <gstreamermm/message.h>
#include <gstreamermm/query.h>
#include <gstreamermm/textoverlay.h>
#include <gstreamermm/videooverlay.h>

#include <debug.h>
#include <gst/pbutils/missing-plugins.h>
#include <gstreamer_utility.h>
#include <i18n.h>
#include <utility.h>
#include "gstplayer.h"

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif
#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif
#if defined(GDK_WINDOWING_WIN32)
#include <gdk/gdkwin32.h>
#endif
#if defined(GDK_WINDOWING_QUARTZ)
// #include <gdk/gdkquartz.h>
#endif

// Constructor
// Init values
GstPlayer::GstPlayer() {
  se_dbg(SE_DBG_VIDEO_PLAYER);

  m_xWindowId = 0;
  m_watch_id = 0;
  m_pipeline_state = Gst::STATE_NULL;
  m_pipeline_duration = Gst::CLOCK_TIME_NONE;
  m_pipeline_rate = 1.0;
  m_pipeline_async_done = false;
  m_loop_seek = cfg::get_boolean("video-player", "repeat");

  show();

  cfg::signal_changed("video-player")
      .connect(
          sigc::mem_fun(*this, &GstPlayer::on_config_video_player_changed));
}

// Destructor
// Set up pipeline to NULL.
GstPlayer::~GstPlayer() {
  se_dbg(SE_DBG_VIDEO_PLAYER);

  if (m_pipeline) {
    set_pipeline_state(Gst::STATE_NULL);
    m_pipeline.clear();
  }
}

// Create the pipeline and sets the uri.
bool GstPlayer::open(const Glib::ustring &uri) {
  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "try to open uri '%s'", uri.c_str());
  // we need to make sure the widget is realized
  realize_if_needed();

  if (!create_pipeline()) {
    se_dbg_msg(SE_DBG_VIDEO_PLAYER, "could not open uri");
    return false;
  }
  // setup the uri property and init the player state to paused
  m_pipeline->property_uri() = uri;

  bool ret = set_pipeline_state(Gst::STATE_PAUSED);

  return ret;
}

// Set up the pipeline to NULL.
void GstPlayer::close() {
  se_dbg(SE_DBG_VIDEO_PLAYER);

  set_pipeline_null();
}

// Return the uri of the current video.
Glib::ustring GstPlayer::get_uri() {
  se_dbg(SE_DBG_VIDEO_PLAYER);

  if (!m_pipeline)
    return Glib::ustring();
  return m_pipeline->property_current_uri();
}

// Sets the pipeline state to playing.
void GstPlayer::play() {
  se_dbg(SE_DBG_VIDEO_PLAYER);

  set_pipeline_state(Gst::STATE_PLAYING);
}

// Try to play the segment defined by the subtitle (from start to end).
// This function supports the looping.
// The state is sets to playing.
void GstPlayer::play_subtitle(const Subtitle &sub) {
  se_dbg(SE_DBG_VIDEO_PLAYER);

  if (!m_pipeline)
    return;
  Gst::SeekFlags flags = Gst::SEEK_FLAG_FLUSH | Gst::SEEK_FLAG_ACCURATE;
  if (m_loop_seek) {
    flags |= Gst::SEEK_FLAG_SEGMENT;
    m_subtitle_play = sub;
  }
  // if the seek success, we update the timeout and
  // swap the pipeline state to playing
  if (seek(sub.get_start().totalmsecs, sub.get_end().totalmsecs, flags)) {
    update_pipeline_state_and_timeout();
    set_pipeline_state(Gst::STATE_PLAYING);
  }
}

// Try to play the segment defined (start to end).
// This function don't support the mode looping.
// The state is sets to playing.
void GstPlayer::play_segment(const SubtitleTime &start,
                             const SubtitleTime &end) {
  se_dbg(SE_DBG_VIDEO_PLAYER);

  if (!m_pipeline)
    return;

  Gst::SeekFlags flags = Gst::SEEK_FLAG_FLUSH | Gst::SEEK_FLAG_ACCURATE;

  if (seek(start.totalmsecs, end.totalmsecs, flags))
    update_pipeline_state_and_timeout();
    set_pipeline_state(Gst::STATE_PLAYING);
}

// Sets the pipeline state to paused.
void GstPlayer::pause() {
  se_dbg(SE_DBG_VIDEO_PLAYER);

  set_pipeline_state(Gst::STATE_PAUSED);
}

// Return true if the state of the pipeline is playing.
bool GstPlayer::is_playing() {
  se_dbg(SE_DBG_VIDEO_PLAYER);

  return (m_pipeline_state == Gst::STATE_PLAYING);
}

// Return the duration of the stream or 0.
long GstPlayer::get_duration() {
//  se_dbg(SE_DBG_VIDEO_PLAYER);

  if (!m_pipeline) {
    se_dbg_msg(SE_DBG_VIDEO_PLAYER, "0 because no pipeline");
    return 0;
  }

  if (!GST_CLOCK_TIME_IS_VALID(m_pipeline_duration))
    if (!update_pipeline_duration()) {
      se_dbg_msg(SE_DBG_VIDEO_PLAYER, "0 because pipeline duration not valid");
      return 0;
     }

  long res = m_pipeline_duration / Gst::MILLI_SECOND;
//  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "%li", res);
  return res;
}

// Return the current position in the stream.
long GstPlayer::get_position() {
//  se_dbg(SE_DBG_VIDEO_PLAYER);

  if (!m_pipeline) {
    se_dbg_msg(SE_DBG_VIDEO_PLAYER, "0 because no pipeline");
    return 0;
  }

  gint64 pos = 0;
  Gst::Format fmt = Gst::FORMAT_TIME;

  if (!m_pipeline->query_position(fmt, pos)) {
    se_dbg_msg(SE_DBG_VIDEO_PLAYER, "0 because query_position() failed");
    return 0;
  }
  long res = pos / Gst::MILLI_SECOND;
  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "%li", res);
  return res;
}

bool GstPlayer::seek(long start, long end, const Gst::SeekFlags &flags) {
  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "try to seek %s (%d) - %s (%d)",
             SubtitleTime(start).str().c_str(), start,
             SubtitleTime(end).str().c_str(), end);

  if (!m_pipeline)
    return false;
  long dur = get_duration();
  // clamp
  start = CLAMP(start, 0, dur);
  end = CLAMP(end, 0, dur);
  // check the order
  if (start > end)
    std::swap(start, end);
  // convert to gstreamer time
  gint64 gstart = start * Gst::MILLI_SECOND;
  gint64 gend = end * Gst::MILLI_SECOND;

  se_dbg_msg(SE_DBG_VIDEO_PLAYER,
             "pipeline->seek(%" GST_TIME_FORMAT ", %" GST_TIME_FORMAT ")",
             GST_TIME_ARGS(gstart), GST_TIME_ARGS(gend));

  bool ret =
      m_pipeline->seek(m_pipeline_rate, Gst::FORMAT_TIME, flags,
                       Gst::SEEK_TYPE_SET, gstart, Gst::SEEK_TYPE_SET, gend);

  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "result of seek %s",
             (ret) ? "true" : "false");

  return ret;
}

// Seeking, the state of the pipeline is not modified.
void GstPlayer::seek(long position) {
  se_dbg(SE_DBG_VIDEO_PLAYER);

  if (!m_pipeline)
    return;

  if (seek(position, get_duration(),
           Gst::SEEK_FLAG_FLUSH | Gst::SEEK_FLAG_ACCURATE))
    update_pipeline_state_and_timeout();
}

// Update the text overlay with this new text.
void GstPlayer::set_subtitle_text(const Glib::ustring &text) {
  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "text='%s'", text.c_str());

  if (!m_textoverlay)
    return;

  Glib::ustring corrected = text;
  utility::replace(corrected, "&", "&amp;");

  m_textoverlay->set_property("text", corrected);
}

// Sets the new playback rate. Used for slow or fast motion.
// Default value : 1.0
// Min : 0.1
// Max : 1.5
void GstPlayer::set_playback_rate(double value) {
  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "rate=%f", value);

  value = CLAMP(value, 0.1, 1.5);  // FIXME

  m_pipeline_rate = value;

  if (seek(get_position(), get_duration(), Gst::SEEK_FLAG_FLUSH))
    update_pipeline_state_and_timeout();
}

// Return the playback rate.
double GstPlayer::get_playback_rate() {
  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "pipeline_rate=%f", m_pipeline_rate);

  return m_pipeline_rate;
}

// Enable/Disable the repeat mode.
// Works only with play_subtitle.
void GstPlayer::set_repeat(bool state) {
  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "state=%s", (state) ? "true" : "false");

  m_loop_seek = state;
  // FIXME flush pipeline ?
}

// Realize the widget and get the xWindowId.
void GstPlayer::on_realize() {
  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "try to realize...");

  Gtk::DrawingArea::on_realize();

  m_xWindowId = get_xwindow_id();

  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "try to realize... ok");
}

void GstPlayer::on_map() {
  Gtk::DrawingArea::on_map();

  set_render_rectangle();
}

void GstPlayer::on_unmap() {
  Gtk::DrawingArea::on_unmap();

  set_render_rectangle(false);
}

// Create a gstreamer pipeline (Gst::PlayBin2), initialize the
// audio and video sink with the configuration.
// Connect the bug message to the player.
bool GstPlayer::create_pipeline() {
  se_dbg(SE_DBG_VIDEO_PLAYER);

  // Clean or destroy the old pipeline
  set_pipeline_null();

  m_pipeline = Gst::PlayBin::create("pipeline");

  m_pipeline->property_audio_sink() = gen_audio_element();
  m_pipeline->property_video_sink() = gen_video_element();
  // each time the audio changed, emit the message STREAM_AUDIO_CHANGED
  m_pipeline->signal_audio_changed().connect(
      sigc::bind(sigc::mem_fun(*this, &GstPlayer::send_message),
                 Player::STREAM_AUDIO_CHANGED));

  Glib::RefPtr<Gst::Bus> bus = m_pipeline->get_bus();

  // Enable synchronous msg emission to set up video
  bus->enable_sync_message_emission();

  // Connect synchronous msg to set up xoverlay with the widget
  bus->signal_sync_message().connect(
      sigc::mem_fun(*this, &GstPlayer::on_bus_message_sync));

  m_watch_id = bus->add_watch(sigc::mem_fun(*this, &GstPlayer::on_bus_message));
  return true;
}

// Return a gstreamer audio sink from the configuration option.
Glib::RefPtr<Gst::Element> GstPlayer::gen_audio_element() {
  se_dbg(SE_DBG_VIDEO_PLAYER);

  Glib::ustring cfg_audiosink = cfg::get_string("video-player", "audio-sink");

  try {
    Glib::RefPtr<Gst::Element> sink =
        Gst::ElementFactory::create_element(cfg_audiosink, "audiosink");
    if (!sink) {
      throw std::runtime_error(
          build_message(_("Failed to create a GStreamer audio output (%s). "
                          "Please check your GStreamer installation."),
                        cfg_audiosink.c_str()));
    }
    return sink;
  } catch (std::runtime_error &ex) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
    se_dbg_msg(SE_DBG_VIDEO_PLAYER, "failed to gen_audio_element '%s'",
               ex.what());
    GST_ELEMENT_WARNING(m_pipeline->gobj(), RESOURCE, NOT_FOUND, (ex.what()),
                        (NULL));
#pragma GCC diagnostic pop
  }
  // Return an NULL ptr
  return Glib::RefPtr<Gst::Element>();
}

// Return a gstreamer video sink from the configuration option.
Glib::RefPtr<Gst::Element> GstPlayer::gen_video_element() {
  se_dbg(SE_DBG_VIDEO_PLAYER);

  Glib::ustring cfg_videosink = cfg::get_string("video-player", "video-sink");
  Glib::ustring cfg_font_desc = cfg::get_string("video-player", "font-desc");
  bool cfg_shaded_background =
      cfg::get_boolean("video-player", "shaded-background");
  bool cfg_force_aspect_ratio =
      cfg::get_boolean("video-player", "force-aspect-ratio");
  guint cfg_text_valignment = get_text_valignment_based_on_config();

  try {
    Glib::RefPtr<Gst::Element> conv, sink;

    // videoconvert
    conv = Gst::ElementFactory::create_element("videoconvert", "conv");
    if (!conv) {
      throw std::runtime_error(
          build_message(_("Failed to create a GStreamer converts video (%s). "
                          "Please check your GStreamer installation."),
                        "videoconvert"));
    }
    // textoverlay
    m_textoverlay = Gst::TextOverlay::create("overlay");
    if (!m_textoverlay) {
      throw std::runtime_error(
          build_message(_("Failed to create a GStreamer text overlay (%s). "
                          "Please check your GStreamer installation."),
                        "textoverlay"));
    }
    // videoconvert ! videoscale ! %s videosink
    sink = Gst::Parse::create_bin(
        Glib::ustring::compose("videoconvert name=videocsp ! "
                               "videoscale name=videoscale ! "
                               "%1 name=videosink",
                               cfg_videosink),
        true);
    if (!sink) {
      throw std::runtime_error(
          build_message(_("Failed to create a GStreamer sink (%s). "
                          "Please check your GStreamer installation."),
                        cfg_videosink.c_str()));
    }

    Glib::RefPtr<Gst::Bin> bin = Gst::Bin::create("videobin");

    // Add in the videobin and link
    bin->add(conv)->add(m_textoverlay)->add(sink);

    conv->link_pads("src", m_textoverlay, "video_sink");
    m_textoverlay->link_pads("src", sink, "sink");

    // Add sink pad to bin element
    Glib::RefPtr<Gst::Pad> pad = conv->get_static_pad("sink");
    bin->add_pad(Gst::GhostPad::create(pad, "sink"));

    // configure text overlay
    // m_textoverlay->set_property("halignment", 1); // "center"
    m_textoverlay->set_property("valignment", cfg_text_valignment);
    m_textoverlay->set_property("shaded_background", cfg_shaded_background);
    m_textoverlay->set_property("font_desc", cfg_font_desc);

    // Configure video output
    Glib::RefPtr<Gst::Element> videosink = bin->get_element("videosink");
    if (videosink) {
#if defined(GDK_WINDOWING_QUARTZ)
      // FIXME ?
#else
      // videosink->set_property("force-aspect-ratio", cfg_force_aspect_ratio);
#endif
    }
    return bin;
  } catch (std::runtime_error &ex) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
    se_dbg_msg(SE_DBG_VIDEO_PLAYER, "failed to gen_video_element '%s'",
               ex.what());
    GST_ELEMENT_ERROR(m_pipeline->gobj(), RESOURCE, NOT_FOUND, (ex.what()),
                      (NULL));
#pragma GCC diagnostic pop
  }
  // Return an NULL ptr
  return Glib::RefPtr<Gst::Element>();
}

// Set the state of the pipeline.
// The state change can be asynchronously.
bool GstPlayer::set_pipeline_state(Gst::State state) {
  if (m_pipeline && m_pipeline_state != state) {
    Gst::StateChangeReturn ret = m_pipeline->set_state(state);
    if (ret != Gst::STATE_CHANGE_FAILURE)
      return true;
  }
  return false;
}

// Sets the state of the pipeline to NULL.
void GstPlayer::set_pipeline_null() {
  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "set up pipeline to null...");

  if (!m_pipeline)
    return;

  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "set up pipeline to NULL");

  set_pipeline_state(Gst::STATE_NULL);

  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "remove watch id");

  m_pipeline->get_bus()->remove_watch(m_watch_id);

  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "set up all values to NULL");

  m_watch_id = 0;
  m_pipeline_state = Gst::STATE_NULL;
  m_pipeline_duration = Gst::CLOCK_TIME_NONE;
  m_pipeline_rate = 1.0;
  m_pipeline_async_done = false;

  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "clear RefPtr");

  m_pipeline.clear();
  m_textoverlay.clear();
  m_xoverlay.clear();

  set_player_state(NONE);

  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "set up pipeline to null... ok");
}

// Check if are missing plugin, if it's true display a message.
// Return true if missing.
bool GstPlayer::check_missing_plugins() {
  if (m_missing_plugins.empty())
    return false;

  gstreamer_utility::dialog_missing_plugins(m_missing_plugins);
  m_missing_plugins.clear();
  return true;
}

// Check if it's a Missing Plugin Message.
// Add the description of the missing plugin in the list.
bool GstPlayer::is_missing_plugin_message(
    const Glib::RefPtr<Gst::MessageElement> &msg) {
  se_dbg(SE_DBG_VIDEO_PLAYER);

  if (!msg)
    return false;
  GstMessage *gstmsg = GST_MESSAGE(msg->gobj());
  if (!gstmsg)
    return false;
  if (!gst_is_missing_plugin_message(gstmsg))
    return false;

  gchar *description = gst_missing_plugin_message_get_description(gstmsg);
  if (!description)
    return false;

  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "missing plugin msg '%s'", description);

  m_missing_plugins.push_back(description);
  g_free(description);
  return true;
}

// Receive synchronous message emission to set up video.
void GstPlayer::on_bus_message_sync(const Glib::RefPtr<Gst::Message> &msg) {
  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "type='%s' name='%s'",
             GST_MESSAGE_TYPE_NAME(msg->gobj()),
             GST_OBJECT_NAME(GST_MESSAGE_SRC(msg->gobj())));

  if (msg->get_message_type() == Gst::MESSAGE_NEED_CONTEXT) {
#ifdef GDK_WINDOWING_WAYLAND
    auto msgNeedContext = Glib::RefPtr<Gst::MessageNeedContext>::cast_static(msg);

    Glib::ustring context_type;

    // FIXME: https://gitlab.gnome.org/GNOME/gstreamermm/merge_requests/3
    // msgNeedContext->parse(context_type);
    const gchar *context_type_c = nullptr;
    gst_message_parse_context_type (msg->gobj(), &context_type_c);
    context_type = context_type_c;

    if (context_type == "GstWaylandDisplayHandleContextType") {
      struct wl_display *wayland_display =
          gdk_wayland_display_get_wl_display (get_display()->gobj());
      if (wayland_display) {
        auto context = Gst::Context::create(context_type, TRUE);

        GstStructure *s = gst_context_writable_structure (context->gobj());
        gst_structure_set (s, "display", G_TYPE_POINTER, wayland_display, NULL);

        Glib::RefPtr<Gst::Element>::cast_static(msg->get_source())->set_context(context);
      }
    }
#endif
  } else if (gst_is_video_overlay_prepare_window_handle_message(msg->gobj())) {
    m_xoverlay = Glib::RefPtr<Gst::Element>::cast_dynamic(msg->get_source());
    if (m_xoverlay) {
      gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(m_xoverlay->gobj()),
          m_xWindowId);

      set_render_rectangle();
    }

    // FIXME: open bug on gstreamermm 1.0
    // Get the gstreamer element source
    // Glib::RefPtr<Gst::Element> el_src =
    // Glib::RefPtr<Gst::Element>::cast_static(msg->get_source());
    // Has an XOverlay
    // Glib::RefPtr< Gst::VideoOverlay > xoverlay =
    // Glib::RefPtr<Gst::VideoOverlay>::cast_dynamic(el_src);
    // xoverlay->set_window_handle(m_xWindowId);

    // We don't need to keep sync message
    m_pipeline->get_bus()->disable_sync_message_emission();
  }
}

// Dispatch the gstreamer message.
bool GstPlayer::on_bus_message(const Glib::RefPtr<Gst::Bus> & /*bus*/,
                               const Glib::RefPtr<Gst::Message> &msg) {
  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "type='%s' name='%s'",
             GST_MESSAGE_TYPE_NAME(msg->gobj()),
             GST_OBJECT_NAME(GST_MESSAGE_SRC(msg->gobj())));

  switch (msg->get_message_type()) {
    case Gst::MESSAGE_ELEMENT:
      on_bus_message_element(
          Glib::RefPtr<Gst::MessageElement>::cast_static(msg));
      break;
    case Gst::MESSAGE_EOS:
      on_bus_message_eos(Glib::RefPtr<Gst::MessageEos>::cast_static(msg));
      break;
    case Gst::MESSAGE_ERROR:
      on_bus_message_error(Glib::RefPtr<Gst::MessageError>::cast_static(msg));
      break;
    case Gst::MESSAGE_WARNING:
      on_bus_message_warning(
          Glib::RefPtr<Gst::MessageWarning>::cast_static(msg));
      break;
    case Gst::MESSAGE_STATE_CHANGED:
      on_bus_message_state_changed(
          Glib::RefPtr<Gst::MessageStateChanged>::cast_static(msg));
      break;
    case Gst::MESSAGE_SEGMENT_DONE:
      on_bus_message_segment_done(
          Glib::RefPtr<Gst::MessageSegmentDone>::cast_static(msg));
      break;
    case Gst::MESSAGE_ASYNC_DONE:
      if (m_pipeline_async_done == false) {
        // We wait for the first async-done message, then the application
        // can ask about duration, info about the stream...
        m_pipeline_async_done = true;
        send_message(Player::STREAM_READY);
      }
      break;
    default:
      break;
  }
  return true;
}

// Check the missing plugin.
// If is missing add in the list of missing plugins.
// This list should be show later.
void GstPlayer::on_bus_message_element(
    const Glib::RefPtr<Gst::MessageElement> &msg) {
  se_dbg(SE_DBG_VIDEO_PLAYER);
  is_missing_plugin_message(msg);
}

// An error is detected.
// Destroy the pipeline and show the error message in a dialog.
void GstPlayer::on_bus_message_error(
    const Glib::RefPtr<Gst::MessageError> &msg) {
  se_dbg(SE_DBG_VIDEO_PLAYER);

  check_missing_plugins();

  Glib::Error err;
  std::string err_dbg;
  msg->parse(err, err_dbg);

  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "GST_MESSAGE_ERROR : %s [%s]",
             err.what().c_str(), err_dbg.c_str());

  dialog_error(build_message(_("Media file could not be played.\n%s"),
                             get_uri().c_str()),
               err.what().c_str());

  set_pipeline_null();
}

// An warning message is detected.
void GstPlayer::on_bus_message_warning(
    const Glib::RefPtr<Gst::MessageWarning> &msg) {
  se_dbg(SE_DBG_VIDEO_PLAYER);

  check_missing_plugins();

  Glib::Error err;
  std::string err_dbg;
  msg->parse(err, err_dbg);

  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "GST_MESSAGE_WARNING : %s [%s]",
             err.what().c_str(), err_dbg.c_str());

  g_warning("%s [%s]", err.what().c_str(), err_dbg.c_str());
}

// The state of the pipeline has changed.
// Update the player state.
void GstPlayer::on_bus_message_state_changed(
    const Glib::RefPtr<Gst::MessageStateChanged> &msg) {
  se_dbg(SE_DBG_VIDEO_PLAYER);

  // We only update when it's the pipeline object
  if (msg->get_source()->get_name() != "pipeline")
    return;

  Gst::State old_state, new_state, pending;

  msg->parse(old_state, new_state, pending);

  // Update the current state of the pipeline
  m_pipeline_state = new_state;

  if (old_state == Gst::STATE_NULL && new_state == Gst::STATE_READY) {
    set_player_state(NONE);
  } else if (old_state == Gst::STATE_READY && new_state == Gst::STATE_PAUSED) {
    set_player_state(PAUSED);

    check_missing_plugins();
  } else if (old_state == Gst::STATE_PAUSED &&
             new_state == Gst::STATE_PLAYING) {
    set_player_state(PLAYING);
  } else if (old_state == Gst::STATE_PLAYING &&
             new_state == Gst::STATE_PAUSED) {
    set_player_state(PAUSED);
  } else if (old_state == Gst::STATE_PAUSED && new_state == Gst::STATE_READY) {
    set_player_state(NONE);
  } else if (old_state == Gst::STATE_READY && new_state == Gst::STATE_NULL) {
    set_player_state(NONE);
  }
}

// End-of-stream (segment or stream) has been detected,
// update the pipeline state to PAUSED.
// Seek to the beginning if it's the end of the stream.
void GstPlayer::on_bus_message_eos(
    const Glib::RefPtr<Gst::MessageEos> & /*msg*/) {
  se_dbg(SE_DBG_VIDEO_PLAYER);

  // FIXME with seek_loop
  set_pipeline_state(Gst::STATE_PAUSED);

  if (get_position() == get_duration())
    seek(0);
}

// The pipeline completed playback of a segment.
// If the looping is activated send new seek event.
// Works only with play_subtitle.
void GstPlayer::on_bus_message_segment_done(
    const Glib::RefPtr<Gst::MessageSegmentDone> & /*msg*/) {
  se_dbg(SE_DBG_VIDEO_PLAYER);

  if (!(m_loop_seek && m_subtitle_play))
    return;

  // TODO debug information of MessageSegmentDone

  seek(m_subtitle_play.get_start().totalmsecs,
       m_subtitle_play.get_end().totalmsecs,
       Gst::SEEK_FLAG_ACCURATE | Gst::SEEK_FLAG_SEGMENT);
}

// The video-player configuration has changed, update the player.
void GstPlayer::on_config_video_player_changed(const Glib::ustring &key,
                                               const Glib::ustring &value) {
  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "%s %s", key.c_str(), value.c_str());

  if (key == "repeat") {
    set_repeat(utility::string_to_bool(value));
  } else if (m_pipeline) {
    if (key == "force-aspect-ratio" && m_xoverlay) {
#if defined(GDK_WINDOWING_QUARTZ)
      // FIXME ?
#else
      // m_xoverlay->set_property("force-aspect-ratio",
      // utility::string_to_bool(value));
#endif
      // g_object_set(G_OBJECT(m_xoverlay->gobj()), "force-aspect-ratio",
      // utility::string_to_bool(value), NULL);
      queue_draw();
    } else if (key == "shaded-background" && m_textoverlay) {
      m_textoverlay->set_property("shaded_background",
                                  utility::string_to_bool(value));
    } else if (key == "font-desc" && m_textoverlay) {
      m_textoverlay->set_property("font_desc", value);
    }
  }
}

// Return the xwindow ID. (Support X11, Wayland, WIN32 and QUARTZ)
// Do not call this function in a gstreamer thread, this cause crash/segfault.
// Caused by the merge of the Client-Side Windows in GTK+.
gulong GstPlayer::get_xwindow_id() {
  se_dbg(SE_DBG_VIDEO_PLAYER);

  gulong xWindowId = 0;
  auto gdkWindow = get_window()->gobj();

#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_WINDOW(gdkWindow)) {
    xWindowId = GDK_WINDOW_XID(gdkWindow);
  }
  else
#endif
#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_WINDOW(gdkWindow)) {
    xWindowId = (gulong)gdk_wayland_window_get_wl_surface(gdkWindow);
  }
  else
#endif
#ifdef GDK_WINDOWING_WIN32
  if (GDK_IS_WIN32_WINDOW(gdkWindow)) {
    xWindowId = gdk_win32_drawable_get_handle(get_window()->gobj());
  }
  else
#endif
#ifdef GDK_WINDOWING_QUARTZ
  if (GDK_IS_QUARTZ_WINDOW(gdkWindow)) {
    xWindowId = 0;
    // gdk_quartz_window_get_nswindow(get_window()->gobj());
    // gdk_quartz_window_get_nsview(get_window()->gobj());
  }
#endif

  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "xWindowId=%d", xWindowId);

  return xWindowId;
}

void GstPlayer::update_pipeline_state_and_timeout() {
  se_dbg(SE_DBG_VIDEO_PLAYER);

  if (!m_pipeline)
    return;
  Gst::State old_st, new_st;
  m_pipeline->get_state(old_st, new_st, 100 * Gst::MILLI_SECOND);
  got_tick();
}

// Set up the duration value of the stream if need.
bool GstPlayer::update_pipeline_duration() {
  se_dbg(SE_DBG_VIDEO_PLAYER);

  if (!m_pipeline)
    return false;

  m_pipeline_duration = Gst::CLOCK_TIME_NONE;

  gint64 dur = -1;
  Gst::Format fmt = Gst::FORMAT_TIME;
  if (m_pipeline->query_duration(fmt, dur) && dur != -1) {
    m_pipeline_duration = dur;
    se_dbg_msg(SE_DBG_VIDEO_PLAYER,
               "Success to query the duration (%" GST_TIME_FORMAT ")",
               GST_TIME_ARGS(dur));
    // send_message(STREAM_DURATION_CHANGED);
    return true;
  }
  se_dbg_msg(SE_DBG_VIDEO_PLAYER,
             "The query of the duration of the stream failed");
  return false;
}

// Return the number of audio track.
gint GstPlayer::get_n_audio() {
  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "n_audio: %d",
             (m_pipeline) ? m_pipeline->property_n_audio() : 0);

  if (m_pipeline)
    return m_pipeline->property_n_audio();
  return 0;
}

// Sets the current audio track. (-1 = auto)
void GstPlayer::set_current_audio(gint track) {
  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "track=%d", track);

  if (!m_pipeline)
    return;

  if (track < -1)
    track = -1;
  m_pipeline->property_current_audio() = track;
  send_message(Player::STREAM_AUDIO_CHANGED);
}

// Return the current audio track.
gint GstPlayer::get_current_audio() {
  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "current_audio: %d",
             (m_pipeline) ? m_pipeline->property_current_audio() : 0);

  if (m_pipeline)
    return m_pipeline->property_current_audio();
  return -1;
}

// Return the framerate of the video or zero (0).
// Update numerator and denominator if the values are not null.
float GstPlayer::get_framerate(int *numerator, int *denominator) {
  se_dbg(SE_DBG_VIDEO_PLAYER);

  if (!m_pipeline)
    return 0;
  Glib::RefPtr<Gst::Pad> pad = m_pipeline->get_video_pad(0);
  g_return_val_if_fail(pad, 0);

  Glib::RefPtr<Gst::Caps> caps = pad->get_current_caps();
  g_return_val_if_fail(caps, 0);

  const Gst::Structure structure = caps->get_structure(0);
  if (structure.has_field("framerate") == false) {
    se_dbg_msg(SE_DBG_VIDEO_PLAYER, "structure has not field \"framerate\"");
    return 0;
  }

  Glib::ValueBase gst_value;
  structure.get_field("framerate", gst_value);

  Gst::Fraction fps(gst_value);
  float framerate = static_cast<float>(fps.num) / static_cast<float>(fps.denom);

  if (numerator != NULL)
    *numerator = fps.num;
  if (denominator != NULL)
    *denominator = fps.denom;

  se_dbg_msg(SE_DBG_VIDEO_PLAYER, "framerate: %f (num: %i, denom: %i)",
             framerate, fps.num, fps.denom);

  return framerate;
}

guint GstPlayer::get_text_valignment_based_on_config() {
  guint alignment = 0;  // baseline by default

  if (!cfg::has_key("video-player", "text-valignment")) {
    cfg::set_string("video-player", "text-valignment", "baseline");
    return alignment;
  }
  auto valignment = cfg::get_string("video-player", "text-valignment");

  if (valignment == "baseline") {
    alignment = 0;
  } else if (valignment == "bottom") {
    alignment = 1;
  } else if (valignment == "top") {
    alignment = 2;
  } else if (valignment == "position") {
    alignment = 3;
  } else if (valignment == "center") {
    alignment = 4;
  }
  return alignment;
}

void GstPlayer::on_hierarchy_changed(Widget* previous_toplevel) {
  if (!previous_toplevel) {
    get_toplevel()->signal_configure_event().connect(
      sigc::mem_fun(*this, &GstPlayer::on_configure_event), false);
  }
}

bool GstPlayer::on_configure_event(GdkEventConfigure*) {
  set_render_rectangle();
  return false;
}

void GstPlayer::set_render_rectangle(bool is_mapped) {
#ifdef GDK_WINDOWING_WAYLAND
  if (!m_xoverlay) {
    return;
  }

  auto gdkWindow = get_window();

  if (GDK_IS_WAYLAND_WINDOW(gdkWindow->gobj())) {
    int x, y, width, height;

    if (is_mapped) {
      width = gdkWindow->get_width();
      height = gdkWindow->get_height();
    } else {
      width = height = 1;
    }

    translate_coordinates(*get_toplevel(), 0, 0, x, y);
    gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(m_xoverlay->gobj()),
        x, y, width, height);
    gst_video_overlay_expose(GST_VIDEO_OVERLAY(m_xoverlay->gobj()));
  }
#endif
}
