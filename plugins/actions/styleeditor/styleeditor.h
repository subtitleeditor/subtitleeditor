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

#include "styles.h"

class DialogStyleEditor : public Gtk::Dialog {
  public:
   DialogStyleEditor(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);

   void execute(Document* doc);

  protected:
   void init_style(const Style& style);

   void on_style_name_edited(const Glib::ustring& path, const Glib::ustring& text);

   void callback_button_clicked(Gtk::Button* button, const Glib::ustring& action);
   void callback_font_button_changed(Gtk::FontButton* w, const Glib::ustring& key);
   void callback_button_toggled(Gtk::ToggleButton* w, const Glib::ustring& key);
   void callback_spin_value_changed(Gtk::SpinButton* w, const Glib::ustring& key);
   void callback_color_button(Gtk::ColorButton* w, const Glib::ustring& key);
   void callback_radio_toggled(Gtk::RadioButton* w, const Glib::ustring& key);
   void callback_alignment_changed(Gtk::RadioButton* w, unsigned int num);
   void callback_style_selection_changed();
   void callback_playres_changed(Gtk::SpinButton* w, const Glib::ustring& key);

  protected:
   Document* m_current_document;
   Style m_current_style;
   Glib::RefPtr<Gtk::ListStore> m_liststore;
   Gtk::TreeView* m_treeview;
   std::map<Glib::ustring, Gtk::Widget*> m_widgets;
};
