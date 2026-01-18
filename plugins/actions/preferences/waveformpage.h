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

#include <map>

#include "preferencepage.h"

class WaveformPage : public PreferencePage {
  public:
   WaveformPage(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& xml) : PreferencePage(cobject) {
      init_color_button(xml, "colorbutton-background", "waveform-renderer", "color-background");
      init_color_button(xml, "colorbutton-text", "waveform-renderer", "color-text");
      init_color_button(xml, "colorbutton-wave", "waveform-renderer", "color-wave");
      init_color_button(xml, "colorbutton-wave-fill", "waveform-renderer", "color-wave-fill");
      init_color_button(xml, "colorbutton-subtitle", "waveform-renderer", "color-subtitle");
      init_color_button(xml, "colorbutton-subtitle-selected", "waveform-renderer", "color-subtitle-selected");
      init_color_button(xml, "colorbutton-subtitle-invalid", "waveform-renderer", "color-subtitle-invalid");
      init_color_button(xml, "colorbutton-player-position", "waveform-renderer", "color-player-position");

      init_widget(xml, "check-display-background", "waveform", "display-background");
      init_widget(xml, "check-display-waveform-fill", "waveform", "display-waveform-fill");
      init_widget(xml, "check-display-subtitle-text", "waveform-renderer", "display-subtitle-text");

      Gtk::Button* reset;
      xml->get_widget("button-reset-to-defaults-waveform-color", reset);

      reset->signal_clicked().connect(sigc::mem_fun(*this, &WaveformPage::on_reset));
   }

  protected:
   void on_reset() {
      for (const auto& it : m_colorButtons) {
         std::string key = it.first;
         Gtk::ColorButton* button = it.second;
         if (!button)
            continue;
         auto value = cfg::get_default("waveform-renderer", key);
         if (value.empty())
            continue;
         Color color(value);
         color.initColorButton(*button);
      }
   }

   void init_color_button(const Glib::RefPtr<Gtk::Builder>& xml,
                          const Glib::ustring& widget_name,
                          const Glib::ustring& config_group,
                          const Glib::ustring& config_key) {
      m_colorButtons[config_key] = dynamic_cast<Gtk::ColorButton*>(init_widget(xml, widget_name, config_group, config_key));
   }

  protected:
   std::map<std::string, Gtk::ColorButton*> m_colorButtons;
};
