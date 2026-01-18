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

#include "dialogcharactercodings.h"

#include "cfg.h"
#include "encodings.h"
#include "gtkmm_utility.h"
#include "utility.h"

DialogCharacterCodings::DialogCharacterCodings(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder) : Gtk::Dialog(cobject) {
   builder->get_widget("treeview-available", treeviewAvailable);
   builder->get_widget("treeview-displayed", m_treeviewDisplayed);
   builder->get_widget("button-add", m_buttonAdd);
   builder->get_widget("button-remove", m_buttonRemove);

   init_encodings_available();
   init_encodings_displayed();

   m_buttonAdd->signal_clicked().connect(sigc::mem_fun(*this, &DialogCharacterCodings::on_button_add));
   m_buttonRemove->signal_clicked().connect(sigc::mem_fun(*this, &DialogCharacterCodings::on_button_remove));

   // add dialog buttons
   add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
   add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);

   set_default_response(Gtk::RESPONSE_OK);
}

// Create the columns "Description" and "Encoding".
void DialogCharacterCodings::create_columns(Gtk::TreeView* view, bool clickable) {
   Gtk::TreeViewColumn* column = NULL;
   Gtk::CellRendererText* description = NULL;
   Gtk::CellRendererText* charset = NULL;

   // column description
   column = manage(new Gtk::TreeViewColumn(_("_Description")));
   view->append_column(*column);

   description = manage(new Gtk::CellRendererText);
   column->pack_start(*description);
   column->add_attribute(description->property_text(), m_column.description);

   if (clickable) {
      column->set_clickable(true);
      column->set_sort_column(m_column.description);
   }

   // column encoding
   column = manage(new Gtk::TreeViewColumn(_("_Encoding")));
   view->append_column(*column);

   charset = manage(new Gtk::CellRendererText);
   column->pack_start(*charset);
   column->add_attribute(charset->property_text(), m_column.charset);

   if (clickable) {
      column->set_clickable(true);
      column->set_sort_column(m_column.charset);
   }
}

// Append encoding to the model.
// Sets description and charset from Encodings.
void DialogCharacterCodings::append_encoding(Glib::RefPtr<Gtk::ListStore> store, const Glib::ustring& charset) {
   EncodingInfo* info = Encodings::get_from_charset(charset);
   if (info == NULL)
      return;

   Gtk::TreeIter it = store->append();

   (*it)[m_column.description] = info->name;
   (*it)[m_column.charset] = info->charset;
}

// Return true if the charset is already in the Displayed list.
bool DialogCharacterCodings::check_if_already_display(const Glib::ustring& charset) {
   Gtk::TreeIter it = m_storeDisplayed->children().begin();
   while (it) {
      if ((*it)[m_column.charset] == charset)
         return true;
      ++it;
   }
   return false;
}

// Init the available treeview with all encodings.
void DialogCharacterCodings::init_encodings_available() {
   create_columns(treeviewAvailable, true);

   m_storeAvailable = Gtk::ListStore::create(m_column);

   EncodingInfo* ei = Encodings::get_encodings_info();

   for (unsigned int i = 0; ei[i].charset != NULL; ++i) {
      append_encoding(m_storeAvailable, ei[i].charset);
   }

   m_storeAvailable->set_sort_column(m_column.description, Gtk::SORT_ASCENDING);
   treeviewAvailable->set_model(m_storeAvailable);

   treeviewAvailable->get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);

   treeviewAvailable->get_selection()->signal_changed().connect(
      sigc::mem_fun(*this, &DialogCharacterCodings::on_encodings_available_selection_changed));

   treeviewAvailable->signal_row_activated().connect(sigc::mem_fun(*this, &DialogCharacterCodings::on_row_available_activated));

   on_encodings_available_selection_changed();
}

