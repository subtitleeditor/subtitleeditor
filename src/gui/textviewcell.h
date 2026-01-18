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

// Cell text editable with multiline support.
class TextViewCell : public Gtk::CellEditable, public Gtk::TextView {
  public:
   // Constructor
   // Initialize the widget with the AutomaticSpellChecker.
   TextViewCell();

   // Destructor
   ~TextViewCell();

   // Define the current text.
   void set_text(const Glib::ustring& text);

   // Return the current text.
   Glib::ustring get_text();

  protected:
   // Check if the user cancel the editing with Escape.
   // Check if the user apply the editing with Enter (depend on user prefs).
   bool on_key_press_event(GdkEventKey* event);

   // bug #23569 : Cursor cannot be moved with mouse when editing subtitles
   bool on_button_press_event(GdkEventButton* event);

   // Before removing the widget we call editing_done
   // if there's no canceled signal.
   void on_remove_widget();

  protected:
   bool m_editing_canceled{false};
   // User preference to confirm and exit editing
   bool m_used_ctrl_enter_to_confirm_change{false};
};
