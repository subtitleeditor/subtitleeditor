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

#include <glibmm.h>
#include <libxml++/libxml++.h>
#include <iostream>
#include <map>
#include "debug.h"
#include "i18n.h"
#include "isocodes.h"

namespace isocodes {

bool iso_codes_load_file(const Glib::ustring &iso_id,
                         const Glib::ustring &id_code,
                         std::map<Glib::ustring, Glib::ustring> &codes) {
#ifdef HAVE_ISO_CODES
  try {
    Glib::ustring filename =
        Glib::build_filename(ISO_CODES_PATH, iso_id + ".xml");

    xmlpp::DomParser parser;
    parser.set_substitute_entities();
    parser.parse_file(filename.c_str());

    const xmlpp::Element *root = dynamic_cast<const xmlpp::Element *>(
        parser.get_document()->get_root_node());
    if (root->get_name() != Glib::ustring::compose("%1_entries", iso_id))
      return false;

    xmlpp::Node::NodeList entries =
        root->get_children(Glib::ustring::compose("%1_entry", iso_id));

    for (const auto &node : entries) {
      auto entry = dynamic_cast<const xmlpp::Element *>(node);

      Glib::ustring code = entry->get_attribute_value(id_code);
      Glib::ustring name = entry->get_attribute_value("name");
      if (code.empty() || name.empty())
        continue;

      codes[code] = name;
    }
    //
    bind_textdomain_codeset(iso_id.c_str(), "UTF-8");

    return true;
  } catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
  }
#endif  // HAVE_ISO_CODES

  return false;
}

// Variables
static bool iso_codes_initialised = false;
static bool init_639 = false;
static bool init_3166 = false;
static bool init_15924 = false;
static std::map<Glib::ustring, Glib::ustring> iso_codes_639;
static std::map<Glib::ustring, Glib::ustring> iso_codes_3166;
static std::map<Glib::ustring, Glib::ustring> iso_codes_15924;

void init_isocodes() {
  if (iso_codes_initialised)
    return;

  init_639 = iso_codes_load_file("iso_639", "iso_639_1_code", iso_codes_639);
  init_3166 = iso_codes_load_file("iso_3166", "alpha_2_code", iso_codes_3166);
  init_15924 =
      iso_codes_load_file("iso_15924", "alpha_4_code", iso_codes_15924);
  iso_codes_initialised = true;
}

Glib::ustring from_isocodes(const Glib::ustring &domain,
                            std::map<Glib::ustring, Glib::ustring> &isocodes,
                            const Glib::ustring &code) {
  auto it = isocodes.find(code);
  if (it == isocodes.end())
    return code;
  return dgettext(domain.c_str(), it->second.c_str());
}

// Convert ISO 639 code to localized language name.
// ex: "fr" to "French"
Glib::ustring to_language(const Glib::ustring &code) {
  init_isocodes();

  if (init_639)
    return from_isocodes("iso_639", iso_codes_639, code);
  return code;
}

// Convert ISO 3166 code to localized country name.
// ex: "FR" to "France"
Glib::ustring to_country(const Glib::ustring &code) {
  init_isocodes();

  if (init_3166)
    return from_isocodes("iso_3166", iso_codes_3166, code);
  return code;
}

// Convert ISO 15924 code to localized country name.
// ex: "Latn" to "Latin"
Glib::ustring to_script(const Glib::ustring &code) {
  init_isocodes();

  if (init_15924)
    return from_isocodes("iso_15924", iso_codes_15924, code);
  return code;
}

// Convert from ISO XXX to good localized name:
// ex: "fr_FR" to "French (France)", "US" to "United States"...
Glib::ustring to_name(const Glib::ustring &code) {
  if (Glib::Regex::match_simple("^[a-z][a-z]$", code)) {
    return to_language(code);
  } else if (Glib::Regex::match_simple("^[A-Z][A-Z]$", code)) {
    return to_country(code);
  } else if (Glib::Regex::match_simple("^[a-z][a-z]_[A-Z][A-Z]$", code)) {
    Glib::ustring language = to_language(code.substr(0, 2));
    Glib::ustring country = to_country(code.substr(3, 5));

    return Glib::ustring::compose("%1 (%2)", language, country);
  } else if (Glib::Regex::match_simple("^[A-Z][a-z]{3}$", code)) {
    return to_script(code);
  }
  return code;
}

}  // namespace isocodes
