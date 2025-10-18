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

#include <gtkmm.h>
#include <keyframes.h>
#include <utility.h>

#include <iomanip>
#include <iostream>

#include "mediadecoder.h"

class KeyframesGenerator : public Gtk::Dialog, public MediaDecoder {
 public:
  KeyframesGenerator(const Glib::ustring &uri, Glib::RefPtr<KeyFrames> &keyframes) : Gtk::Dialog(_("Generate Keyframes"), true), MediaDecoder(1000) {
    set_border_width(12);
    set_default_size(300, -1);
    get_vbox()->pack_start(m_progressbar, false, false);
    add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    m_progressbar.set_show_text(true);
    m_progressbar.set_text(_("Waiting..."));
    show_all();

    try {
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

  static void static_handoff_callback(GstElement *fakesink, GstBuffer *buffer, GstPad *pad, gpointer data) {
    KeyframesGenerator *kfg = static_cast<KeyframesGenerator *>(data);
    kfg->on_video_identity_handoff(fakesink, buffer, pad);
  }
  // Check buffer and try to catch keyframes.
  void on_video_identity_handoff(GstElement *, GstBuffer *buf, GstPad *) {
    if (!GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DELTA_UNIT)) {
      long pos = GST_BUFFER_PTS(buf) / GST_MSECOND;
      m_values.push_back(pos);
    }
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
    g_signal_connect(fakesink, "handoff", G_CALLBACK(KeyframesGenerator::static_handoff_callback), this);

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
};

Glib::RefPtr<KeyFrames> generate_keyframes_from_file(const Glib::ustring &uri) {
  Glib::RefPtr<KeyFrames> kf;
  KeyframesGenerator ui(uri, kf);
  return kf;
}
