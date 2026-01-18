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

#include "comboboxencoding.h"

#include "cfg.h"
#include "comboboxtextcolumns.h"
#include "dialogcharactercodings.h"
#include "encodings.h"

// Constructor
ComboBoxEncoding::ComboBoxEncoding(bool auto_detected) : m_with_auto_detected(auto_detected) {
   init_encodings();

   // separator function
   Gtk::ComboBoxText::set_row_separator_func(sigc::mem_fun(*this, &ComboBoxEncoding::on_row_separator_func));

   // m_connection_changed is need to disable the signal when the combobox is
   // rebuild.
   m_connection_changed = signal_changed().connect(sigc::mem_fun(*this, &ComboBoxEncoding::on_combo_changed));
}

// Constructor
ComboBoxEncoding::ComboBoxEncoding(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>&)
    : Gtk::ComboBoxText(cobject), m_with_auto_detected(true) {
   init_encodings();

   // separator function
   Gtk::ComboBoxText::set_row_separator_func(sigc::mem_fun(*this, &ComboBoxEncoding::on_row_separator_func));

   // m_connection_changed is need to disable the signal when the combobox is
   // rebuild.
   m_connection_changed = signal_changed().connect(sigc::mem_fun(*this, &ComboBoxEncoding::on_combo_changed));
}

// Sets current value.
void ComboBoxEncoding::set_value(const Glib::ustring& value) {
   Glib::ustring label = Encodings::get_label_from_charset(value);

   if (label.empty())
      return;
   set_active_text(label);
}

// Returns only the charset value.
// ex: "UTF-8", "ISO-8859-15" ...
// Return empty charset if it's "Auto Detected".
Glib::ustring ComboBoxEncoding::get_value() const {
   return get_active_id();
}

// Enable or disable the auto detected mode.
void ComboBoxEncoding::show_auto_detected(bool value) {
   m_with_auto_detected = value;

   bool state = is_sensitive();
   set_sensitive(false);

   init_encodings();

   set_sensitive(state);
}

// Rebuild the combobox with encoding user preferences.
void ComboBoxEncoding::init_encodings() {
   m_connection_changed.block();

   remove_all();

   // Setup auto_detected
   bool used_auto_detected = cfg::get_boolean("encodings", "used-auto-detected");

   if (m_with_auto_detected) {
      append(_("Auto Detected"));
      append("<separator>", "");
   }

   // Setup charsets
   auto encodings = cfg::get_string_list("encodings", "encodings");
   if (!encodings.empty()) {
      for (const auto& enc : encodings) {
         append(enc, Encodings::get_label_from_charset(enc));
      }
   } else {
      std::string charset;
      Glib::get_charset(charset);

      Glib::ustring item;
      item += _("Current Locale");
      item += " (" + charset + ")";

      Glib::ustring id = charset;
      append(id, item);
   }

   // Setup configure
   append("<separator>", "");
   append(_("Add or Remove..."));

   if (m_with_auto_detected) {
      if (used_auto_detected)
         set_active(0);
      else
         set_active(2);  // auto detected (0), separator (1), first charset (2)
   } else {
      set_active(0);
   }
   m_connection_changed.unblock();
}

// Gtk::ComboBox::on_changed
// Used for intercepte "Add or Remove..."
void ComboBoxEncoding::on_combo_changed() {
   unsigned int size = get_model()->children().size();
   unsigned int activated = get_active_row_number();

   if (activated == size - 1) {
      std::unique_ptr<DialogCharacterCodings> dialog = DialogCharacterCodings::create(*dynamic_cast<Gtk::Window*>(get_toplevel()));
      if (dialog->run() == Gtk::RESPONSE_OK) {
         init_encodings();
      } else if (m_with_auto_detected) {
         if (cfg::get_boolean("encodings", "used-auto-detected")) {
            set_active(0);
         } else {
            set_active(2);
         }
      } else
         set_active(0);
   }
}

// Used to define the separator.
// label = "<separator>"

bool ComboBoxEncoding::on_row_separator_func(const Glib::RefPtr<Gtk::TreeModel>& /*model*/, const Gtk::TreeModel::iterator& it) {
   ComboBoxTextColumns cols;
   if ((*it)[cols.m_col_id] == "<separator>")
      return true;
   return false;
}
