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

#include <cfg.h>
#include <utility.h>
#include "patternmanager.h"

// Read and create all patterns as type from the install directory
// and the user profile directory.
// type: 'common-error', 'hearing-impaired'
PatternManager::PatternManager(const Glib::ustring &type) {
  se_dbg_msg(SE_DBG_PLUGINS, "pattern manager for '%s'", type.c_str());
  m_type = type;

  Glib::ustring path = SE_DEV_VALUE(SE_PLUGIN_PATH_PATTERN, SE_PLUGIN_PATH_DEV);
  load_path(path);
  // Read the user patterns in '$config/plugins/textcorrection'
  load_path(get_config_dir("plugins/textcorrection"));
}

// Delete patterns.
PatternManager::~PatternManager() {
  se_dbg(SE_DBG_PLUGINS);

  for (auto p : m_patterns) {
    delete p;
  }
  m_patterns.clear();
}

// Load patterns in the directory.
void PatternManager::load_path(const Glib::ustring &path) {
  if (Glib::file_test(path, Glib::FILE_TEST_EXISTS | Glib::FILE_TEST_IS_DIR) ==
      false) {
    se_dbg_msg(SE_DBG_PLUGINS, "could not open the path %s", path.c_str());
    return;
  }

  try {
    se_dbg_msg(SE_DBG_PLUGINS, "path '%s'", path.c_str());
    // Only the pattern type
    Glib::RefPtr<Glib::Regex> re = Glib::Regex::create(
        Glib::ustring::compose("^(.*)\\.%1\\.se-pattern$", m_type));

    Glib::Dir dir(path);
    std::vector<Glib::ustring> files(dir.begin(), dir.end());
    for (const auto &file : files) {
      if (re->match(file)) {
        load_pattern(path, file);
      }
    }
  } catch (const Glib::Error &ex) {
    std::cerr << ex.what() << std::endl;
  } catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
  }
}

// Load a pattern from a file.
void PatternManager::load_pattern(const Glib::ustring &path,
                                  const Glib::ustring &filename) {
  try {
    Glib::ustring fullname = Glib::build_filename(path, filename);

    se_dbg_msg(SE_DBG_PLUGINS, "filename '%s'", fullname.c_str());
    // name of file :
    // Script[-language-[COUNTRY]].PatternType.pattern
    Glib::RefPtr<Glib::Regex> re =
        Glib::Regex::create("^(.*)\\..*\\.se-pattern$");
    if (re->match(filename) == false)
      return;
    // Get codes
    Glib::ustring codes;
    std::vector<Glib::ustring> group = re->split(filename);
    codes = group[1];
    // Read the pattern
    xmlpp::DomParser parser;
    parser.set_substitute_entities();
    parser.parse_file(fullname.c_str());
    // patterns (root)
    const xmlpp::Element *xml_patterns = dynamic_cast<const xmlpp::Element *>(
        parser.get_document()->get_root_node());
    if (xml_patterns->get_name() != "patterns") {
      se_dbg_msg(SE_DBG_PLUGINS, "The file '%s' is not a pattern file",
                 fullname.c_str());
      // throw InvalidFile
      return;
    }
    // read patterns
    auto xml_pattern_list = xml_patterns->get_children("pattern");
    for (const auto &node : xml_pattern_list) {
      const auto xml_pattern = dynamic_cast<const xmlpp::Element *>(node);
      // read and add the patterns to the list
      Pattern *pattern = read_pattern(xml_pattern);
      if (pattern) {
        pattern->m_codes = codes;
        m_patterns.push_back(pattern);
      }
    }
  } catch (const std::exception &ex) {
    se_dbg_msg(SE_DBG_PLUGINS, "Could not read the pattern '%s' : %s",
               filename.c_str(), ex.what());
    std::cerr << ex.what() << std::endl;
  }
}

// Convert string flags to Glib::RegexCompileFlags
Glib::RegexCompileFlags parse_flags(const Glib::ustring &string) {
  Glib::RegexCompileFlags flags = static_cast<Glib::RegexCompileFlags>(0);

  if (string.find("CASELESS") != Glib::ustring::npos)
    flags |= Glib::REGEX_CASELESS;
  else if (string.find("MULTILINE") != Glib::ustring::npos)
    flags |= Glib::REGEX_MULTILINE;
  else if (string.find("DOTALL") != Glib::ustring::npos)
    flags |= Glib::REGEX_DOTALL;
  // FIXME UNICODE ?
  return flags;
}

