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

// The combobox display all videos on the current folder.
// Try to found the good video from the selected subtitle.
class ComboBoxVideo : public Gtk::ComboBoxText {
  public:
   // Constructor
   ComboBoxVideo(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);

   // Search all videos in the folder.
   bool set_current_folder(const Glib::ustring& path);

   // Try to select the good video from the subtitle.
   // Only if the option "automatically-open-video" is enabled.
   bool auto_select_video(const Glib::ustring& subtitle);

   // Return the video selected or a empty string.
   Glib::ustring get_value() const;

  protected:
   // Used to define the separator.
   // label = "<separator>"
   bool on_row_separator_func(const Glib::RefPtr<Gtk::TreeModel>& model, const Gtk::TreeModel::iterator& it);
};
