// subtitleeditor -- a tool to create or edit subtitle
//
// https://subtitleeditor.github.io/subtitleeditor/
// https://github.com/subtitleeditor/subtitleeditor/
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

#include "gstplayer.h"

#include <debug.h>
#include <gst/pbutils/missing-plugins.h>
#include <gst/video/video.h>
#include <gstreamer_utility.h>
#include <i18n.h>
#include <utility.h>

static gboolean vp_handle_message(GstBus* bus, GstMessage* msg, void* data) {
   GstPlayer* p = static_cast<GstPlayer*>(data);
   return p->on_bus_message(bus, msg);
}

// Constructor
// Init values
GstPlayer::GstPlayer() {
   se_dbg(SE_DBG_VIDEO_PLAYER);

   m_pipeline = nullptr;
   m_gtksink_widget = nullptr;
   m_textoverlay = nullptr;
   m_watch_id = 0;
   m_pipeline_state = GST_STATE_NULL;
   m_pipeline_duration = GST_CLOCK_TIME_NONE;
   m_pipeline_rate = 1.0;
   m_pipeline_async_done = false;
   m_loop_seek = cfg::get_boolean("video-player", "repeat");

   show();

   cfg::signal_changed("video-player").connect(sigc::mem_fun(*this, &GstPlayer::on_config_video_player_changed));
}

// Destructor
// Set up pipeline to NULL.
GstPlayer::~GstPlayer() {
   se_dbg(SE_DBG_VIDEO_PLAYER);

   if (m_pipeline) {
      set_pipeline_state(GST_STATE_NULL);
   }
}

// Create the pipeline and sets the uri.
bool GstPlayer::open(const Glib::ustring& uri) {
   se_dbg_msg(SE_DBG_VIDEO_PLAYER, "try to open uri '%s'", uri.c_str());
   // we need to make sure the widget is realized
   realize_if_needed();

   if (!create_pipeline()) {
      se_dbg_msg(SE_DBG_VIDEO_PLAYER, "could not open uri");
      return false;
   }
   // setup the uri property and init the player state to paused
   g_object_set(m_pipeline, "uri", uri.c_str(), NULL);
   m_uri = uri;

   return set_pipeline_state(GST_STATE_PAUSED);
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
   return m_uri;
}

// Sets the pipeline state to playing.
void GstPlayer::play() {
   se_dbg(SE_DBG_VIDEO_PLAYER);

   set_pipeline_state(GST_STATE_PLAYING);
}

// Try to play the segment defined by the subtitle (from start to end).
// This function supports the looping.
// The state is sets to playing.
void GstPlayer::play_subtitle(const Subtitle& sub) {
   se_dbg(SE_DBG_VIDEO_PLAYER);

   if (!m_pipeline)
      return;
   GstSeekFlags flags = (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE);
   if (m_loop_seek) {
      flags = (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE | GST_SEEK_FLAG_SEGMENT);
      m_subtitle_play = sub;
   }
   // if the seek success, we update the timeout and
   // swap the pipeline state to playing
   if (seek(sub.get_start().totalmsecs, sub.get_end().totalmsecs, flags)) {
      update_pipeline_state_and_timeout();
      set_pipeline_state(GST_STATE_PLAYING);
   }
}

// Try to play the segment defined (start to end).
// This function don't support the mode looping.
// The state is sets to playing.
void GstPlayer::play_segment(const SubtitleTime& start, const SubtitleTime& end) {
   se_dbg(SE_DBG_VIDEO_PLAYER);

   if (!m_pipeline)
      return;

   GstSeekFlags flags = (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE);

   if (seek(start.totalmsecs, end.totalmsecs, flags))
      update_pipeline_state_and_timeout();
   set_pipeline_state(GST_STATE_PLAYING);
}

// Sets the pipeline state to paused.
void GstPlayer::pause() {
   se_dbg(SE_DBG_VIDEO_PLAYER);

   set_pipeline_state(GST_STATE_PAUSED);
}

// Return true if the state of the pipeline is playing.
bool GstPlayer::is_playing() {
   // se_dbg(SE_DBG_VIDEO_PLAYER);

   return (m_pipeline_state == GST_STATE_PLAYING);
}

