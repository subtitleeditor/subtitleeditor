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

#include "preferencepage.h"

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif

class ComboBoxOutput : public Gtk::ComboBox {
  class Column : public Gtk::TreeModel::ColumnRecord {
   public:
    Column() {
      add(label);
      add(name);
    }
    Gtk::TreeModelColumn<Glib::ustring> label;  // human label
    Gtk::TreeModelColumn<Glib::ustring> name;   // internal name
  };

 public:
  ComboBoxOutput(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &)
      : Gtk::ComboBox(cobject) {
    m_model = Gtk::ListStore::create(m_column);
    set_model(m_model);

    Gtk::CellRendererText *renderer = manage(new Gtk::CellRendererText);
    pack_start(*renderer, true);
    add_attribute(*renderer, "text", 0);
  }

  void append_output(const Glib::ustring &label, const Glib::ustring &name) {
    Gtk::TreeIter it = m_model->append();
    (*it)[m_column.label] = label;
    (*it)[m_column.name] = name;
  }

  void set_active_name(const Glib::ustring &name) {
    for (Gtk::TreeIter it = m_model->children().begin(); it; ++it) {
      if ((*it)[m_column.name] == name) {
        set_active(it);
        return;
      }
    }
  }

  Glib::ustring get_active_name() {
    Gtk::TreeIter it = get_active();
    if (it)
      return (*it)[m_column.name];
    return Glib::ustring();
  }

 protected:
  Column m_column;
  Glib::RefPtr<Gtk::ListStore> m_model;
};

class VideoPlayerPage : public PreferencePage {
 public:
  VideoPlayerPage(BaseObjectType *cobject,
                  const Glib::RefPtr<Gtk::Builder> &xml)
      : PreferencePage(cobject) {
    init_widget(xml, "fontbutton-subtitle", "video-player", "font-desc");
    init_widget(xml, "check-use-shaded-background", "video-player",
                "shaded-background");
    init_widget(xml, "check-display-translated-subtitle", "video-player",
                "display-translated-subtitle");

    init_widget(xml, "check-force-aspect-ratio", "video-player",
                "force-aspect-ratio");
    init_widget(xml, "check-automatically-open-video", "video-player",
                "automatically-open-video");

    init_widget(xml, "combo-text-valignment", "video-player",
                "text-valignment");

    // outputs
    xml->get_widget_derived("combo-audio-output", m_comboAudioOutput);
    xml->get_widget_derived("combo-video-output", m_comboVideoOutput);

    // audio output
    m_comboAudioOutput->append_output(_("Autodetect"), "autoaudiosink");
    m_comboAudioOutput->append_output(_("Pulse - PulseAudio Sound Server"),
                                      "pulsesink");
    m_comboAudioOutput->append_output(
        _("ALSA - Advanced Linux Sound Architecture"), "alsasink");
    m_comboAudioOutput->append_output(_("ESD - Enlightenment Sound Daemon"),
                                      "esdsink");
    m_comboAudioOutput->append_output(_("OSS - Open Sound System"), "osssink");
    m_comboAudioOutput->append_output(_("SDL - Simple DirectMedia Layer"),
                                      "sdlaudiosink");
    m_comboAudioOutput->append_output(_("GConf"), "gconfaudiosink");
#ifdef USE_OSX
    m_comboAudioOutput->append_output(_("OSX"), "osxaudiosink");
#endif

    // video output
#ifdef GDK_WINDOWING_WAYLAND
    if (!GDK_IS_WAYLAND_DISPLAY(get_display()->gobj()))
#endif
    {
      m_comboVideoOutput->append_output(_("Autodetect"), "autovideosink");
      m_comboVideoOutput->append_output(_("X Window System (X11/XShm/Xv)"),
                                        "xvimagesink");
      m_comboVideoOutput->append_output(_("X Window System (No Xv)"),
                                        "ximagesink");
      m_comboVideoOutput->append_output(_("SDL - Simple DirectMedia Layer"),
                                        "sdlvideosink");
      m_comboVideoOutput->append_output(_("GConf"), "gconfvideosink");
#ifdef USE_OSX
      m_comboVideoOutput->append_output(_("OSX"), "osxvideosink");
#endif
    }
    m_comboVideoOutput->append_output(_("OpenGL"), "glimagesink");

    Glib::ustring audiosink = cfg::get_string("video-player", "audio-sink");
    Glib::ustring videosink = cfg::get_string("video-player", "video-sink");

    m_comboAudioOutput->set_active_name(audiosink);
    m_comboVideoOutput->set_active_name(videosink);

    m_comboAudioOutput->signal_changed().connect(
        sigc::mem_fun(*this, &VideoPlayerPage::on_audio_output_changed));
    m_comboVideoOutput->signal_changed().connect(
        sigc::mem_fun(*this, &VideoPlayerPage::on_video_output_changed));
  }

 protected:
  void on_audio_output_changed() {
    Glib::ustring name = m_comboAudioOutput->get_active_name();
    cfg::set_string("video-player", "audio-sink", name);
  }

  void on_video_output_changed() {
    Glib::ustring name = m_comboVideoOutput->get_active_name();
    cfg::set_string("video-player", "video-sink", name);
  }

 protected:
  ComboBoxOutput *m_comboAudioOutput;
  ComboBoxOutput *m_comboVideoOutput;
};
