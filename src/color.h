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

// clamp [0, 255] for color
class Color {
 public:
  Color();
  Color(unsigned int r, unsigned int g, unsigned int b, unsigned int a);
  Color(const Glib::ustring &color);
  void set(unsigned int r, unsigned int g, unsigned int b, unsigned a = 255);

  unsigned int getR() const;
  unsigned int getG() const;
  unsigned int getB() const;
  unsigned int getA() const;

  template <class T>
  bool set_value(T rgba[4], unsigned int scale);

  template <class T>
  bool get_value(T rgba[4], unsigned int scale);

  // init button avec les info de color
  void initColorButton(Gtk::ColorButton &button);

  // init color a partir de button
  void getFromColorButton(const Gtk::ColorButton &button);

  Glib::ustring to_string() const;

  bool from_string(const Glib::ustring &str);

 protected:
  // unsigned int rgba;
  unsigned int m_rgba[4];
};

template <class T>
bool Color::set_value(T rgba[4], unsigned int scale) {
  m_rgba[0] = (unsigned int)(((float)rgba[0] / scale) * 255);
  m_rgba[1] = (unsigned int)(((float)rgba[1] / scale) * 255);
  m_rgba[2] = (unsigned int)(((float)rgba[2] / scale) * 255);
  m_rgba[3] = (unsigned int)(((float)rgba[3] / scale) * 255);

  return true;
}

template <class T>
bool Color::get_value(T rgba[4], unsigned int scale) {
  unsigned int r = getR();
  unsigned int g = getG();
  unsigned int b = getB();
  unsigned int a = getA();

  rgba[0] = (((float)r / 255) * scale);
  rgba[1] = (((float)g / 255) * scale);
  rgba[2] = (((float)b / 255) * scale);
  rgba[3] = (((float)a / 255) * scale);
  return true;
}
