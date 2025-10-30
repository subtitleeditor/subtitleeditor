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

#include "comboboxframerate.h"

// Constructor
ComboBoxFramerate::ComboBoxFramerate() : ComboBox() {
  liststore = Gtk::ListStore::create(column);
  set_model(liststore);

  Gtk::CellRendererText* renderer = manage(new Gtk::CellRendererText);
  pack_start(*renderer);
  add_attribute(*renderer, "text", 0);

  liststore->set_sort_column(0, Gtk::SORT_ASCENDING);

  append(FRAMERATE_23_976);
  append(FRAMERATE_24);
  append(FRAMERATE_25, " (PAL)");
  append(FRAMERATE_29_97, " (NTSC)");
  append(FRAMERATE_30);

  set_active(0);
}

// Return the current framerate value
FRAMERATE ComboBoxFramerate::get_value() {
  Gtk::TreeIter it = get_active();
  return (*it)[column.value];
}

// Set the current framerate value
void ComboBoxFramerate::set_value(FRAMERATE value) {
  Gtk::TreeIter it = get_model()->children().begin();
  while (it) {
    FRAMERATE framerate = (*it)[column.value];
    if (framerate == value) {
      set_active(it);
      return;
    }
    ++it;
  }
}

// Add a new item
void ComboBoxFramerate::append(FRAMERATE framerate, const Glib::ustring& text) {
  Gtk::TreeIter it = liststore->append();
  (*it)[column.label] = get_framerate_label(framerate) + text;
  (*it)[column.value] = framerate;
}
