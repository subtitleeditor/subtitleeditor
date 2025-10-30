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

#include <libxml++/libxml++.h>
#include <vector>
#include "pattern.h"

class PatternManager {
 public:
  // Read and create all patterns as type from the install directory
  // and the user profile directory.
  // type: 'common-error', 'hearing-impaired'
  explicit PatternManager(const Glib::ustring &type);

  // Delete patterns.
  ~PatternManager();

  // Return all scripts available. (Zyyy is skipped)
  std::vector<Glib::ustring> get_scripts();

  // Return all languages available for the script code.
  std::vector<Glib::ustring> get_languages(const Glib::ustring &script);

  // Return all countries available for the script and language codes.
  std::vector<Glib::ustring> get_countries(const Glib::ustring &script,
                                           const Glib::ustring &language);

  // Return a list of patterns available from the codes.
  std::list<Pattern *> get_patterns(
      const Glib::ustring &script = Glib::ustring(),
      const Glib::ustring &language = Glib::ustring(),
      const Glib::ustring &country = Glib::ustring());

  // Enable or disable the patterns from his name.
  // The configuration is update with the new state.
  // It's managed in this class because a multiple pattern can be have a same
  // name.
  void set_active(const Glib::ustring &name, bool state);

  // Return the state of the pattern from his name.
  bool get_active(const Glib::ustring &name);

 protected:
  // Load patterns in the directory.
  void load_path(const Glib::ustring &path);

  // Load a pattern from a file.
  void load_pattern(const Glib::ustring &path, const Glib::ustring &filename);

  // Read, create and return a pattern from xml element.
  Pattern *read_pattern(const xmlpp::Element *xml_pattern);

  // The patterns need to be filtered to respect the Replace policy
  // Maintain order of patterns with the same name
  std::list<Pattern *> filter_patterns(std::list<Pattern *> &list);

  // Return all codes needs to be used from args.
  // 'Zyyy', 'script', 'script-language' and 'script-language-country'.
  // Zyyy is the first and it is always added.
  std::vector<Glib::ustring> get_codes(
      const Glib::ustring &script = Glib::ustring(),
      const Glib::ustring &language = Glib::ustring(),
      const Glib::ustring &country = Glib::ustring());

 protected:
  Glib::ustring m_type;
  std::list<Pattern *> m_patterns;
};
