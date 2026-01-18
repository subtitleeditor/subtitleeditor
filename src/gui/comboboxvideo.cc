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

#include "comboboxvideo.h"

#include "cfg.h"
#include "comboboxtextcolumns.h"
#include "i18n.h"

// Constructor
ComboBoxVideo::ComboBoxVideo(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& /*builder*/) : Gtk::ComboBoxText(cobject) {
   // separator function
   Gtk::ComboBoxText::set_row_separator_func(sigc::mem_fun(*this, &ComboBoxVideo::on_row_separator_func));
}

// Search all videos in the folder.
bool ComboBoxVideo::set_current_folder(const Glib::ustring& path) {
   remove_all();

   Glib::Dir dir(path);
   std::vector<std::string> files(dir.begin(), dir.end());

   // No files in the directory, unsensitive the widget
   if (files.empty()) {
      set_sensitive(false);
      return false;
   }

   // Try to find video files
   Glib::RefPtr<Glib::Regex> re = Glib::Regex::create("^(.*)\\.((avi)|(wma)|(mkv)|(mpg)|(mpeg)|(ogg)|(mov)|(mp4)|(xvid))$");

   for (unsigned int i = 0; i < files.size(); ++i) {
      if (re->match(files[i]))
         append(files[i]);
   }

   // Not found? unsensitive the widget
   if (get_model()->children().size() == 0) {
      set_sensitive(false);
      return false;
   }

   set_sensitive(true);

   prepend("<separator>", "");
   prepend(_("None"));
   return true;
}

// Try to select the good video from the subtitle.
// Only if the option "automatically-open-video" is enabled.
bool ComboBoxVideo::auto_select_video(const Glib::ustring& subtitle) {
   if (get_model()->children().empty())
      return false;

   bool is_directory_or_empty = (Glib::file_test(subtitle, Glib::FILE_TEST_IS_DIR) || subtitle.empty());
   bool is_automatically_open_video = cfg::get_boolean("video-player", "automatically-open-video");

   if (is_directory_or_empty || is_automatically_open_video == false) {
      set_active(0);
      return false;
   }

   Glib::RefPtr<Glib::Regex> re = Glib::Regex::create("^(.*)\\.((avi)|(wma)|(mkv)|(mpg)|(mpeg)|(ogg)|(mov)|(mp4)|(xvid))$");

   Gtk::TreeIter it = get_model()->children().begin();
   ComboBoxTextColumns cols;

   for (; it; ++it) {
      Glib::ustring video = (*it)[cols.m_col_name];
      if (video.empty())
         continue;

      std::vector<Glib::ustring> group = re->split(video);
      if (group.size() == 1)
         continue;

      if (subtitle.find(group[1]) != Glib::ustring::npos) {
         set_active_text(video);
         return true;
      }
   }
   // not found, active "None"
   set_active(0);
   return false;
}

// Return the video selected or a empty string.
Glib::ustring ComboBoxVideo::get_value() const {
   if (is_sensitive() == false || get_active_row_number() == 0)
      return Glib::ustring();

   return get_active_text();
}

// Used to define the separator.
// label = "<separator>"
bool ComboBoxVideo::on_row_separator_func(const Glib::RefPtr<Gtk::TreeModel>& /*model*/, const Gtk::TreeModel::iterator& it) {
   ComboBoxTextColumns cols;
   if ((*it)[cols.m_col_id] == "<separator>")
      return true;
   return false;
}
