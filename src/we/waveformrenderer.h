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

#include <gtkmm.h>
#include "document.h"
#include "waveform.h"

class WaveformRenderer {
 public:
  WaveformRenderer();

  virtual ~WaveformRenderer();

  void init_default_config();

  void load_config();

  // Return the widget attached to the renderer.
  virtual Gtk::Widget* widget() = 0;

  // This function is call when the waveform is changed.
  // Like a new Waveform.
  virtual void waveform_changed();

  virtual void keyframes_changed();

  virtual void redraw_all();

  virtual void force_redraw_all();

  int get_start_area();

  int get_end_area();

  // return the time of the position in the area
  // time is in msec (SubtitleTime.totalmsecs)
  long get_time_by_pos(int pos);

  // return the position of the time in the area
  int get_pos_by_time(long msec);

  // return the position in the area with scrolling support
  int get_mouse_coords(int x);

  long get_mouse_time(int x);

  sigc::signal<Document*>& signal_document();

  sigc::signal<int>& signal_zoom();

  sigc::signal<float>& signal_scale();

  sigc::signal<int>& signal_scrolling();

  void set_waveform(const Glib::RefPtr<Waveform>& wf);

  void on_config_waveform_renderer_changed(const Glib::ustring& key,
                                           const Glib::ustring& value);

  // protected:

  Glib::RefPtr<Waveform> m_waveform;

  sigc::signal<Document*> document;
  sigc::signal<int> zoom;
  sigc::signal<float> scale;
  sigc::signal<int> scrolling;
  sigc::signal<long> player_time;  // the current time of the player

  // config
  // color = rgba [0:1]
  float m_color_background[4];
  float m_color_wave[4];
  float m_color_wave_fill[4];
  float m_color_subtitle[4];
  float m_color_subtitle_selected[4];
  float m_color_subtitle_invalid[4];  // invalid time start > end
  float m_color_text[4];              // used for time, subtitle text ...
  float m_color_player_position[4];
  float m_color_keyframe[4];

  bool m_display_subtitle_text;
  bool m_display_time_info;  // when is true display the time of the mouse
};
