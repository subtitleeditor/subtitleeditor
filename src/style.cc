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

#include <iostream>
#include "document.h"
#include "style.h"
#include "utility.h"

// static
StyleColumnRecorder Style::column;

Style::Style() {
}

Style::Style(Document *doc, const Gtk::TreeIter &iter)
    : m_document(doc), m_iter(iter) {
}

Style::~Style() {
}

Style::operator bool() const {
  return (m_iter) ? true : false;
}

Style &Style::operator++() {
  ++m_iter;
  // m_path =
  return *this;
}

Style &Style::operator--() {
  --m_iter;
  // m_path =
  return *this;
}

bool Style::operator==(const Style &style) const {
  return m_iter == style.m_iter;
}

bool Style::operator!=(const Style &style) const {
  return m_iter != style.m_iter;
}

void Style::set(const Glib::ustring &name, const Glib::ustring &value) {
  g_return_if_fail(m_iter);

  if (name == "name")
    (*m_iter)[column.name] = value;
  else if (name == "font-name")
    (*m_iter)[column.font_name] = value;
  else if (name == "font-size")
    (*m_iter)[column.font_size] = utility::string_to_double(value);
  else if (name == "primary-color")
    (*m_iter)[column.primary_colour] = value;
  else if (name == "secondary-color")
    (*m_iter)[column.secondary_colour] = value;
  else if (name == "outline-color")
    (*m_iter)[column.outline_colour] = value;
  else if (name == "shadow-color")
    (*m_iter)[column.shadow_colour] = value;
  else if (name == "bold")
    (*m_iter)[column.bold] = utility::string_to_bool(value);
  else if (name == "italic")
    (*m_iter)[column.italic] = utility::string_to_bool(value);
  else if (name == "underline")
    (*m_iter)[column.underline] = utility::string_to_bool(value);
  else if (name == "strikeout")
    (*m_iter)[column.strikeout] = utility::string_to_bool(value);
  else if (name == "scale-x")
    (*m_iter)[column.scale_x] = utility::string_to_int(value);
  else if (name == "scale-y")
    (*m_iter)[column.scale_y] = utility::string_to_int(value);
  else if (name == "spacing")
    (*m_iter)[column.spacing] = utility::string_to_int(value);
  else if (name == "angle")
    (*m_iter)[column.angle] = utility::string_to_int(value);
  else if (name == "margin-r")
    (*m_iter)[column.margin_r] = utility::string_to_int(value);
  else if (name == "margin-l")
    (*m_iter)[column.margin_l] = utility::string_to_int(value);
  else if (name == "margin-v")
    (*m_iter)[column.margin_v] = utility::string_to_int(value);
  else if (name == "alignment")
    (*m_iter)[column.alignment] = utility::string_to_int(value);
  else if (name == "border-style")
    (*m_iter)[column.border_style] = utility::string_to_int(value);
  else if (name == "outline")
    (*m_iter)[column.outline] = utility::string_to_int(value);
  else if (name == "shadow")
    (*m_iter)[column.shadow] = utility::string_to_int(value);
  else if (name == "encoding")
    (*m_iter)[column.encoding] = utility::string_to_int(value);
  else
    std::cerr << "Style::set " << name << " UNKNOWN" << std::endl;

  m_document->emit_signal("style-changed");

}

Glib::ustring Style::get(const Glib::ustring &name) const {
  g_return_val_if_fail(m_iter, "");

  if (name == "name")
    return (*m_iter)[column.name];
  else if (name == "font-name")
    return (*m_iter)[column.font_name];
  else if (name == "font-size")
    return to_string((*m_iter)[column.font_size]);
  else if (name == "primary-color")
    return Color((*m_iter)[column.primary_colour]).to_string();
  else if (name == "secondary-color")
    return Color((*m_iter)[column.secondary_colour]).to_string();
  else if (name == "outline-color")
    return Color((*m_iter)[column.outline_colour]).to_string();
  else if (name == "shadow-color")
    return Color((*m_iter)[column.shadow_colour]).to_string();
  else if (name == "bold")
    return to_string((*m_iter)[column.bold]);
  else if (name == "italic")
    return to_string((*m_iter)[column.italic]);
  else if (name == "underline")
    return to_string((*m_iter)[column.underline]);
  else if (name == "strikeout")
    return to_string((*m_iter)[column.strikeout]);
  else if (name == "scale-x")
    return to_string((*m_iter)[column.scale_x]);
  else if (name == "scale-y")
    return to_string((*m_iter)[column.scale_y]);
  else if (name == "spacing")
    return to_string((*m_iter)[column.spacing]);
  else if (name == "angle")
    return to_string((*m_iter)[column.angle]);
  else if (name == "margin-r")
    return to_string((*m_iter)[column.margin_r]);
  else if (name == "margin-l")
    return to_string((*m_iter)[column.margin_l]);
  else if (name == "margin-v")
    return to_string((*m_iter)[column.margin_v]);
  else if (name == "alignment")
    return to_string((*m_iter)[column.alignment]);
  else if (name == "border-style")
    return to_string((*m_iter)[column.border_style]);
  else if (name == "outline")
    return to_string((*m_iter)[column.outline]);
  else if (name == "shadow")
    return to_string((*m_iter)[column.shadow]);
  else if (name == "encoding")
    return to_string((*m_iter)[column.encoding]);
  else
    std::cerr << "Style::get: " << name << " UNKNOWN!" << std::endl;

  return Glib::ustring();
}

void Style::copy_to(Style &style) {
  g_return_if_fail(m_iter);

  std::map<Glib::ustring, Glib::ustring> values;

  get(values);

  style.set(values);
}

void Style::set(const std::map<Glib::ustring, Glib::ustring> &values) {
  g_return_if_fail(m_iter);

  for (const auto &value : values) {
    set(value.first, value.second);
  }
}

void Style::get(std::map<Glib::ustring, Glib::ustring> &values) {
#define set_value(name) values[name] = get(name);

  set_value("name");
  set_value("font-name");
  set_value("font-size");

  set_value("primary-color");
  set_value("secondary-color");
  set_value("outline-color");
  set_value("shadow-color");

  set_value("bold");
  set_value("italic");
  set_value("underline");
  set_value("strikeout");

  set_value("scale-x");
  set_value("scale-y");
  set_value("spacing");
  set_value("angle");

  set_value("margin-r");
  set_value("margin-l");
  set_value("margin-v");

  set_value("alignment");
  set_value("border-style");
  set_value("outline");
  set_value("shadow");

#undef set_value
}
