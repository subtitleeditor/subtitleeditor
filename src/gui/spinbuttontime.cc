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

#include "spinbuttontime.h"
#include "utility.h"

SpinButtonTime::SpinButtonTime() {
  default_init();
  set_timing_mode(TIME);
}

SpinButtonTime::SpinButtonTime(BaseObjectType *cobject,
                               const Glib::RefPtr<Gtk::Builder> & /*builder*/)
    : Gtk::SpinButton(cobject) {
  default_init();
  set_timing_mode(TIME);
}

void SpinButtonTime::default_init() {
  m_negative = false;
  // set_alignment(1.0);
}

void SpinButtonTime::set_timing_mode(TIMING_MODE mode) {
  if (mode == FRAME)
    init_frame_mode();
  else
    init_time_mode();
}

TIMING_MODE SpinButtonTime::get_timing_mode() {
  return m_timing_mode;
}

void SpinButtonTime::set_negative(bool state) {
  m_negative = state;
  init_range();
}

void SpinButtonTime::init_frame_mode() {
  m_timing_mode = FRAME;
  set_increments(1, 1);

  init_range();
}

void SpinButtonTime::init_time_mode() {
  m_timing_mode = TIME;

  set_increments(100, 1);

  init_range();
}

int SpinButtonTime::on_input(double *new_value) {
  if (m_timing_mode == TIME) {
    Glib::ustring text = get_text();

    if (SubtitleTime::validate(text))
      *new_value = static_cast<double>(SubtitleTime(text).totalmsecs);
    else
      *new_value = get_value();

    return true;
  }

  return Gtk::SpinButton::on_input(new_value);
}

bool SpinButtonTime::on_output() {
  if (m_timing_mode == FRAME)
    return Gtk::SpinButton::on_output();

  // TIME output
  long value = (long)get_adjustment()->get_value();

  std::string sign;

  if (value < 0) {
    sign = "-";
    value = -value;
  }

  SubtitleTime time(value);

  std::string text =
      build_message("%s%01d:%02d:%02d.%03d", sign.c_str(), time.hours(),
                    time.minutes(), time.seconds(), time.mseconds());

  set_text(text);

  return true;
}

void SpinButtonTime::get_preferred_width_vfunc(int &minimum_width,
                                               int &natural_width) const {
  Gtk::SpinButton::get_preferred_width_vfunc(minimum_width, natural_width);
  if (m_timing_mode == TIME) {
    minimum_width += 30;
    natural_width += 30;
  }
}

bool SpinButtonTime::on_scroll_event(GdkEventScroll *ev) {
  double step, page;

  get_increments(step, page);

  if (ev->state & GDK_SHIFT_MASK && ev->state & GDK_CONTROL_MASK)
    step *= 100;
  else if (ev->state & GDK_CONTROL_MASK)
    step *= 10;

  if (ev->direction == GDK_SCROLL_UP) {
    set_value(get_value() + step);
  } else if (ev->direction == GDK_SCROLL_DOWN) {
    set_value(get_value() - step);
  }

  return true;
}

void SpinButtonTime::on_insert_text(const Glib::ustring &str, int *pos) {
  Gtk::SpinButton::on_insert_text(str, pos);
}

void SpinButtonTime::init_range() {
  if (m_timing_mode == TIME) {
    long max = 86399999;

    if (m_negative)
      set_range(-max, max);
    else
      set_range(0, max);
  } else {  // FRAME
    if (m_negative)
      set_range(-3000000, 3000000);
    else
      set_range(0, 3000000);
  }
}
