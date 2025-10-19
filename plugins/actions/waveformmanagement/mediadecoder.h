#pragma once

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

#include <gst/gst.h>
#include <gst/pbutils/missing-plugins.h>

#include <iomanip>
#include <iostream>

#include "gstreamer_utility.h"
#include "utility.h"

// Class to help with gstreamer(mm)
class MediaDecoder : virtual public sigc::trackable {
 public:
  explicit MediaDecoder(guint timeout = 0) : m_watch_id(0), m_pipeline(nullptr), m_timeout(timeout) {
  }

  virtual ~MediaDecoder() {
    destroy_pipeline();
  }

  static void static_on_pad_added(GstElement *src, GstPad *pad, void *data) {
    MediaDecoder *md = static_cast<MediaDecoder *>(data);
    md->on_pad_added(src, pad);
  }

  static gboolean static_handle_message(GstBus *bus, GstMessage *msg, void *data) {
    MediaDecoder *md = static_cast<MediaDecoder *>(data);
    return md->on_bus_message(bus, msg);
  }

  void create_pipeline(const Glib::ustring &uri) {
    se_dbg_msg(SE_DBG_PLUGINS, "uri=%s", uri.c_str());

    if (m_pipeline)
      destroy_pipeline();

    m_pipeline = gst_pipeline_new("pipeline");

    GstElement *giosrc = gst_element_factory_make("giosrc", NULL);

    GstElement *decodebin = gst_element_factory_make("decodebin", "decoder");

    g_signal_connect(decodebin, "pad-added", G_CALLBACK(static_on_pad_added), this);

    gst_bin_add_many(GST_BIN(m_pipeline), giosrc, decodebin, NULL);

    if (!gst_element_link(giosrc, decodebin)) {
      g_printerr("Elements could not be linked.\n");
      gst_object_unref(m_pipeline);
      return;
    }

    g_object_set(giosrc, "location", uri.c_str(), NULL);

    // Bus watching
    GstBus *bus = gst_element_get_bus(m_pipeline);
    m_watch_id = gst_bus_add_watch(bus, (GstBusFunc)static_handle_message, this);
    gst_object_unref(bus);

    if (gst_element_set_state(m_pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
      se_dbg_msg(SE_DBG_PLUGINS, "Failed to change the state of the pipeline to PLAYING");
    }
  }

  void destroy_pipeline() {
    se_dbg(SE_DBG_PLUGINS);

    if (m_connection_timeout)
      m_connection_timeout.disconnect();

    if (m_pipeline) {
      g_source_remove(m_watch_id);
      gst_element_set_state(m_pipeline, GST_STATE_NULL);
      g_object_unref(m_pipeline);
    }

    m_watch_id = 0;
    m_pipeline = nullptr;
  }

  virtual void on_pad_added(GstElement *, GstPad *newpad) {
    se_dbg(SE_DBG_PLUGINS);

    GstCaps *caps = gst_pad_query_caps(newpad, NULL);

    se_dbg_msg(SE_DBG_PLUGINS, "newpad->caps: %s", GST_PAD_NAME(caps));

    GstStructure *structure = gst_caps_get_structure(caps, 0);
    if (!structure) {
      // FIXME: TODO
      // FIXME: unref at the end too
      gst_caps_unref(caps);
      return;
    }
    const gchar *structure_name = gst_structure_get_name(structure);
    GstElement *sink = create_element(structure_name);
    // FIXME: unref sink after ?
    if (sink) {
      // Add bin to the pipeline
      gst_bin_add_many(GST_BIN(m_pipeline), sink, NULL);

      // Set the new sink tp PAUSED as well
      GstStateChangeReturn retst = gst_element_set_state(sink, GST_STATE_PAUSED);
      if (retst == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Could not change state of new sink: " << retst << std::endl;
        se_dbg_msg(SE_DBG_PLUGINS, "Could not change the state of new sink");
        gst_bin_remove(GST_BIN(m_pipeline), sink);
        return;
      }
      // Get the ghostpad of the sink bin
      GstPad *sinkpad = gst_element_get_static_pad(sink, "sink");

      GstPadLinkReturn ret = gst_pad_link(newpad, sinkpad);

      if (ret != GST_PAD_LINK_OK && ret != GST_PAD_LINK_WAS_LINKED) {
        std::cerr << "Linking of pads " << GST_PAD_NAME(newpad) << " and " << GST_PAD_NAME(sinkpad) << " failed." << std::endl;
        se_dbg_msg(SE_DBG_PLUGINS, "Linking of pads failed");
      } else {
        se_dbg_msg(SE_DBG_PLUGINS, "Pads linking with success");
      }
    } else {
      se_dbg_msg(SE_DBG_PLUGINS, "create_element return an NULL sink");
    }
  }

  // BUS MESSAGE
  virtual bool on_bus_message(GstBus *, GstMessage *msg) {
    se_dbg_msg(SE_DBG_PLUGINS, "type='%s' name='%s'", GST_MESSAGE_TYPE_NAME(msg), GST_OBJECT_NAME(GST_MESSAGE_SRC(msg)));

    GstMessageType msg_type = GST_MESSAGE_TYPE(msg);
    switch (msg_type) {
      case GST_MESSAGE_ELEMENT:
        return on_bus_message_element(msg);
      case GST_MESSAGE_EOS:
        return on_bus_message_eos(msg);
      case GST_MESSAGE_ERROR:
        return on_bus_message_error(msg);
      case GST_MESSAGE_WARNING:
        return on_bus_message_warning(msg);
      case GST_MESSAGE_STATE_CHANGED:
        return on_bus_message_state_changed(msg);
      default:
        break;
    }
    return true;
  }

  virtual bool on_bus_message_error(GstMessage *msg) {
    check_missing_plugins();

    GError *err = nullptr;
    gchar *err_dbg = nullptr;
    gst_message_parse_error(msg, &err, &err_dbg);

    g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
    g_printerr("Debugging information: %s\n", err_dbg ? err_dbg : "none");

    // FIXME: dialog
    // dialog_error(build_message(_("Media file could not be played.\n%s"), err->message));

    g_clear_error(&err);
    g_free(err_dbg);

    // Critical error, cancel the work.
    on_work_cancel();
    return true;
  }

  virtual bool on_bus_message_warning(GstMessage *msg) {
    check_missing_plugins();

    GError *err = nullptr;
    gchar *err_dbg = nullptr;
    gst_message_parse_error(msg, &err, &err_dbg);

    g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
    g_printerr("Debugging information: %s\n", err_dbg ? err_dbg : "none");

    // FIXME: dialog
    // dialog_error(build_message(_("Media file could not be played.\n%s"), err->message));

    g_clear_error(&err);
    g_free(err_dbg);

    return true;
  }

  virtual bool on_bus_message_state_changed(GstMessage *msg) {
    if (m_timeout > 0)
      return on_bus_message_state_changed_timeout(msg);
    return true;
  }

  virtual bool on_bus_message_eos(GstMessage *) {
    gst_element_set_state(m_pipeline, GST_STATE_PAUSED);
    on_work_finished();
    return true;
  }

  virtual bool on_bus_message_element(GstMessage *msg) {
    check_missing_plugin_message(msg);
    return true;
  }

  virtual void on_work_finished() {
    // FIXME
  }

  virtual void on_work_cancel() {
    // FIXME
  }

  virtual GstElement *create_element(const Glib::ustring &) {
    return nullptr;
  }

  virtual bool on_timeout() {
    return false;
  }

  // utility
  Glib::ustring time_to_string(gint64 time) {
    if (!GST_CLOCK_TIME_IS_VALID(time)) {
      return "0:00:000";
    }

    auto h = time / (GST_SECOND * 60 * 60);
    auto m = (time / (GST_SECOND * 60)) % 60;
    auto s = (time / GST_SECOND) % 60;
    return build_message("%02u:%02u:%03u", h, m, s);
  }

 protected:
  bool on_bus_message_state_changed_timeout(GstMessage *msg) {
    se_dbg(SE_DBG_PLUGINS);

    // We only update when it is the pipeline object
    if (GST_MESSAGE_SRC(msg) != GST_OBJECT(m_pipeline))
      return true;

    GstState old_state, new_state, pending;

    gst_message_parse_state_changed(msg, &old_state, &new_state, &pending);

    if (old_state == GST_STATE_PAUSED && new_state == GST_STATE_PLAYING) {
      if (!m_connection_timeout) {
        m_connection_timeout = Glib::signal_timeout().connect(sigc::mem_fun(*this, &MediaDecoder::on_timeout), m_timeout);
      }
    } else if (old_state == GST_STATE_PLAYING && new_state == GST_STATE_PAUSED) {
      if (m_connection_timeout) {
        m_connection_timeout.disconnect();
      }
    }
    return true;
  }

  void check_missing_plugin_message(GstMessage *msg) {
    se_dbg(SE_DBG_PLUGINS);

    if (!msg)
      return;
    if (!gst_is_missing_plugin_message(msg))
      return;

    gchar *description = gst_missing_plugin_message_get_description(msg);
    if (!description)
      return;

    se_dbg_msg(SE_DBG_PLUGINS, "missing plugin msg '%s'", description);

    m_missing_plugins.push_back(description);
    g_free(description);
    return;
  }

  bool check_missing_plugins() {
    if (m_missing_plugins.empty())
      return false;

    dialog_missing_plugins(m_missing_plugins);
    m_missing_plugins.clear();
    return true;
  }

  // Display a message for missing plugins.
  void dialog_missing_plugins(const std::list<Glib::ustring> &list) {
    Glib::ustring plugins;

    std::list<Glib::ustring>::const_iterator it = list.begin();
    std::list<Glib::ustring>::const_iterator end = list.end();

    while (it != end) {
      plugins += *it;
      plugins += "\n";
      ++it;
    }

    Glib::ustring msg =
        _("GStreamer plugins missing.\n"
          "The playback of this movie requires the following decoders which are not installed:");

    dialog_error(msg, plugins);

    se_dbg_msg(SE_DBG_UTILITY, "%s %s", msg.c_str(), plugins.c_str());
  }

 protected:
  guint m_watch_id;
  GstElement *m_pipeline;

  // timeout
  guint m_timeout;
  sigc::connection m_connection_timeout;

  std::list<Glib::ustring> m_missing_plugins;
};
