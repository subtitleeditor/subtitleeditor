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

#include <cfg.h>
#include <gtkmm.h>
#include <keyframes.h>
#include <utility.h>

#include <iomanip>
#include <iostream>

#include "mediadecoder.h"

class KeyframesGeneratorUsingFrame : public Gtk::Dialog, public MediaDecoder {
 public:
  KeyframesGeneratorUsingFrame(const Glib::ustring &uri, Glib::RefPtr<KeyFrames> &keyframes)
      : Gtk::Dialog(_("Generate Keyframes"), true), MediaDecoder(1000), m_duration(0), m_prev_frame_size(0), m_prev_frame(NULL), m_difference(0.2f) {
    set_border_width(12);
    set_default_size(300, -1);
    get_vbox()->pack_start(m_progressbar, false, false);
    add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    m_progressbar.set_show_text(true);
    m_progressbar.set_text(_("Waiting..."));
    show_all();

    try {
      read_config();
      create_pipeline(uri);

      if (run() == Gtk::RESPONSE_OK) {
        keyframes = Glib::RefPtr<KeyFrames>(new KeyFrames);
        keyframes->insert(keyframes->end(), m_values.begin(), m_values.end());
        keyframes->set_video_uri(uri);
      }
    } catch (const std::runtime_error &ex) {
      std::cerr << ex.what() << std::endl;
    }
  }

  ~KeyframesGeneratorUsingFrame(void) {
    delete[] m_prev_frame;
  }

  void read_config() {
    if (cfg::has_key("KeyframesGeneratorUsingFrame", "difference")) {
      cfg::set_string("KeyframesGeneratorUsingFrame", "difference", "0.2");
      cfg::set_comment("KeyframesGeneratorUsingFrame", "difference", "difference between frames as percent");
    }
    m_difference = cfg::get_float("KeyframesGeneratorUsingFrame", "difference");
  }

  static void static_handoff_callback(GstElement *fakesink, GstBuffer *buffer, GstPad *pad, gpointer data) {
    KeyframesGeneratorUsingFrame *kfg = static_cast<KeyframesGeneratorUsingFrame *>(data);
    kfg->on_video_identity_handoff(fakesink, buffer, pad);
  }

  // Check buffer and try to catch keyframes.
  void on_video_identity_handoff(GstElement *, GstBuffer *buf, GstPad *) {
    GstMapInfo map;
    gst_buffer_map(GST_BUFFER(buf), &map, GST_MAP_READ);

    // first frame or change of buffer size, alloc & push
    if (!m_prev_frame || map.size != m_prev_frame_size) {
      delete[] m_prev_frame;
      m_prev_frame_size = map.size;
      m_prev_frame = new guint8[m_prev_frame_size];

      m_values.push_back(GST_BUFFER_PTS(buf) / GST_MSECOND);
    } else if (compare_frame(m_prev_frame, map.data, map.size)) {
      m_values.push_back(GST_BUFFER_PTS(buf) / GST_MSECOND);
    }
    // update the previous frame with this one
    memcpy(m_prev_frame, map.data, map.size);

    gst_buffer_unmap(GST_BUFFER(buf), &map);
  }

  bool compare_frame(const guint8 *old_frame, const guint8 *new_frame, gsize size) {
    guint64 delta = 0;
    guint64 full = size / 3;

    gulong diff, i, j;
    long tmp;
    // calculate difference between frames
    for (i = 0; i < full; ++i) {
      diff = 0;
      // get max difference in individual color channels
      for (j = 0; j < 3; ++j) {
        tmp = (int)new_frame[3 * i + j] - (int)old_frame[3 * i + j];
        if (tmp < 0)
          tmp = -tmp;
        diff = tmp > diff ? tmp : diff;
      }
      // add max color diff to total delta
      delta += diff;
    }
    full *= 255;

    // >20% difference => scene cut
    return ((double)delta / (double)full > m_difference);
  }

  // Create video bin
  GstElement *create_element(const Glib::ustring &structure_name) {
    // We only need and want create the video sink
    if (structure_name.find("video") == Glib::ustring::npos)
      return nullptr;

    GstElement *fakesink = gst_element_factory_make("fakesink", NULL);
    // fakesink->set_sync(false);
    g_object_set(G_OBJECT(fakesink), "silent", TRUE, NULL);
    g_object_set(G_OBJECT(fakesink), "signal-handoffs", TRUE, NULL);
    g_signal_connect(fakesink, "handoff", G_CALLBACK(KeyframesGeneratorUsingFrame::static_handoff_callback), this);

    // Set the new sink tp READY as well
    GstStateChangeReturn retst = gst_element_set_state(fakesink, GST_STATE_READY);
    if (retst == GST_STATE_CHANGE_FAILURE)
      std::cerr << "Could not change state of new sink: " << retst << std::endl;

    return fakesink;
  }

  // Update the progress bar
  bool on_timeout() {
    if (!m_pipeline)
      return false;

    gint64 pos = 0, len = 0;
    if (gst_element_query_position(m_pipeline, GST_FORMAT_TIME, &pos) && gst_element_query_duration(m_pipeline, GST_FORMAT_TIME, &len)) {
      double percent = static_cast<double>(pos) / static_cast<double>(len);

      percent = CLAMP(percent, 0.0, 1.0);

      m_progressbar.set_fraction(percent);
      m_progressbar.set_text(time_to_string(pos) + " / " + time_to_string(len));
      m_duration = len;

      return pos != len;
    }
    return true;
  }

  void on_work_finished() {
    response(Gtk::RESPONSE_OK);
  }

  void on_work_cancel() {
    response(Gtk::RESPONSE_CANCEL);
  }

 protected:
  Gtk::ProgressBar m_progressbar;

  std::list<long> m_values;
  guint64 m_duration;
  guint64 m_prev_frame_size;
  guint8 *m_prev_frame;
  gfloat m_difference;
};

Glib::RefPtr<KeyFrames> generate_keyframes_from_file_using_frame(const Glib::ustring &uri) {
  Glib::RefPtr<KeyFrames> kf;
  KeyframesGeneratorUsingFrame ui(uri, kf);
  return kf;
}
