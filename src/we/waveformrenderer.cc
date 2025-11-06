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
#include "utility.h"
#include "waveformrenderer.h"

WaveformRenderer::WaveformRenderer() {
  init_default_config();
  load_config();

  cfg::signal_changed("waveform-renderer")
      .connect(sigc::mem_fun(
          *this, &WaveformRenderer::on_config_waveform_renderer_changed));
}

WaveformRenderer::~WaveformRenderer() {
}

// Initialize the default value like colors.
void WaveformRenderer::init_default_config() {
#define SET_COLOR(x, r, g, b, a) \
  x[0] = r;                      \
  x[1] = g;                      \
  x[2] = b;                      \
  x[3] = a;

  SET_COLOR(m_color_player_position, 1.0f, 1.0f, 1.0f, 1.0f);
  SET_COLOR(m_color_background, 0.3f, 0.3f, 0.3f, 1.0f);
  SET_COLOR(m_color_wave, 0.6f, 0.8f, 0.3f, 1.0f);
  SET_COLOR(m_color_wave_fill, 1.0f, 1.0f, 1.0f, 1.0f);
  SET_COLOR(m_color_subtitle, 0.6f, 0.3f, 0.1f, 0.6f);
  SET_COLOR(m_color_subtitle_selected, 0.9f, 0.5f, 0.3f, 0.6f);
  SET_COLOR(m_color_subtitle_invalid, 1.0f, 1.0f, 0.0f,
            0.8f);  // invalid time start > end
  SET_COLOR(m_color_text, 1.0f, 1.0f, 1.0f, 1.0f);
  SET_COLOR(m_color_keyframe, 0.3f, 0.6f, 1.0f, 1.0f);

#undef SET_COLOR

  m_display_time_info = false;
  m_display_subtitle_text = true;

#define check_color(key, rgba)                                  \
  if (!cfg::has_key("waveform-renderer", key)) {                \
    Color col;                                                  \
    col.set_value(rgba, 1);                                     \
    cfg::set_string("waveform-renderer", key, col.to_string()); \
  }
#define check_bool(key, value)                 \
  if (!cfg::has_key("waveform-renderer", key)) \
    cfg::set_boolean("waveform-renderer", key, value);

  check_bool("display-subtitle-text", m_display_subtitle_text);

  check_color("color-background", m_color_background);
  check_color("color-wave", m_color_wave);
  check_color("color-wave-fill", m_color_wave_fill);
  check_color("color-subtitle", m_color_subtitle);
  check_color("color-subtitle-selected", m_color_subtitle_selected);
  check_color("color-subtitle-invalid", m_color_subtitle_invalid);
  check_color("color-text", m_color_text);
  check_color("color-player-position", m_color_player_position);
  check_color("color-keyframe", m_color_keyframe);

#undef check_color
#undef check_bool
}

void WaveformRenderer::load_config() {
  m_display_subtitle_text =
      cfg::get_boolean("waveform-renderer", "display-subtitle-text");

#define get_color(key, col) \
  Color(cfg::get_string("waveform-renderer", key)).get_value(col, 1);

  get_color("color-background", m_color_background);
  get_color("color-wave", m_color_wave);
  get_color("color-wave-fill", m_color_wave_fill);
  get_color("color-subtitle", m_color_subtitle);
  get_color("color-subtitle-selected", m_color_subtitle_selected);
  get_color("color-subtitle-invalid", m_color_subtitle_invalid);
  get_color("color-text", m_color_text);
  get_color("color-player-position", m_color_player_position);
  get_color("color-keyframe", m_color_keyframe);

#undef get_color
}

void WaveformRenderer::set_waveform(const Glib::RefPtr<Waveform> &wf) {
  m_waveform = wf;

  waveform_changed();
}

// This function is call when the waveform is changed.
// Like a new Waveform.
void WaveformRenderer::waveform_changed() {
}

void WaveformRenderer::keyframes_changed() {
}

void WaveformRenderer::redraw_all() {
}

void WaveformRenderer::force_redraw_all() {
}

int WaveformRenderer::get_start_area() {
  return scrolling();
}

int WaveformRenderer::get_end_area() {
  return get_start_area() + widget()->get_width();
}

// return the time of the position in the area
// time is in msec (SubtitleTime.totalmsecs)
long WaveformRenderer::get_time_by_pos(int pos) {
  if (!m_waveform)
    return 0;
  float width = static_cast<float>(widget()->get_width()) * zoom();
  float percent = static_cast<float>(pos) / width;
  return static_cast<long>(m_waveform->get_duration() * percent);
}

// return the position of the time in the area
int WaveformRenderer::get_pos_by_time(long msec) {
  float duration = static_cast<float>(m_waveform->get_duration());
  if (duration <= 0)
    return 0;
  float percent = static_cast<float>(msec) / duration;
  float width = static_cast<float>(widget()->get_width()) * zoom();
  float pos = width * percent;
  pos = CLAMP(pos, 0, width);
  return static_cast<int>(pos);
}

// return the position in the area with scrolling support
int WaveformRenderer::get_mouse_coords(int x) {
  return get_start_area() + x;
}

long WaveformRenderer::get_mouse_time(int x) {
  return get_time_by_pos(get_mouse_coords(x));
}

sigc::signal<Document *> &WaveformRenderer::signal_document() {
  return document;
}

sigc::signal<int> &WaveformRenderer::signal_zoom() {
  return zoom;
}

sigc::signal<float> &WaveformRenderer::signal_scale() {
  return scale;
}

sigc::signal<int> &WaveformRenderer::signal_scrolling() {
  return scrolling;
}

void WaveformRenderer::on_config_waveform_renderer_changed(
    const Glib::ustring &key, const Glib::ustring &value) {
#define string_to_rgba(string, col) Color(string).get_value(col, 1)

  if ("display-subtitle-text" == key) {
    m_display_subtitle_text = utility::string_to_bool(value);
  } else if ("color-background" == key) {
    string_to_rgba(value, m_color_background);
  } else if ("color-wave" == key) {
    string_to_rgba(value, m_color_wave);
  } else if ("color-wave-fill" == key) {
    string_to_rgba(value, m_color_wave_fill);
  } else if ("color-subtitle" == key) {
    string_to_rgba(value, m_color_subtitle);
  } else if ("color-subtitle-selected" == key) {
    string_to_rgba(value, m_color_subtitle_selected);
  } else if ("color-subtitle-invalid" == key) {
    string_to_rgba(value, m_color_subtitle_invalid);
  } else if ("color-text" == key) {
    string_to_rgba(value, m_color_text);
  } else if ("color-player-position" == key) {
    string_to_rgba(value, m_color_player_position);
  } else if ("color-keyframe" == key) {
    string_to_rgba(value, m_color_keyframe);
  }

#undef string_to_rgba

  force_redraw_all();
}
