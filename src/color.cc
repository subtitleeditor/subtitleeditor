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

#include "color.h"

#include <iostream>

#include "utility.h"

Color::Color() {
   set(0, 0, 0, 255);
}

Color::Color(unsigned int r, unsigned int g, unsigned int b, unsigned int a) {
   set(r, g, b, a);
}

Color::Color(const Glib::ustring& color) {
   from_string(color);
}

void Color::set(unsigned int r, unsigned int g, unsigned int b, unsigned a) {
   m_rgba[0] = CLAMP(r, 0, 255);
   m_rgba[1] = CLAMP(g, 0, 255);
   m_rgba[2] = CLAMP(b, 0, 255);
   m_rgba[3] = CLAMP(a, 0, 255);
}

unsigned int Color::getR() const {
   return m_rgba[0];
}

unsigned int Color::getG() const {
   return m_rgba[1];
}

unsigned int Color::getB() const {
   return m_rgba[2];
}

unsigned int Color::getA() const {
   return m_rgba[3];
}

Glib::ustring Color::to_string() const {
   return build_message("#%02X%02X%02X%02X", m_rgba[0], m_rgba[1], m_rgba[2], m_rgba[3]);
}

bool hex(const Glib::ustring& spec, unsigned int* c) {
   *c = 0;
   for (unsigned int i = 0; i < spec.size(); ++i) {
      if (g_ascii_isxdigit(spec[i]))
         *c = (*c << 4) | g_ascii_xdigit_value(spec[i]);
      else
         return false;
   }
   return true;
}

bool Color::from_string(const Glib::ustring& color) {
   if (color[0] == '#') {
      Glib::ustring value = color.substr(1, color.size());

      size_t len;
      unsigned int r = 0, g = 0, b = 0, a = 0;

      len = value.size();

      len /= 4;

      if (!hex(value.substr(0, len), &r) || !hex(value.substr(len, len), &g) || !hex(value.substr(len * 2, len), &b) ||
          !hex(value.substr(len * 3, len), &a))
         return false;

      m_rgba[0] = r;
      m_rgba[1] = g;
      m_rgba[2] = b;
      m_rgba[3] = a;

      return true;
   } else
      std::cerr << "Color from_string FAILED: '" << color << "'" << std::endl;

   return false;
}

// init button avec les info de color
void Color::initColorButton(Gtk::ColorButton& button) {
   Gdk::Color gdkcolor;
   gdkcolor.set_rgb(getR() * 257, getG() * 257, getB() * 257);

   button.set_alpha(getA() * 257);
   button.set_color(gdkcolor);
}

// init color a partir de button
void Color::getFromColorButton(const Gtk::ColorButton& button) {
   Gdk::Color gdkcolor = button.get_color();

   unsigned int r = gdkcolor.get_red();
   unsigned int g = gdkcolor.get_green();
   unsigned int b = gdkcolor.get_blue();
   unsigned int a = button.get_alpha();

   set(r / 257, g / 257, b / 257, a / 257);
}