// Read, create and return a pattern from xml element.
Pattern *PatternManager::read_pattern(const xmlpp::Element *xml_pattern) {
  Pattern *pattern = new Pattern;
  // get description
  pattern->m_name = xml_pattern->get_attribute_value("name");
  pattern->m_label = _(pattern->m_name.c_str());  // Localized name
  pattern->m_description = xml_pattern->get_attribute_value("description");
  pattern->m_classes = xml_pattern->get_attribute_value("classes");
  pattern->m_policy = xml_pattern->get_attribute_value("policy");
  pattern->m_enabled = get_active(pattern->m_name);
  // get rules
  auto xml_rule_list = xml_pattern->get_children("rule");

  for (const auto &node : xml_rule_list) {
    auto xml_rule = dynamic_cast<const xmlpp::Element *>(node);

    Glib::ustring regex = xml_rule->get_attribute_value("regex");
    Glib::ustring flags = xml_rule->get_attribute_value("flags");
    Glib::ustring replacement = xml_rule->get_attribute_value("replacement");
    Glib::ustring repeat = xml_rule->get_attribute_value("repeat");

    try {
      Pattern::Rule *rule = new Pattern::Rule;
      rule->m_regex = Glib::Regex::create(regex, parse_flags(flags));
      rule->m_replacement = replacement;
      rule->m_repeat = (repeat == "True") ? true : false;

      // Previous match rule
      auto xml_previous_match = xml_rule->get_children("previousmatch");
      if (!xml_previous_match.empty()) {
        auto pre =
            dynamic_cast<const xmlpp::Element *>(*xml_previous_match.begin());

        Glib::ustring preregex = pre->get_attribute_value("regex");
        Glib::ustring preflags = pre->get_attribute_value("flags");

        rule->m_previous_match =
            Glib::Regex::create(preregex, parse_flags(preflags));
      }

      pattern->m_rules.push_back(rule);
    } catch (Glib::Error &ex) {
      std::cerr << ex.what();
    }
  }

  return pattern;
}

// Return all codes needs to be used from args.
// 'Zyyy', 'script', 'script-language' and 'script-language-country'.
// Zyyy is the first and it is always added.
std::vector<Glib::ustring> PatternManager::get_codes(
    const Glib::ustring &script, const Glib::ustring &language,
    const Glib::ustring &country) {
  std::vector<Glib::ustring> codes;
  codes.push_back("Zyyy");

  if (!script.empty()) {
    codes.push_back(script);

    if (!language.empty()) {
      codes.push_back(Glib::ustring::compose("%1-%2", script, language));

      if (!country.empty())
        codes.push_back(
            Glib::ustring::compose("%1-%2-%3", script, language, country));
    }
  }

  return codes;
}

// Return a list of patterns available from the codes.
std::list<Pattern *> PatternManager::get_patterns(
    const Glib::ustring &script, const Glib::ustring &language,
    const Glib::ustring &country) {
  se_dbg_msg(SE_DBG_PLUGINS, "Codes: %s-%s-%s", script.c_str(),
             language.c_str(), country.c_str());

  std::vector<Glib::ustring> codes = get_codes(script, language, country);

  std::list<Pattern *> patterns;

  for (auto const &code : codes) {
    for (auto const &pattern : m_patterns) {
      if (pattern->m_codes == code)
        patterns.push_back(pattern);
    }
  }
  // the patterns need to be filtered to respect the Replace policy
  std::list<Pattern *> filtered = filter_patterns(patterns);

  if (se_dbg_check_flags(SE_DBG_PLUGINS)) {
    se_dbg_msg(SE_DBG_PLUGINS, "pattern list before filter (%d)",
               patterns.size());
    for (const auto &p : patterns) {
      se_dbg_msg(SE_DBG_PLUGINS, "[%s] [%s]", p->m_codes.c_str(),
                 p->m_name.c_str());
    }

    se_dbg_msg(SE_DBG_PLUGINS, "pattern list after filter (%d)",
               filtered.size());
    for (const auto &p : filtered) {
      se_dbg_msg(SE_DBG_PLUGINS, "[%s] [%s]", p->m_codes.c_str(),
                 p->m_name.c_str());
    }
  }

  return filtered;
}

