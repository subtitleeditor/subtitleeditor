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

#include "comboboxnewline.h"

// Constructor
ComboBoxNewLine::ComboBoxNewLine(BaseObjectType* cobject,
                                 const Glib::RefPtr<Gtk::Builder>& /*builder*/)
    : Gtk::ComboBoxText(cobject) {
  append("Macintosh");
  append("Unix");
  append("Windows");

  set_active_text("Unix");
}

void ComboBoxNewLine::set_value(const Glib::ustring& value) {
  set_active_text(value);
}

// Returns the NewLine type.
// Windows or Unix.
Glib::ustring ComboBoxNewLine::get_value() const {
  return get_active_text();
}