// Return the duration of the stream or 0.
long GstPlayer::get_duration() {
   // se_dbg(SE_DBG_VIDEO_PLAYER);

   if (!m_pipeline) {
      return 0;
   }
   if (!GST_CLOCK_TIME_IS_VALID(m_pipeline_duration)) {
      if (!update_pipeline_duration()) {
         return 0;
      }
   }
   return GST_TIME_AS_MSECONDS(m_pipeline_duration);
}

// Return the current position in the stream.
long GstPlayer::get_position() {
   // se_dbg(SE_DBG_VIDEO_PLAYER);

   if (!m_pipeline) {
      return 0;
   }

   gint64 pos = 0;
   if (!gst_element_query_position(m_pipeline, GST_FORMAT_TIME, &pos))
      return 0;
   return GST_TIME_AS_MSECONDS(pos);
}

bool GstPlayer::seek(long start, long end, const GstSeekFlags& flags) {
   se_dbg_msg(SE_DBG_VIDEO_PLAYER,
              "try to seek %s (%d) - %s (%d) (%d)",
              SubtitleTime(start).str().c_str(),
              start,
              SubtitleTime(end).str().c_str(),
              end,
              m_pipeline_rate);

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
   gint64 gstart = start * GST_MSECOND;
   gint64 gend = end * GST_MSECOND;

   se_dbg_msg(SE_DBG_VIDEO_PLAYER, "pipeline->seek(%" GST_TIME_FORMAT ", %" GST_TIME_FORMAT ")", GST_TIME_ARGS(gstart), GST_TIME_ARGS(gend));

   // out of range ?
   if (start == end) {
      return false;
   }

   bool ret = gst_element_seek(m_pipeline, m_pipeline_rate, GST_FORMAT_TIME, flags, GST_SEEK_TYPE_SET, gstart, GST_SEEK_TYPE_SET, gend);

   se_dbg_msg(SE_DBG_VIDEO_PLAYER, "result of seek %s", (ret) ? "true" : "false");

   return ret;
}

// Seeking, the state of the pipeline is not modified.
void GstPlayer::seek(long position) {
   se_dbg(SE_DBG_VIDEO_PLAYER);

   if (!m_pipeline)
      return;

   GstSeekFlags flags = (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE);
   if (seek(position, get_duration(), flags))
      update_pipeline_state_and_timeout();
}

// Update the text overlay with this new text.
void GstPlayer::set_subtitle_text(const Glib::ustring& text) {
   // se_dbg_msg(SE_DBG_VIDEO_PLAYER, "text='%s'", text.c_str());

   if (!m_textoverlay)
      return;

   Glib::ustring corrected = text;
   utility::replace(corrected, "&", "&amp;");
   g_object_set(G_OBJECT(m_textoverlay), "text", corrected.c_str(), NULL);
}

