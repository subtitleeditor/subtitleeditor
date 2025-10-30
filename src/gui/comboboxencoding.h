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

// ComboBox for choosing the encoding.
// Only the encoding preferences are displayed.
// The support of "Auto Detected" is enable by default.
class ComboBoxEncoding : public Gtk::ComboBoxText {
 public:
  // Constructor
  explicit ComboBoxEncoding(bool auto_detected);

  // Constructor
  ComboBoxEncoding(BaseObjectType *cobject,
                   const Glib::RefPtr<Gtk::Builder> &builder);

  // Sets the current value.
  void set_value(const Glib::ustring &charset);

  // Returns only the charset value.
  // ex: "UTF-8", "ISO-8859-15" ...
  // Return empty charset if it's "Auto Detected".
  Glib::ustring get_value() const;

  // Enable or disable the auto detected mode.
  void show_auto_detected(bool value);

 protected:
  // Rebuild the combobox with encoding user preferences.
  void init_encodings();

  // Gtk::ComboBox::on_changed
  // Used for intercepte "Add or Remove..."
  void on_combo_changed();

  // Used to define the separator.
  // label = "<separator>"
  bool on_row_separator_func(const Glib::RefPtr<Gtk::TreeModel> &model,
                             const Gtk::TreeModel::iterator &it);

 protected:
  bool m_with_auto_detected;
  sigc::connection m_connection_changed;
};