// Return all scripts available. (Zyyy is skipped)
std::vector<Glib::ustring> PatternManager::get_scripts() {
  std::list<Glib::ustring> codes;

  Glib::RefPtr<Glib::Regex> re = Glib::Regex::create("^([A-Za-z]{4}).*$");
  for (const auto &p : m_patterns) {
    if (!re->match(p->m_codes))
      continue;

    std::vector<Glib::ustring> group = re->split(p->m_codes);
    if (group[1] == "Zyyy")
      continue;

    codes.push_back(group[1]);
  }
  codes.unique();
  return std::vector<Glib::ustring>(codes.begin(), codes.end());
}

// Return all languages available for the script code.
std::vector<Glib::ustring> PatternManager::get_languages(
    const Glib::ustring &script) {
  std::list<Glib::ustring> codes;

  Glib::RefPtr<Glib::Regex> re = Glib::Regex::create(
      Glib::ustring::compose("^%1-([A-Za-z]{2}).*$", script));

  for (const auto &p : m_patterns) {
    if (!re->match(p->m_codes))
      continue;

    std::vector<Glib::ustring> group = re->split(p->m_codes);

    codes.push_back(group[1]);
  }
  codes.unique();
  return std::vector<Glib::ustring>(codes.begin(), codes.end());
}

// Return all countries available for the script and language codes.
std::vector<Glib::ustring> PatternManager::get_countries(
    const Glib::ustring &script, const Glib::ustring &language) {
  std::list<Glib::ustring> codes;

  Glib::RefPtr<Glib::Regex> re = Glib::Regex::create(
      Glib::ustring::compose("^%1-%2-([A-Za-z]{2})$", script, language));

  for (const auto &p : m_patterns) {
    if (!re->match(p->m_codes))
      continue;

    std::vector<Glib::ustring> group = re->split(p->m_codes);

    codes.push_back(group[1]);
  }
  codes.unique();
  return std::vector<Glib::ustring>(codes.begin(), codes.end());
}

// The patterns need to be filtered to respect the Replace policy
// Maintain order of patterns with the same name
std::list<Pattern *> PatternManager::filter_patterns(
    std::list<Pattern *> &pattern) {
  std::list<Pattern *> filtered;
  std::list<Pattern *>::iterator p, f, last_idx;

  for (p = pattern.begin(); p != pattern.end(); ++p) {
    bool replace = ((*p)->m_policy == "Replace");

    last_idx = filtered.end();
    for (f = filtered.begin(); f != filtered.end(); ++f) {
      if ((*f)->m_name == (*p)->m_name) {
        last_idx = f;
        if (replace)
          *f = NULL;
      }
    }

    if (last_idx == filtered.end())
      filtered.push_back(*p);
    else
      filtered.insert(++last_idx, *p);

    // Remove NULL
    f = filtered.begin();
    while (f != filtered.end()) {
      if (*f == NULL)
        f = filtered.erase(f);
      else
        ++f;
    }
  }
  return filtered;
}

// Enable or disable the patterns from his name.
// The configuration is update with the new state.
// It's managed in this class because a multiple pattern can be have a same
// name.
void PatternManager::set_active(const Glib::ustring &name, bool state) {
  if (name.empty()) {
    std::cerr << "* set_active failed. name is empty." << std::endl;
    return;
  }

  cfg::set_string("patterns", name, state ? "enable" : "disable");

  for (auto p : m_patterns) {
    if (p->m_name == name)
      p->m_enabled = state;
  }
}

// Return the state of the pattern from his name.
bool PatternManager::get_active(const Glib::ustring &name) {
  if (name.empty()) {
    std::cerr << "* get_active failed. name is empty." << std::endl;
    return false;
  }

  if (cfg::has_key("patterns", name) == false) {
    cfg::set_string("patterns", name, "enable");
    return true;
  }
  Glib::ustring value = cfg::get_string("patterns", name);
  return (value == "enable") ? true : false;
}