// Sets the new playback rate. Used for slow or fast motion.
// Default value : 1.0
// Min : 0.1
// Max : 1.5
void GstPlayer::set_playback_rate(double value) {
   se_dbg_msg(SE_DBG_VIDEO_PLAYER, "rate=%f", value);

   value = CLAMP(value, 0.1, 1.5);  // FIXME

   m_pipeline_rate = value;

   if (seek(get_position(), get_duration(), GST_SEEK_FLAG_FLUSH))
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

// Create a gstreamer pipeline (Gst::PlayBin2), initialize the
// audio and video sink with the configuration.
// Connect the bug message to the player.
bool GstPlayer::create_pipeline() {
   se_dbg(SE_DBG_VIDEO_PLAYER);

   // Clean or destroy the old pipeline
   set_pipeline_null();

   m_pipeline = gst_element_factory_make("playbin3", NULL);

   GstElement* videosink = gen_video_element();
   g_object_set(GST_OBJECT(m_pipeline), "video-sink", videosink, NULL);
   if (videosink) {
      // release our temporary reference to the element
      gst_object_unref(videosink);
   }

   GstElement* audiosink = gen_audio_element();
   g_object_set(GST_OBJECT(m_pipeline), "audio-sink", audiosink, NULL);
   if (audiosink) {
      // release our temporary reference to the element
      gst_object_unref(audiosink);
   }

   // Use scaletempo to apply playback rates without the chipmunk effect
   GstElement* scaletempo = gst_element_factory_make("scaletempo", "scaletempo");
   if (scaletempo) {
      g_object_set(GST_OBJECT(m_pipeline), "audio-filter", scaletempo, NULL);
   }

   show_all();

   // Add a bus watch, so we get notified when a message arrives
   GstBus* bus = gst_element_get_bus(m_pipeline);
   m_watch_id = gst_bus_add_watch(bus, (GstBusFunc)vp_handle_message, this);
   gst_object_unref(bus);
   return true;
}

// Return a gstreamer audio sink from the configuration option.
GstElement* GstPlayer::gen_audio_element() {
   se_dbg(SE_DBG_VIDEO_PLAYER);

   // FIXME: we should remove cfg "audio-sink" ?
   Glib::ustring cfg_audiosink = cfg::get_string("video-player", "audio-sink");

   // Try configured sink first
   GstElement* sink = gst_element_factory_make(cfg_audiosink.c_str(), "audiosink");
   if (!sink) {
      // Warn and attempt a sensible fallback
      const char* fmt = _("Failed to create a GStreamer audio output (%s). Please check your GStreamer installation.");
      gchar* msg = g_strdup_printf(fmt, cfg_audiosink.c_str());
      se_dbg_msg(SE_DBG_VIDEO_PLAYER, "%s", msg);
      if (m_pipeline) {
         GST_ELEMENT_WARNING(m_pipeline, RESOURCE, NOT_FOUND, ("%s", msg), (NULL));
      }
      g_free(msg);

      // Fallback to autoaudiosink
      sink = gst_element_factory_make("autoaudiosink", "audiosink");
   }

   return sink;  // may be nullptr if fallback also failed
}

// Return a gstreamer video sink from the configuration option.
GstElement* GstPlayer::gen_video_element() {
   se_dbg(SE_DBG_VIDEO_PLAYER);

   Glib::ustring cfg_font_desc = cfg::get_string("video-player", "font-desc");
   bool cfg_shaded_background = cfg::get_boolean("video-player", "shaded-background");
   bool cfg_force_aspect_ratio = cfg::get_boolean("video-player", "force-aspect-ratio");
   guint cfg_text_valignment = get_text_valignment_based_on_config();

   GError* error = nullptr;
   GstElement* videobin = gst_parse_bin_from_description("textoverlay name=textoverlay ! videoconvert ! gtksink name=gtksink", true, &error);
   if (error) {
      g_printerr("Error trying generate video element: %s\n", error->message);
      g_clear_error(&error);
      return nullptr;
   }

   // get gtksink widget
   GstElement* gtksink = gst_bin_get_by_name(GST_BIN(videobin), "gtksink");
   g_object_set(GST_OBJECT(gtksink), "force-aspect-ratio", cfg_force_aspect_ratio, NULL);

   g_object_get(gtksink, "widget", &m_gtksink_widget, NULL);
   gtk_container_add(GTK_CONTAINER(gobj()), m_gtksink_widget);
   g_object_unref(m_gtksink_widget);
   gtk_widget_realize(m_gtksink_widget);
   // release our temporary reference to the element
   if (gtksink) {
      // release our temporary reference to the element
      gst_object_unref(gtksink);
   }

   // configure text overlay
   // Get a pointer to the element owned by the bin; we drop our ref so the
   // bin remains the sole owner. Do not unref this pointer during teardown.
   m_textoverlay = gst_bin_get_by_name(GST_BIN(videobin), "textoverlay");
   g_object_set(GST_OBJECT(m_textoverlay), "valignment", cfg_text_valignment, NULL);
   g_object_set(GST_OBJECT(m_textoverlay), "shaded-background", cfg_shaded_background, NULL);
   g_object_set(GST_OBJECT(m_textoverlay), "font-desc", cfg_font_desc.c_str(), NULL);
   // Release the reference from gst_bin_get_by_name; keep only a borrowed pointer
   gst_object_unref(m_textoverlay);

   return videobin;
}

// Set the state of the pipeline.
// The state change can be asynchronously.
bool GstPlayer::set_pipeline_state(GstState state) {
   if (!m_pipeline) {
      return false;
   }
   if (m_pipeline_state == state) {
      return false;
   }
   GstStateChangeReturn ret = gst_element_set_state(m_pipeline, state);
   return (ret != GST_STATE_CHANGE_FAILURE);
}

// Sets the state of the pipeline to NULL.
void GstPlayer::set_pipeline_null() {
   se_dbg_msg(SE_DBG_VIDEO_PLAYER, "set up pipeline to null...");

   if (!m_pipeline)
      return;

   se_dbg_msg(SE_DBG_VIDEO_PLAYER, "stop receiving bus messages before changing state to NULL...");
   // Stop receiving bus messages before changing state to NULL.
   if (m_watch_id) {
      g_source_remove(m_watch_id);
      m_watch_id = 0;
   }

   se_dbg_msg(SE_DBG_VIDEO_PLAYER, "remove gtksink from parent");
   // Remove the gtksink widget from its parent; don't destroy it directly
   // since it's owned by the sink which we are tearing down.
   if (m_gtksink_widget) {
      GtkWidget* parent = gtk_widget_get_parent(m_gtksink_widget);
      if (parent && GTK_IS_CONTAINER(parent)) {
         gtk_container_remove(GTK_CONTAINER(parent), m_gtksink_widget);
      }
      m_gtksink_widget = nullptr;
   }

   se_dbg_msg(SE_DBG_VIDEO_PLAYER, "release m_pipeline");
   if (m_pipeline) {
      se_dbg_msg(SE_DBG_VIDEO_PLAYER, "set up pipeline status to GST_STATE_NULL...");
      gst_element_set_state(m_pipeline, GST_STATE_NULL);
      // g_object_unref(m_pipeline);
      m_pipeline = nullptr;
      m_textoverlay = nullptr;
   }

   se_dbg_msg(SE_DBG_VIDEO_PLAYER, "set up all values to NULL");

   m_pipeline_state = GST_STATE_NULL;
   m_pipeline_duration = GST_CLOCK_TIME_NONE;
   m_pipeline_rate = 1.0;
   m_pipeline_async_done = false;

   se_dbg_msg(SE_DBG_VIDEO_PLAYER, "clear RefPtr");

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
bool GstPlayer::is_missing_plugin_message(GstMessage* msg) {
   se_dbg(SE_DBG_VIDEO_PLAYER);

   if (!msg)
      return false;

   if (!gst_is_missing_plugin_message(msg))
      return false;

   gchar* description = gst_missing_plugin_message_get_description(msg);
   if (!description)
      return false;

   se_dbg_msg(SE_DBG_VIDEO_PLAYER, "missing plugin msg '%s'", description);

   m_missing_plugins.push_back(description);
   g_free(description);
   return true;
}

// Dispatch the gstreamer message.
bool GstPlayer::on_bus_message(GstBus* bus, GstMessage* msg) {
   // If the pipeline is tearing down or already gone, ignore messages.
   if (!m_pipeline) {
      return true;
   }
   se_dbg_msg(SE_DBG_VIDEO_PLAYER, "type='%s' name='%s'", GST_MESSAGE_TYPE_NAME(msg), GST_OBJECT_NAME(GST_MESSAGE_SRC(msg)));

   GstMessageType msg_type = GST_MESSAGE_TYPE(msg);
   switch (msg_type) {
      case GST_MESSAGE_ELEMENT:
         on_bus_message_element(msg);
         break;
      case GST_MESSAGE_EOS:
         on_bus_message_eos(msg);
         break;
      case GST_MESSAGE_ERROR:
         on_bus_message_error(msg);
         break;
      case GST_MESSAGE_WARNING:
         on_bus_message_warning(msg);
         break;
      case GST_MESSAGE_STATE_CHANGED:
         on_bus_message_state_changed(msg);
         break;
      case GST_MESSAGE_SEGMENT_DONE:
         on_bus_message_segment_done(msg);
         break;
      case GST_MESSAGE_ASYNC_DONE:
         if (m_pipeline_async_done == false) {
            // We wait for the first async-done message, then the application
            // can ask about duration, info about the stream...
            m_pipeline_async_done = true;
            send_message(Player::STREAM_READY);
         }
         break;
      case GST_MESSAGE_STREAM_COLLECTION:
         on_bus_message_stream_collection(msg);
         break;
      default:
         break;
   }
   return true;
}

// Check the missing plugin.
// If is missing add in the list of missing plugins.
// This list should be show later.
void GstPlayer::on_bus_message_element(GstMessage* msg) {
   se_dbg(SE_DBG_VIDEO_PLAYER);
   is_missing_plugin_message(msg);
}

// An error is detected.
// Destroy the pipeline and show the error message in a dialog.
void GstPlayer::on_bus_message_error(GstMessage* msg) {
   se_dbg(SE_DBG_VIDEO_PLAYER);

   check_missing_plugins();

   GError* err = nullptr;
   gchar* err_dbg = nullptr;
   gst_message_parse_error(msg, &err, &err_dbg);

   g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
   g_printerr("Debugging information: %s\n", err_dbg ? err_dbg : "none");

   se_dbg_msg(SE_DBG_VIDEO_PLAYER, "GST_MESSAGE_ERROR : %s [%s]", err ? err->message : "(null)", err_dbg ? err_dbg : "(null)");

   dialog_error(build_message(_("Media file could not be played.\n%s"), get_uri().c_str()), err->message);

   g_clear_error(&err);
   g_free(err_dbg);

   set_pipeline_null();
}

// An warning message is detected.
void GstPlayer::on_bus_message_warning(GstMessage* msg) {
   se_dbg(SE_DBG_VIDEO_PLAYER);

   check_missing_plugins();

   GError* err = nullptr;
   gchar* err_dbg = nullptr;
   gst_message_parse_warning(msg, &err, &err_dbg);

   g_warning("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
   g_warning("Debugging information: %s\n", err_dbg ? err_dbg : "none");

   se_dbg_msg(SE_DBG_VIDEO_PLAYER, "GST_MESSAGE_WARNING : %s [%s]", err ? err->message : "(null)", err_dbg ? err_dbg : "(null)");

   g_clear_error(&err);
   g_free(err_dbg);
}

// The state of the pipeline has changed.
// Update the player state.
void GstPlayer::on_bus_message_state_changed(GstMessage* msg) {
   se_dbg(SE_DBG_VIDEO_PLAYER);

   // We only update when it's the pipeline object
   if (GST_MESSAGE_SRC(msg) != GST_OBJECT(m_pipeline))
      return;

   GstState old_state, new_state, pending;

   gst_message_parse_state_changed(msg, &old_state, &new_state, &pending);

   // Update the current state of the pipeline
   m_pipeline_state = new_state;

   if (old_state == GST_STATE_NULL && new_state == GST_STATE_READY) {
      set_player_state(NONE);
   } else if (old_state == GST_STATE_READY && new_state == GST_STATE_PAUSED) {
      set_player_state(PAUSED);
      check_missing_plugins();
   } else if (old_state == GST_STATE_PAUSED && new_state == GST_STATE_PLAYING) {
      set_player_state(PLAYING);
   } else if (old_state == GST_STATE_PLAYING && new_state == GST_STATE_PAUSED) {
      set_player_state(PAUSED);
   } else if (old_state == GST_STATE_PAUSED && new_state == GST_STATE_READY) {
      set_player_state(NONE);
   } else if (old_state == GST_STATE_READY && new_state == GST_STATE_NULL) {
      set_player_state(NONE);
   }
}

// End-of-stream (segment or stream) has been detected,
// update the pipeline state to PAUSED.
// Seek to the beginning if it's the end of the stream.
void GstPlayer::on_bus_message_eos(GstMessage* msg) {
   se_dbg(SE_DBG_VIDEO_PLAYER);

   // FIXME with seek_loop
   set_pipeline_state(GST_STATE_PAUSED);

   if (get_position() == get_duration())
      seek(0);
}

// The pipeline completed playback of a segment.
// If the looping is activated send new seek event.
// Works only with play_subtitle.
void GstPlayer::on_bus_message_segment_done(GstMessage* msg) {
   se_dbg(SE_DBG_VIDEO_PLAYER);

   if (!(m_loop_seek && m_subtitle_play))
      return;

   // TODO debug information of MessageSegmentDone
   GstSeekFlags flags = (GstSeekFlags)(GST_SEEK_FLAG_ACCURATE | GST_SEEK_FLAG_SEGMENT);
   seek(m_subtitle_play.get_start().totalmsecs, m_subtitle_play.get_end().totalmsecs, flags);
}

// Handles GST_MESSAGE_STREAM_COLLECTION.
// - Replaces any existing m_stream_collection_* with the one parsed from msg.
void GstPlayer::on_bus_message_stream_collection(GstMessage* msg) {
   se_dbg(SE_DBG_VIDEO_PLAYER);

   m_stream_collection_audio.clear();
   m_stream_collection_video.clear();

   GstStreamCollection* stream_collection;
   gst_message_parse_stream_collection(msg, &stream_collection);
   if (!stream_collection) {
      return;
   }

   guint numStreams = gst_stream_collection_get_size(stream_collection);
   for (guint i = 0; i < numStreams; i++) {
      GstStream* stream = gst_stream_collection_get_stream(stream_collection, i);

      if (!GST_IS_STREAM(stream))
         continue;

      GstStreamType type = gst_stream_get_stream_type(stream);
      const gchar* stream_id = gst_stream_get_stream_id(stream);

      // parse tags for LANGUAGE CODE
      Glib::ustring language;
      GstTagList* tags = gst_stream_get_tags(stream);
      if (tags) {
         const GValue* tagValue = gst_tag_list_get_value_index(tags, GST_TAG_LANGUAGE_CODE, 0);
         if (G_VALUE_HOLDS_STRING(tagValue)) {
            gchar* str = g_value_dup_string(tagValue);
            language = str;
            g_free(str);
         }
         gst_tag_list_unref(tags);
      }

      se_dbg_msg(SE_DBG_VIDEO_PLAYER, "Stream %d: Type=%s, ID=%s, language=%s\n", i, gst_stream_type_get_name(type), stream_id, language.c_str());

      switch (type) {
         case GST_STREAM_TYPE_VIDEO:
            m_stream_collection_video.push_back({stream_id, language});
            break;
         case GST_STREAM_TYPE_AUDIO:
            m_stream_collection_audio.push_back({stream_id, language});
            break;
         case GST_STREAM_TYPE_TEXT:
            break;
         default:
            break;
      }
      // Drop our reference to the retrieved GstStream.
      gst_object_unref(stream);
   }
   // RELEASE our reference, we use m_stream_collection_video and m_stream_collection_audio for references
   g_object_unref(stream_collection);
}

// The video-player configuration has changed, update the player.
void GstPlayer::on_config_video_player_changed(const Glib::ustring& key, const Glib::ustring& value) {
   se_dbg_msg(SE_DBG_VIDEO_PLAYER, "%s %s", key.c_str(), value.c_str());

   if (key == "repeat") {
      set_repeat(utility::string_to_bool(value));
   }

   if (!m_pipeline) {
      return;
   }

   if (key == "shaded-background" && m_textoverlay) {
      g_object_set(G_OBJECT(m_textoverlay), "shaded-background", utility::string_to_bool(value), NULL);
   } else if (key == "font-desc" && m_textoverlay) {
      g_object_set(G_OBJECT(m_textoverlay), "font-desc", value.c_str(), NULL);
   } else if (key == "force-aspect-ratio") {
      GstElement* gtksink = gst_bin_get_by_name(GST_BIN(m_pipeline), "gtksink");
      g_object_set(GST_OBJECT(gtksink), "force-aspect-ratio", utility::string_to_bool(value), NULL);
      g_object_unref(gtksink);
   }
}

void GstPlayer::update_pipeline_state_and_timeout() {
   se_dbg(SE_DBG_VIDEO_PLAYER);

   if (!m_pipeline)
      return;
   GstState old_st, new_st;
   gst_element_get_state(m_pipeline, &old_st, &new_st, 100 * GST_MSECOND);
   got_tick();
}

// Set up the duration value of the stream if need.
bool GstPlayer::update_pipeline_duration() {
   se_dbg(SE_DBG_VIDEO_PLAYER);

   if (!m_pipeline)
      return false;

   m_pipeline_duration = GST_CLOCK_TIME_NONE;

   gint64 dur = -1;
   if (gst_element_query_duration(m_pipeline, GST_FORMAT_TIME, &dur) && dur != -1) {
      m_pipeline_duration = dur;
      se_dbg_msg(SE_DBG_VIDEO_PLAYER, "Success to query the duration (%" GST_TIME_FORMAT ")", GST_TIME_ARGS(dur));
      // send_message(STREAM_DURATION_CHANGED);
      return true;
   }
   se_dbg_msg(SE_DBG_VIDEO_PLAYER, "The query of the duration of the stream failed");
   return false;
}

// Return the number of audio track.
gint GstPlayer::get_n_audio() {
   se_dbg(SE_DBG_VIDEO_PLAYER);

   if (m_pipeline == nullptr) {
      return 0;
   }
   return m_stream_collection_audio.size();
}

// Sets the current audio track. (-1 = auto)
void GstPlayer::set_current_audio(gint track) {
   se_dbg_msg(SE_DBG_VIDEO_PLAYER, "track=%d", track);

   if (m_pipeline == nullptr) {
      return;
   }

   if (track < 0) {
      track = 0;  // auto, first track
   }

   GList* selected_streams = NULL;
   if (m_stream_collection_video.size() > 0) {
      selected_streams = g_list_append(selected_streams, (char*)m_stream_collection_video[0].first.c_str());
   }
   if (m_stream_collection_audio.size() > 0) {
      // FIXME: check for size of track < m_stream_collection_audio.size()
      selected_streams = g_list_append(selected_streams, (char*)m_stream_collection_audio[track].first.c_str());
   }

   if (selected_streams == NULL) {
      se_dbg_msg(SE_DBG_VIDEO_PLAYER, "no stream found");
      return;
   }
   gst_element_send_event(m_pipeline, gst_event_new_select_streams(selected_streams));
   g_list_free(selected_streams);

   send_message(Player::STREAM_AUDIO_CHANGED);
}

// Return the framerate of the video or zero (0).
// Update numerator and denominator if the values are not null.
float GstPlayer::get_framerate(int* numerator, int* denominator) {
   se_dbg(SE_DBG_VIDEO_PLAYER);

   if (!m_pipeline)
      return 0.0f;

   float fr = 0.0f;
   int num = 0, den = 1;

   // Prefer the 'video_sink' pad on our textoverlay inside the video bin.
   // This sits just upstream of the actual sink and has negotiated video caps.
   GstPad* pad = nullptr;
   if (m_textoverlay) {
      pad = gst_element_get_static_pad(m_textoverlay, "video_sink");
   }

   // Fallback to the 'sink' pad on the configured playbin3 'video-sink'.
   GstElement* video_sink = nullptr;
   if (!pad) {
      g_object_get(G_OBJECT(m_pipeline), "video-sink", &video_sink, NULL);
      if (video_sink) {
         pad = gst_element_get_static_pad(video_sink, "sink");
      }
   }

   if (!pad) {
      if (video_sink)
         gst_object_unref(video_sink);
      return 0.0f;
   }

   // Use negotiated caps when available; otherwise query potential caps.
   GstCaps* caps = gst_pad_get_current_caps(pad);
   if (!caps) {
      caps = gst_pad_query_caps(pad, NULL);
   }

   if (caps) {
      // Parse caps using GstVideoInfo to read fps numerator/denominator.
      GstVideoInfo vinfo;
      gst_video_info_init(&vinfo);
      if (gst_video_info_from_caps(&vinfo, caps)) {
         num = vinfo.fps_n;
         den = vinfo.fps_d;
      } else {
         // Fallback: inspect a fixed fraction field named "framerate".
         const GstStructure* s = gst_caps_get_structure(caps, 0);
         const GValue* fps_val = s ? gst_structure_get_value(s, "framerate") : NULL;
         if (fps_val && GST_VALUE_HOLDS_FRACTION(fps_val)) {
            num = gst_value_get_fraction_numerator(fps_val);
            den = gst_value_get_fraction_denominator(fps_val);
         }
      }
      gst_caps_unref(caps);
   }

   if (den != 0) {
      fr = static_cast<float>(num) / static_cast<float>(den);
   }
   if (numerator) {
      *numerator = num;
   }
   if (denominator) {
      *denominator = den;
   }

   se_dbg_msg(SE_DBG_VIDEO_PLAYER, "framerate: %f (num: %d, denom: %d)", fr, num, den);

   // Release temporary references.
   gst_object_unref(pad);
   if (video_sink) {
      gst_object_unref(video_sink);
   }

   return fr;
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