// Init the displayed treeview with the config.
void DialogCharacterCodings::init_encodings_displayed() {
   create_columns(m_treeviewDisplayed, false);

   m_storeDisplayed = Gtk::ListStore::create(m_column);

   auto encodings = cfg::get_string_list("encodings", "encodings");
   for (const auto& encoding : encodings) {
      append_encoding(m_storeDisplayed, encoding);
   }

   m_treeviewDisplayed->set_model(m_storeDisplayed);
   m_treeviewDisplayed->get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);

   m_treeviewDisplayed->get_selection()->signal_changed().connect(
      sigc::mem_fun(*this, &DialogCharacterCodings::on_encodings_displayed_selection_changed));

   m_treeviewDisplayed->signal_row_activated().connect(sigc::mem_fun(*this, &DialogCharacterCodings::on_row_displayed_activated));

   on_encodings_displayed_selection_changed();
}

// Add character codings selected from Available to the Displayed.
void DialogCharacterCodings::on_button_add() {
   auto selected_rows = treeviewAvailable->get_selection()->get_selected_rows();

   if (selected_rows.empty())
      return;

   for (const auto& row : selected_rows) {
      Gtk::TreeIter it = treeviewAvailable->get_model()->get_iter(row);
      if (it) {
         // only once
         if (check_if_already_display((*it)[m_column.charset]) == false)
            append_encoding(m_storeDisplayed, (*it)[m_column.charset]);
      }
   }
}

// Remove selected items to the displayed treeview.
void DialogCharacterCodings::on_button_remove() {
   auto rows = m_treeviewDisplayed->get_selection()->get_selected_rows();

   for (auto iter = rows.rbegin(); iter != rows.rend(); ++iter) {
      Gtk::TreeIter it = m_storeDisplayed->get_iter(*iter);
      m_storeDisplayed->erase(it);
   }
}

// Update the sensitive of the "add" button.
void DialogCharacterCodings::on_encodings_available_selection_changed() {
   int count = treeviewAvailable->get_selection()->count_selected_rows();

   m_buttonAdd->set_sensitive(count > 0);
}

// Update the sensitive of the "remove" button.
void DialogCharacterCodings::on_encodings_displayed_selection_changed() {
   int count = m_treeviewDisplayed->get_selection()->count_selected_rows();

   m_buttonRemove->set_sensitive(count > 0);
}

// Save the values in the config.
void DialogCharacterCodings::save_config() {
   Gtk::TreeIter it = m_storeDisplayed->children().begin();

   std::vector<Glib::ustring> encodings;
   while (it) {
      encodings.push_back((*it)[m_column.charset]);
      ++it;
   }

   cfg::set_string_list("encodings", "encodings", encodings);
}

// if the response is RESPONSE_OK save the config.
void DialogCharacterCodings::on_response(int id) {
   if (id == Gtk::RESPONSE_OK)
      save_config();
}

// Add the selected charset.
void DialogCharacterCodings::on_row_available_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* /*column*/) {
   Gtk::TreeIter it = m_storeAvailable->get_iter(path);
   if (it) {
      // only once
      if (check_if_already_display((*it)[m_column.charset]) == false)
         append_encoding(m_storeDisplayed, (*it)[m_column.charset]);
   }
}

// Remove the selected charset.
void DialogCharacterCodings::on_row_displayed_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* /*column*/) {
   Gtk::TreeIter it = m_storeDisplayed->get_iter(path);
   if (it)
      m_storeDisplayed->erase(it);
}

// Create an instance of the dialog .ui file)
// If the response is OK the config is saved.
std::unique_ptr<DialogCharacterCodings> DialogCharacterCodings::create(Gtk::Window& parent) {
   std::unique_ptr<DialogCharacterCodings> ptr(gtkmm_utility::get_widget_derived<DialogCharacterCodings>(
      SE_DEV_VALUE(PACKAGE_UI_DIR, PACKAGE_UI_DIR_DEV), "dialog-character-codings.ui", "dialog-character-codings"));
   ptr->set_transient_for(parent);
   return ptr;
}
