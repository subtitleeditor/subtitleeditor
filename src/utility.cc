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

#include "utility.h"

#include <glibmm.h>
#include <gtkmm.h>

#include <iostream>
#include <sstream>
#include <string>

#include "cfg.h"
#include "color.h"
#include "scriptinfo.h"
#include "style.h"
#include "subtitleeditorwindow.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// profile name use by config dir
Glib::ustring static_profile_name = "default";

Glib::ustring build_message(const char* format, ...) {
   Glib::ustring res;

   va_list args;
   char* formatted = nullptr;

   va_start(args, format);
   formatted = g_strdup_vprintf(format, args);
   va_end(args);

   res = formatted;

   g_free(formatted);

   return res;
}

// the profile name for the config dir
// ~/config/subtitleeditor/{profile}
void set_profile_name(const Glib::ustring& profile) {
   se_dbg_msg(SE_DBG_UTILITY, "profile=%s", profile.c_str());

   if (!profile.empty())
      static_profile_name = profile;
}

// ~/.config/subtitleeditor/{profile}/
// XDG Base Directory Specification
Glib::ustring get_config_dir(const Glib::ustring& file) {
   const gchar* configdir = g_get_user_config_dir();

   Glib::ustring path = Glib::build_filename(configdir, "subtitleeditor");

   // create config path if need
   if (Glib::file_test(path, Glib::FILE_TEST_IS_DIR) == false) {
      // g_mkdir(path.c_str(), 0700);
      Glib::spawn_command_line_sync("mkdir " + path);
   }

   // create profile path if need
   path = Glib::build_filename(path, static_profile_name);

   if (Glib::file_test(path, Glib::FILE_TEST_IS_DIR) == false) {
      Glib::spawn_command_line_sync("mkdir " + path);
   }

   return Glib::build_filename(path, file);
}

void dialog_warning(const Glib::ustring& primary_text, const Glib::ustring& secondary_text) {
   Glib::ustring msg;

   msg += "<span weight=\"bold\" size=\"larger\">";
   msg += primary_text;
   msg += "</span>\n\n";
   msg += secondary_text;

   Gtk::MessageDialog dialog(msg, true, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true);
   dialog.run();
   // MessageDialog *dialog = new MessageDialog(msg, Gtk::MESSAGE_WARNING);
}

