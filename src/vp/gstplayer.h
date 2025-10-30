#pragma once

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

#include <gst/gst.h>
#include <gtkmm.h>
#include <player.h>

class GstPlayer : public Gtk::Bin, public Player {
 public:
  // Constructor
  // Init values
  GstPlayer();

  // Destructor
  // Set up pipeline to NULL.
  virtual ~GstPlayer();

  // Create the pipeline and sets the uri.
  bool open(const Glib::ustring &uri);

  // Set up the pipeline to NULL.
  void close();

  // Return the uri of the current video.
  Glib::ustring get_uri();

  // Sets the pipeline state to playing.
  void play();

  // Try to play the segment defined by the subtitle (from start to end).
  // This function supports the looping.
  // The state is sets to playing.
  void play_subtitle(const Subtitle &sub);

  // Try to play the segment defined (start to end).
  // This function don't support the mode looping.
  // The state is sets to playing.
  void play_segment(const SubtitleTime &start, const SubtitleTime &end);

  // Sets the pipeline state to paused.
  void pause();

  // Return true if the state of the pipeline is playing.
  bool is_playing();

  // Return the duration of the stream or 0.
  long get_duration();

  // Return the current position in the stream.
  long get_position();

  bool seek(long start, long end, const GstSeekFlags &flags);

  // Seeking, the state of the pipeline is not modified.
  void seek(long position);

  // Update the text overlay with this new text.
  void set_subtitle_text(const Glib::ustring &text);

  // Sets the new playback rate. Used for slow or fast motion.
  // Default value : 1.0
  // Min : 0.1
  // Max : 1.5
  void set_playback_rate(double value);

  // Return the playback rate.
  double get_playback_rate();

  // Enable/Disable the repeat mode.
  // Works only with play_subtitle.
  void set_repeat(bool state);

  // Return the framerate of the video.
  // Update numerator and denominator if the values are not null.
  virtual float get_framerate(int *numerator = NULL, int *denominator = NULL);

  // protected:

  // Create a gstreamer pipeline (Gst PlayBin3), initialize the
  // audio and video sink with the configuration.
  // Connect the bug message to the player.
  bool create_pipeline();

  // Return a gstreamer audio sink from the configuration option.
  GstElement *gen_audio_element();

  // Return a gstreamer video sink from the configuration option.
  GstElement *gen_video_element();

  // Check if are missing plugin, if it's true display a message.
  // Return true if missing.
  bool check_missing_plugins();

  // Check if it's a Missing Plugin Message.
  // Add the description of the missing plugin in the list.
  bool is_missing_plugin_message(GstMessage *msg);

  // Dispatch the gstreamer message.
  bool on_bus_message(GstBus *bus, GstMessage *msg);

  // The state of the pipeline has changed.
  // Update the player state.
  void on_bus_message_state_changed(GstMessage *msg);

  // Check the missing plugin.
  // If is missing add in the list of missing plugins.
  // This list should be show later.
  void on_bus_message_element(GstMessage *msg);

  // An error is detected.
  // Destroy the pipeline and show the error message in a dialog.
  void on_bus_message_error(GstMessage *msg);

  // An error is detected.
  void on_bus_message_warning(GstMessage *msg);

  // End-of-stream (segment or stream) has been detected,
  // update the pipeline state to PAUSED.
  // Seek to the beginning if it's the end of the stream.
  void on_bus_message_eos(GstMessage *msg);

  // The pipeline completed playback of a segment.
  // If the looping is activated send new seek event.
  // Works only with play_subtitle.
  void on_bus_message_segment_done(GstMessage *msg);

  // Handle GST_MESSAGE_STREAM_COLLECTION: store the collection and
  // enumerate streams for debugging. Keeps a ref in m_stream_collection.
  void on_bus_message_stream_collection(GstMessage *msg);

  // Set the state of the pipeline.
  // The state change can be asynchronously.
  bool set_pipeline_state(GstState state);

  // Sets the state of the pipeline to NULL.
  void set_pipeline_null();

  // The video-player configuration has changed, update the player.
  void on_config_video_player_changed(const Glib::ustring &key, const Glib::ustring &value);

  void update_pipeline_state_and_timeout();

  // Set up the duration value of the stream if need.
  bool update_pipeline_duration();

  // Return the number of audio track.
  gint get_n_audio();

  // Sets the current audio track. (-1 = auto)
  void set_current_audio(gint track);

  guint get_text_valignment_based_on_config();

 private:
  // Returns the Nth stream of a given type from the current GstStreamCollection.
  // - stream_type: The desired type (e.g., GST_STREAM_TYPE_AUDIO/VIDEO).
  // - track_num: Zero-based index among streams of that type (0 = first).
  // Caller must gst_object_unref() the returned stream when done.
  GstStream *get_stream_from_type(GstStreamType stream_type, gint track_num);

 protected:
  guint m_watch_id;
  // Gstreamer Elements
  GstElement *m_pipeline;
  GtkWidget *m_gtksink_widget;
  GstElement *m_textoverlay;
  std::vector<std::pair<std::string, Glib::ustring>> m_stream_collection_audio;
  std::vector<std::pair<std::string, Glib::ustring>> m_stream_collection_video;

  bool m_pipeline_async_done;
  GstState m_pipeline_state;
  gint64 m_pipeline_duration;
  double m_pipeline_rate;
  bool m_loop_seek;
  Subtitle m_subtitle_play;
  Glib::ustring m_uri;
  std::list<Glib::ustring> m_missing_plugins;
};
