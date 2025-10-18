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
#include <utility.h>
#include <waveform.h>

#include <iomanip>
#include <iostream>

#include "mediadecoder.h"

class WaveformGenerator : public Gtk::Dialog, public MediaDecoder {
 public:
  WaveformGenerator(const Glib::ustring &uri, Glib::RefPtr<Waveform> &wf)
      : Gtk::Dialog(_("Generate Waveform"), true), MediaDecoder(1000), m_duration(GST_CLOCK_TIME_NONE), m_n_channels(0) {
    se_dbg_msg(SE_DBG_PLUGINS, "uri=%s", uri.c_str());

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
        wf = Glib::RefPtr<Waveform>(new Waveform);
        wf->m_duration = m_duration / GST_MSECOND;
        wf->m_n_channels = m_n_channels;
        for (guint i = 0; i < m_n_channels; ++i) {
          wf->m_channels[i] = std::vector<double>(m_values[i].begin(), m_values[i].end());
        }
        wf->m_video_uri = uri;
      }
    } catch (const std::runtime_error &ex) {
      std::cerr << ex.what() << std::endl;
    }
  }

  // Create audio bin
  GstElement *create_element(const Glib::ustring &structure_name) {
    se_dbg_msg(SE_DBG_PLUGINS, "structure_name=%s", structure_name.c_str());
    // We only need and want create the video sink
    if (structure_name.find("audio") == Glib::ustring::npos)
      return nullptr;

    GError *error = nullptr;
    GstElement *audiobin = gst_parse_bin_from_description("audioconvert ! level name=level ! fakesink name=asink", true, &error);
    if (error) {
      // FIXME: print error
      g_clear_error(&error);
      return nullptr;
    }
    // Set the new sink tp READY as well
    GstStateChangeReturn retst = gst_element_set_state(audiobin, GST_STATE_READY);
    if (retst == GST_STATE_CHANGE_FAILURE)
      std::cerr << "Could not change state of new sink: " << retst << std::endl;

    return audiobin;
  }

  // BUS MESSAGE
  bool on_bus_message(GstBus *bus, GstMessage *msg) {
    MediaDecoder::on_bus_message(bus, msg);

    if (GST_MESSAGE_TYPE(msg) != GST_MESSAGE_ELEMENT)
      return true;

    const GstStructure *structure = gst_message_get_structure(msg);
    Glib::ustring struct_name = gst_structure_get_name(structure);
    // FIXME: check if ok
    // if (msg->get_structure().get_name() == "level")
    if (struct_name != "level")
      return true;
    return on_bus_message_element_level(msg);
  }

  // Update the progress bar
  bool on_timeout() {
    se_dbg(SE_DBG_PLUGINS);

    if (!m_pipeline)
      return false;

    gint64 pos = 0, len = 0;
    if (gst_element_query_position(m_pipeline, GST_FORMAT_TIME, &pos) && gst_element_query_duration(m_pipeline, GST_FORMAT_TIME, &len)) {
      double percent = static_cast<double>(pos) / static_cast<double>(len);

      percent = CLAMP(percent, 0.0, 1.0);

      m_progressbar.set_fraction(percent);
      m_progressbar.set_text(time_to_string(pos) + " / " + time_to_string(len));

      return pos != len;
    }

    return true;
  }

  bool on_bus_message_element_level(GstMessage *msg) {
    se_dbg_msg(SE_DBG_PLUGINS, "type='%s' name='%s'", GST_MESSAGE_TYPE_NAME(msg), GST_OBJECT_NAME(GST_MESSAGE_SRC(msg)));

    const GstStructure *structure = gst_message_get_structure(msg);
    const GValue *array_val = gst_structure_get_value(structure, "rms");
    GValueArray *rms_arr = static_cast<GValueArray *>(g_value_get_boxed(array_val));

    guint num_channels = rms_arr->n_values;

    guint first_channel, last_channel;
    if (num_channels >= 6) {
      first_channel = 1;
      last_channel = 3;
    } else if (num_channels == 5) {
      first_channel = 1;
      last_channel = 2;
    } else if (num_channels == 2) {
      first_channel = 0;
      last_channel = 1;
    } else {
      first_channel = last_channel = 0;
    }
    // build the number of channels
    m_n_channels = last_channel - first_channel + 1;

    // get peak from channels
    for (guint c = first_channel, i = 0; c <= last_channel; ++c, ++i) {
      double peak = pow(10, g_value_get_double(g_value_array_get_nth(rms_arr, c)) / 20);
      m_values[i].push_back(peak);
    }
    return true;
  }

  void on_work_finished() {
    se_dbg(SE_DBG_PLUGINS);

    // set duration to position at eos
    gint64 pos = 0;

    if (m_pipeline && gst_element_query_position(m_pipeline, GST_FORMAT_TIME, &pos)) {
      m_duration = pos;
      response(Gtk::RESPONSE_OK);
    } else {
      GST_ELEMENT_ERROR(m_pipeline, STREAM, FAILED, (_("Could not determinate the duration of the stream.")), (NULL));
    }
  }

  void on_work_cancel() {
    se_dbg(SE_DBG_PLUGINS);

    response(Gtk::RESPONSE_CANCEL);
  }

 protected:
  Gtk::ProgressBar m_progressbar;
  guint64 m_duration;
  guint m_n_channels;
  std::list<gdouble> m_values[3];
};

Glib::RefPtr<Waveform> generate_waveform_from_file(const Glib::ustring &uri) {
  Glib::RefPtr<Waveform> wf;
  WaveformGenerator ui(uri, wf);
  return wf;
}