void dialog_error(const Glib::ustring& primary_text, const Glib::ustring& secondary_text) {
   Glib::ustring msg;

   msg += "<span weight=\"bold\" size=\"larger\">";
   msg += primary_text;
   msg += "</span>\n\n";
   msg += secondary_text;

   Gtk::MessageDialog dialog(msg, true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
   dialog.run();
   // MessageDialog *dialog = new MessageDialog(msg, Gtk::MESSAGE_ERROR);
}

namespace utility {

bool string_to_bool(const std::string& str) {
   std::istringstream s(str);
   bool val = false;
   s >> val;
   return val;
}

int string_to_int(const std::string& str) {
   std::istringstream s(str);
   int val = 0;
   s >> val;
   return val;
}

int string_to_long(const std::string& str) {
   std::istringstream s(str);
   long val = 0;
   s >> val;
   return val;
}

double string_to_double(const std::string& str) {
   std::istringstream s(str);
   double val = 0;
   s >> val;
   return val;
}

void split(const std::string& str, const char& c, std::vector<std::string>& array, int max) {
   array.clear();

   std::istringstream iss(str);
   std::string word;

   if (max > 0) {
      int count = 1;
      while (std::getline(iss, word, (count < max) ? c : '\n')) {
         // std::cout << "word:" << word << std::endl;
         array.push_back(word);

         ++count;
      }
   } else {
      while (std::getline(iss, word, c)) {
         // std::cout << "word:" << word << std::endl;
         array.push_back(word);
      }
   }
}

void usplit(const Glib::ustring& str, const Glib::ustring::value_type& delimiter, std::vector<Glib::ustring>& container) {
   Glib::ustring::const_iterator it = str.begin(), end = str.end(), first;

   for (first = it; it != end; ++it) {
      if (delimiter == *it) {
         if (first != it) {  // || keep_blank)
            // extract the current field from the string
            container.push_back(Glib::ustring(first, it));
            // skip the next delimiter
            first = it;
            ++first;
         } else {
            ++first;
         }
      }
   }

   if (first != it) {  // || keep_blank
      // extract the last field from the string
      container.push_back(Glib::ustring(first, it));
   }
}

// Search and replace function.
void replace(Glib::ustring& text, const Glib::ustring& pattern, const Glib::ustring& replace_by) {
   Glib::ustring::size_type pos = 0;

   while ((pos = text.find(pattern, pos)) != Glib::ustring::npos) {
      text.replace(pos, pattern.size(), replace_by);
      pos = pos + replace_by.size();
   }
}

// Search and replace function.
void replace(std::string& text, const std::string& pattern, const std::string& replace_by) {
   std::string::size_type pos = 0;

   while ((pos = text.find(pattern, pos)) != std::string::npos) {
      text.replace(pos, pattern.size(), replace_by);
      pos = pos + replace_by.size();
   }
}

// transforme test/file.srt en /home/toto/test/file.srt
Glib::ustring create_full_path(const Glib::ustring& _path) {
   if (_path.empty())
      return Glib::ustring();

   if (Glib::path_is_absolute(_path))
      return _path;

   Glib::ustring path = _path;

   // remove ./
   {
      Glib::ustring str("./");
      if (path.compare(0, str.length(), str) == 0)
         path.replace(0, str.length(), "");
   }

   Glib::ustring curdir = Glib::get_current_dir();

   Glib::ustring newpath = Glib::build_filename(curdir, path);

   return newpath;
}

// Get the number of characters per second.
// msec = SubtitleTime::totalmsecs
double get_characters_per_second(const Glib::ustring& text, const long msecs) {
   if (msecs == 0)
      return 0;

   unsigned int len = get_text_length_for_timing(text);

   if (len == 0)
      return 0;

   auto l = static_cast<double>(len);
   auto m = static_cast<double>(msecs);

   double cps = (l * 1000.0) / m;

   return cps;
}

// Count characters in a subtitle the way they need to be counted
// for subtitle timing.
unsigned int get_text_length_for_timing(const Glib::ustring& text) {
   std::vector<int> num_characters = utility::get_characters_per_line(text);

   if (num_characters.size() == 0)
      return 0;

   unsigned int len = 0;

   for (const auto& number : num_characters) {
      len += number;
   }

   len += 2 * (num_characters.size() - 1);  // a newline counts as 2 characters
   return len;
}

// Calculate the minimum acceptable duration for a string of this length.
unsigned long get_min_duration_msecs(unsigned long textlen, double maxcps) {
   if (maxcps > 0) {
      auto tl = static_cast<double>(textlen);
      auto min = ceil((1000 * tl) / maxcps);
      return static_cast<unsigned long>(min);
   }
   return 0;
}

// Calculate the minimum acceptable duration for a string of this length.
unsigned long get_min_duration_msecs(const Glib::ustring& text, double maxcps) {
   return utility::get_min_duration_msecs((unsigned long)get_text_length_for_timing(text), maxcps);
}

// get number of characters for each line in the text
std::vector<int> get_characters_per_line(const Glib::ustring& text) {
   std::vector<int> num_characters;
   std::istringstream iss(utility::get_stripped_text(text));
   std::string line;

   while (std::getline(iss, line)) {
      Glib::ustring::size_type len = reinterpret_cast<Glib::ustring&>(line).size();
      num_characters.push_back(len);
   }

   return num_characters;
}

// get a text stripped from tags
Glib::ustring get_stripped_text(const Glib::ustring& text) {
   // pattern for tags like <i>, </i>, {\comment}, etc.
   // or space
   static bool ignore_space = cfg::get_boolean("timing", "ignore-space");
   static Glib::RefPtr<Glib::Regex> tag_pattern = ignore_space ? Glib::Regex::create("<.*?>|{.*?}| ") : Glib::Regex::create("<.*?>|{.*?}");

   return tag_pattern->replace(text, 0, "", static_cast<Glib::RegexMatchFlags>(0));
}

void set_transient_parent(Gtk::Window& window) {
   Gtk::Window* root = dynamic_cast<Gtk::Window*>(SubtitleEditorWindow::get_instance());
   if (root)
      window.set_transient_for(*root);
}

Glib::ustring add_or_replace_extension(const Glib::ustring& filename, const Glib::ustring& extension) {
   Glib::ustring renamed;
   Glib::RefPtr<Glib::Regex> re = Glib::Regex::create("^(.*)(\\.)(.*)$");
   if (re->match(filename)) {
      renamed = re->replace(filename, 0, "\\1." + extension, Glib::RegexMatchFlags(0));
   } else {
      renamed = filename + "." + extension;
   }
   return renamed;
}

}  // namespace utility

namespace ASS {
// Convert bool from ASS to SE
// ASS: 0 == false, -1 == true
Glib::ustring from_ass_bool(const Glib::ustring& value) {
   return (value == "0") ? "0" : "1";
}

// Convert color from ASS to SE
Glib::ustring from_ass_color(const Glib::ustring& str) {
   try {
      Glib::ustring value = str;

      if (value.size() > 2) {
         if (value[0] == '&')
            value.erase(0, 1);
         if (value[0] == 'h' || value[0] == 'H')
            value.erase(0, 1);
         if (value[value.size()] == '&')
            value.erase(value.size() - 1, 1);
      }

      long temp[4] = {0, 0, 0, 0};

      for (int i = 0; i < 4; ++i) {
         if (value.size() > 0) {
            Glib::ustring tmp = value.substr(value.size() - 2, 2);

            temp[i] = strtoll(tmp.c_str(), NULL, 16);

            value = value.substr(0, value.size() - 2);
         }
      }
      return Color(static_cast<unsigned int>(temp[0]),
                   static_cast<unsigned int>(temp[1]),
                   static_cast<unsigned int>(temp[2]),
                   static_cast<unsigned int>(255 - temp[3]))
         .to_string();
   } catch (...) {
   }

   return Color(255, 255, 255, 255).to_string();
}

// Convert bool from SE to ASS
// ASS: false == 0, true == -1
Glib::ustring to_ass_bool(const Glib::ustring& value) {
   return (value == "0") ? "0" : "-1";
}

// Convert color from SE to ASS
Glib::ustring to_ass_color(const Color& color) {
   Color c(color);

   unsigned int r = c.getR();
   unsigned int g = c.getG();
   unsigned int b = c.getB();
   unsigned int a = 255 - c.getA();

   unsigned int abgr = a << 24 | b << 16 | g << 8 | r << 0;

   return build_message("&H%08X", abgr);
}

// Returns style written as string, like for example this:
// Default,Sans,40,&H00FFFFFF,&H00FFFFFF,&H00FFFFFF,&H00FFFFFF,0,0,0,0,100,100,0,0,1,0,0,20,20,20,20,0
Glib::ustring style_to_string(const Style& style) {
   Glib::ustring style_string = Glib::ustring::compose(
      "%1,%2,%3,%4,%5,%6,%7",
      Glib::ustring::compose("%1,%2,%3", style.get("name"), style.get("font-name"), style.get("font-size")),
      Glib::ustring::compose("%1,%2,%3,%4",
                             to_ass_color(style.get("primary-color")),
                             to_ass_color(style.get("secondary-color")),
                             to_ass_color(style.get("outline-color")),
                             to_ass_color(style.get("shadow-color"))),
      Glib::ustring::compose("%1,%2,%3,%4",
                             to_ass_bool(style.get("bold")),
                             to_ass_bool(style.get("italic")),
                             to_ass_bool(style.get("underline")),
                             to_ass_bool(style.get("strikeout"))),
      Glib::ustring::compose("%1,%2,%3,%4", style.get("scale-x"), style.get("scale-y"), style.get("spacing"), style.get("angle")),
      Glib::ustring::compose("%1,%2,%3,%4", style.get("border-style"), style.get("outline"), style.get("shadow"), style.get("alignment")),
      Glib::ustring::compose("%1,%2,%3", style.get("margin-l"), style.get("margin-r"), style.get("margin-v")),
      style.get("encoding"));

   return style_string;
}

// Sets style from string (typically something like
// Default,Sans,18,&H00FFFFFF,&H0000FFFF,&H000078B4,&H00000000,0,0,0,0,100,100,0,0,1,0,0,2,20,20,20,0
// expects the first member of the vector of strings to be empty
void set_style_from_string(Style& style, std::vector<Glib::ustring> group) {
   style.set("name", group[1]);

   style.set("font-name", group[2]);
   style.set("font-size", group[3]);

   style.set("primary-color", ASS::from_ass_color(group[4]));
   style.set("secondary-color", ASS::from_ass_color(group[5]));
   style.set("outline-color", ASS::from_ass_color(group[6]));
   style.set("shadow-color", ASS::from_ass_color(group[7]));

   style.set("bold", ASS::from_ass_bool(group[8]));
   style.set("italic", ASS::from_ass_bool(group[9]));
   style.set("underline", ASS::from_ass_bool(group[10]));
   style.set("strikeout", ASS::from_ass_bool(group[11]));

   style.set("scale-x", group[12]);
   style.set("scale-y", group[13]);

   style.set("spacing", group[14]);
   style.set("angle", group[15]);

   style.set("border-style", group[16]);
   style.set("outline", group[17]);
   style.set("shadow", group[18]);

   style.set("alignment", group[19]);

   style.set("margin-l", group[20]);
   style.set("margin-r", group[21]);
   style.set("margin-v", group[22]);

   style.set("encoding", group[23]);
}

// Sets_default style from config (if no config is set, a hardcoded value set by styles.append is used
void set_default_style(Style& style) {
   if (cfg::has_key("AdvancedSubStationAlpha", "default-style") == true) {
      Glib::ustring default_style = cfg::get_string("AdvancedSubStationAlpha", "default-style");
      // when we read an ASS file, the group we get starts with empty item, so add it here
      default_style = "," + default_style;
      std::vector<Glib::ustring> group = Glib::Regex::split_simple(",", default_style);
      ASS::set_style_from_string(style, group);
   }
}

// sets PlayResX and PlayResY for the current document (and write default values to config if they are not there yet)
void set_default_playres(ScriptInfo& scriptInfo) {
   Glib::ustring play_res_x;
   Glib::ustring play_res_y;

   if (cfg::has_key("AdvancedSubStationAlpha", "default-playres-x") == false) {
      cfg::set_string("AdvancedSubStationAlpha", "default-playres-x", "1920");
      play_res_x = "1920";
   } else {
      play_res_x = cfg::get_string("AdvancedSubStationAlpha", "default-playres-x");
   }

   if (cfg::has_key("AdvancedSubStationAlpha", "default-playres-y") == false) {
      cfg::set_string("AdvancedSubStationAlpha", "default-playres-y", "1080");
      play_res_y = "1080";
   } else {
      play_res_y = cfg::get_string("AdvancedSubStationAlpha", "default-playres-y");
   }
   scriptInfo.data["PlayResY"] = play_res_y;
   scriptInfo.data["PlayResX"] = play_res_x;
}
}  // namespace ASS
