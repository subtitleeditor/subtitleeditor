#pragma once

// subtitleeditor -- a tool to create or edit subtitle
//
// https://kitone.github.io/subtitleeditor/
// https://github.com/kitone/subtitleeditor/
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <gtkmm.h>
#include <iostream>
#include <sstream>
#include "debug.h"
#include "i18n.h"
#include "style.h"
#include "color.h"
#include "scriptinfo.h"


// Check whether a gtkmm version equal to or greater than
// major.minor.micro is present.
#define GTKMM_CHECK_VERSION(major, minor, micro)                        \
  (GTKMM_MAJOR_VERSION > (major) ||                                     \
   (GTKMM_MAJOR_VERSION == (major) && GTKMM_MINOR_VERSION > (minor)) || \
   (GTKMM_MAJOR_VERSION == (major) && GTKMM_MINOR_VERSION == (minor) && \
    GTKMM_MICRO_VERSION >= (micro)))

// Return one of the values depending on whether
// environment variable SE_DEV is defined or not.
#define SE_DEV_VALUE(value, dev_value) \
  ((Glib::getenv("SE_DEV") != "1") ? (value) : (dev_value))

// the profile name for the config dir
// ~/config/subtitleeditor/{profile}
void set_profile_name(const Glib::ustring &profile);

// ~/.config/subtitleeditor/{profile}/{file}
// XDG Base Directory Specification
Glib::ustring get_config_dir(const Glib::ustring &file);

// convertir str en n'importe quel type
template <class T>
bool from_string(const std::string &src, T &dest) {
  std::istringstream s(src);

  bool state = s >> dest != 0;

  if (!state)
    se_dbg_msg(SE_DBG_UTILITY, "string:'%s'failed.", src.c_str());

#ifdef DEBUG
  g_return_val_if_fail(state, false);
#endif
  return state;
}

// convertir str en n'importe quel type
template <class T>
bool from_string(const Glib::ustring &src, T &dest) {
  std::istringstream s(src);

  bool state = static_cast<bool>(s >> dest) != 0;

  if (!state)
    se_dbg_msg(SE_DBG_UTILITY, "string:'%s'failed.", src.c_str());

#ifdef DEBUG
  g_return_val_if_fail(state, false);
#endif
  return state;
}

// convertir n'importe quoi en string
template <class T>
std::string to_string(const T &src) {
  std::ostringstream oss;
  oss << src;

  return oss.str();
}

Glib::ustring build_message(const gchar *str, ...);

void dialog_warning(const Glib::ustring &primary_text,
                    const Glib::ustring &secondary_text);

void dialog_error(const Glib::ustring &primary_text,
                  const Glib::ustring &secondary_text);

namespace utility {

template <class T>
inline void clamp(T &val, const T &min, const T &max) {
  val = CLAMP(val, min, max);
}

bool string_to_bool(const std::string &str);

int string_to_int(const std::string &str);

int string_to_long(const std::string &str);

double string_to_double(const std::string &str);

void split(const std::string &str, const char &c,
           std::vector<std::string> &array, int max = -1);

// Split with best utf8 support...
void usplit(const Glib::ustring &str,
            const Glib::ustring::value_type &delimiter,
            std::vector<Glib::ustring> &container);

// Search and replace function.
void replace(Glib::ustring &text, const Glib::ustring &pattern,
             const Glib::ustring &replace_by);

// Search and replace function.
void replace(std::string &text, const std::string &pattern,
             const std::string &replace_by);

// transforme test/file.srt en /home/toto/test/file.srt
Glib::ustring create_full_path(const Glib::ustring &path);

// Get the number of characters per second.
// msec = SubtitleTime::totalmsecs
double get_characters_per_second(const Glib::ustring &text, const long msecs);

// Count characters in a subtitle the way they need to be counted
// for subtitle timing.
unsigned int get_text_length_for_timing(const Glib::ustring &text);

// Calculate the minimum acceptable duration for a string of this length.
unsigned long get_min_duration_msecs(unsigned long textlen, double maxcps);
unsigned long get_min_duration_msecs(const Glib::ustring &text, double maxcps);

// get number of characters for each line in the text
std::vector<int> get_characters_per_line(const Glib::ustring &text);

// get a text stripped from tags
Glib::ustring get_stripped_text(const Glib::ustring &text);

void set_transient_parent(Gtk::Window &window);

Glib::ustring add_or_replace_extension(const Glib::ustring &filename,
                                       const Glib::ustring &extension);
}  // namespace utility

namespace ASS {
  // Convert bool from ASS to SE
  // ASS: 0 == false, -1 == true
  Glib::ustring from_ass_bool(const Glib::ustring &value);

  // Convert color from ASS to SE
  Glib::ustring from_ass_color(const Glib::ustring &str);

  // Convert bool from SE to ASS
  // ASS: false == 0, true == -1
  Glib::ustring to_ass_bool(const Glib::ustring &value);

  // Convert color from SE to ASS
  Glib::ustring to_ass_color(const Color &color);

  // Returns style written as string, like for example this:
  // Default,Sans,40,&H00FFFFFF,&H00FFFFFF,&H00FFFFFF,&H00FFFFFF,0,0,0,0,100,100,0,0,1,0,0,20,20,20,20,0
  Glib::ustring style_to_string(const Style &style);

  // Sets style from string (typically something like
  // Default,Sans,18,&H00FFFFFF,&H0000FFFF,&H000078B4,&H00000000,0,0,0,0,100,100,0,0,1,0,0,2,20,20,20,0
  // expects the first member of the vector of strings to be empty
  void set_style_from_string(Style &style, std::vector<Glib::ustring> group);

  // sets_default style from config (if no config is set, a hardcoded value set by styles.append is used
  void set_default_style(Style &style);

  // sets PlayResX and PlayResY for the current document (and write default values to config if they are not there yet)
  void set_default_playres(ScriptInfo &scriptInfo);
}
