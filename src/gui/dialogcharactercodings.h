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

#include <memory>

class DialogCharacterCodings : public Gtk::Dialog {
   class ColumnEncoding : public Gtk::TreeModel::ColumnRecord {
     public:
      ColumnEncoding() {
         add(description);
         add(charset);
      }

      Gtk::TreeModelColumn<Glib::ustring> description;
      Gtk::TreeModelColumn<Glib::ustring> charset;
   };

  public:
   DialogCharacterCodings(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);

   // Create an instance of the dialog .ui file)
   // If the response is OK the config is saved.
   static std::unique_ptr<DialogCharacterCodings> create(Gtk::Window& parent);

  protected:
   // Create the columns "Description" and "Encoding".
   void create_columns(Gtk::TreeView* view, bool clickable);

   // Append encoding to the model.
   // Sets description and charset from Encodings.
   void append_encoding(Glib::RefPtr<Gtk::ListStore> store, const Glib::ustring& charset);

   // Return true if the charset is already in the Displayed list.
   bool check_if_already_display(const Glib::ustring& charset);

   // Init the available treeview with all encodings.
   void init_encodings_available();

   // Init the displayed treeview with the config.
   void init_encodings_displayed();

   // Add character codings selected from Available to the Displayed.
   void on_button_add();

   // Remove selected items to the displayed treeview.
   void on_button_remove();

   // Update the sensitive of the "add" button.
   void on_encodings_available_selection_changed();

   // Update the sensitive of the "remove" button.
   void on_encodings_displayed_selection_changed();

   // Add the selected charset.
   void on_row_available_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);

   // Remove the selected charset.
   void on_row_displayed_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);

   // Save the values in the config.
   void save_config();

   // if the response is RESPONSE_OK save the config.
   virtual void on_response(int id);

  protected:
   ColumnEncoding m_column;

   Gtk::TreeView* treeviewAvailable;
   Glib::RefPtr<Gtk::ListStore> m_storeAvailable;

   Gtk::TreeView* m_treeviewDisplayed;
   Glib::RefPtr<Gtk::ListStore> m_storeDisplayed;

   Gtk::Button* m_buttonAdd;
   Gtk::Button* m_buttonRemove;
};
