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

#include "spellchecker.h"

#include <debug.h>
#include <enchant.h>

#include "cfg.h"

// So why do not using enchant++ ?
// There are a lots of segfault and memory leaks at the exit
// of subtitleeditor caused by enchant++
// (static Broker instance in the header ...)
class SEEnchantDict {
  public:
   class Exception : public std::exception {
     public:
      explicit Exception(const char* ex) : std::exception(), m_ex("") {
         if (ex)
            m_ex = ex;
      }

      virtual ~Exception() throw() {
      }

      virtual const char* what() throw() {
         return m_ex.c_str();
      }

     private:
      Exception();
      std::string m_ex;
   };

  public:
   SEEnchantDict() : m_broker(enchant_broker_init()), m_dict(NULL) {
   }

   ~SEEnchantDict() {
      free_dict();
      enchant_broker_free(m_broker);
   }

   bool request_dict(const std::string& lang) {
      free_dict();
      m_dict = enchant_broker_request_dict(m_broker, lang.c_str());
      if (!m_dict)
         throw Exception(enchant_broker_get_error(m_broker));
      m_active_lang = lang;
      return true;
   }

   void add_word_to_session(const std::string& utf8) {
      g_return_if_fail(m_dict);
      g_return_if_fail(!m_active_lang.empty());

      enchant_dict_add_to_session(m_dict, utf8.c_str(), utf8.size());
   }

   void add_word_to_personal(const std::string& utf8) {
      g_return_if_fail(m_dict);
      g_return_if_fail(!m_active_lang.empty());

      enchant_dict_add(m_dict, utf8.c_str(), utf8.size());
   }

   bool check(const std::string& utf8) {
      g_return_val_if_fail(m_dict, false);
      g_return_val_if_fail(!m_active_lang.empty(), false);

      int val = enchant_dict_check(m_dict, utf8.c_str(), utf8.size());
      if (val == 0)
         return true;
      else if (val > 0)
         return false;
      else
         throw Exception(enchant_dict_get_error(m_dict));
      return false;  // never reached
   }

   void suggest(const std::string& utf8word, std::vector<std::string>& out_suggestions) {
      g_return_if_fail(m_dict);
      g_return_if_fail(!m_active_lang.empty());
      g_return_if_fail(!utf8word.empty());

      size_t n_suggs = 0;
      char** suggs = nullptr;

      out_suggestions.clear();

      suggs = enchant_dict_suggest(m_dict, utf8word.c_str(), utf8word.size(), &n_suggs);
      if (suggs && n_suggs) {
         for (size_t i = 0; i < n_suggs; ++i) {
            out_suggestions.push_back(suggs[i]);
         }
         enchant_dict_free_string_list(m_dict, suggs);
      }
   }

   void store_replacement(const std::string& utf8bad, const std::string& utf8good) {
      g_return_if_fail(m_dict);
      g_return_if_fail(!m_active_lang.empty());

      enchant_dict_store_replacement(m_dict, utf8bad.c_str(), utf8bad.size(), utf8good.c_str(), utf8good.size());
   }

   void get_dictionaries(std::list<std::string>& list) {
      list.clear();

      g_return_if_fail(m_broker);

      enchant_broker_list_dicts(m_broker, callback_list_dicts, &list);
   }

   std::string get_lang() {
      return m_active_lang;
   }

  protected:
   static void callback_list_dicts(const char* const lang_tag,
                                   const char* const /*provider_name*/,
                                   const char* const /*provider_desc*/,
                                   const char* const /*provider_file*/,
                                   void* user_data) {
      reinterpret_cast<std::list<std::string>*>(user_data)->push_back(lang_tag);
   }

   void free_dict() {
      if (m_dict != NULL) {
         enchant_broker_free_dict(m_broker, m_dict);
         m_dict = NULL;
         m_active_lang = std::string();
      }
   }

  protected:
   EnchantBroker* m_broker;
   EnchantDict* m_dict;
   std::string m_active_lang;
};

bool spell_checker_is_digit(const Glib::ustring& text) {
   for (const auto& c : text) {
      if (!g_unichar_isdigit(c) && c != '.' && c != ',')
         return false;
   }
   return true;
}

// Constructor
SpellChecker::SpellChecker() : m_spellcheckerDict(new SEEnchantDict) {
   se_dbg(SE_DBG_SPELL_CHECKING);

   init_dictionary();
}

// Desctructor
SpellChecker::~SpellChecker() {
   se_dbg(SE_DBG_SPELL_CHECKING);
}

// Return an instance of the SpellChecker.
SpellChecker* SpellChecker::instance() {
   static SpellChecker _instance;
   return &_instance;
}

// Setup the default dictionary.
bool SpellChecker::init_dictionary() {
   Glib::ustring lang;

   // Try with the last config
   se_dbg_msg(SE_DBG_SPELL_CHECKING, "Try with the last config...");

   if (cfg::has_key("spell-checker", "lang")) {
      lang = cfg::get_string("spell-checker", "lang");
      if (set_dictionary(lang))
         return true;
   }
   // Second try to get a default language
   se_dbg_msg(SE_DBG_SPELL_CHECKING, "Second try to get a default language...");

   lang = Glib::getenv("LANG");
   if (!lang.empty()) {
      Glib::ustring::size_type p = lang.find(".");
      if (p != Glib::ustring::npos)
         lang = lang.substr(0, p);

      if (set_dictionary(lang))
         return true;
   }
   // Last try to get a first language
   se_dbg_msg(SE_DBG_SPELL_CHECKING, "Last try to get a first language...");

   std::vector<Glib::ustring> dicts = get_dictionaries();
   if (!dicts.empty() && set_dictionary(dicts[0]))
      return true;

   se_dbg_msg(SE_DBG_SPELL_CHECKING, "cannot select a default language!");
   g_warning("Spell checker: cannot select a default language");
   return false;
}

// Add this word to the dictionary only the time of the session.
void SpellChecker::add_word_to_session(const Glib::ustring& word) {
   se_dbg_msg(SE_DBG_SPELL_CHECKING, "add word '%s' to session", word.c_str());

   m_spellcheckerDict->add_word_to_session(word);
}

// Add this word to the personal dictionary.
void SpellChecker::add_word_to_personal(const Glib::ustring& word) {
   se_dbg_msg(SE_DBG_SPELL_CHECKING, "add word '%s' to personal dictionary", word.c_str());

   m_spellcheckerDict->add_word_to_personal(word);
}

// Spell a word.
bool SpellChecker::check(const Glib::ustring& word) {
   se_dbg_msg(SE_DBG_SPELL_CHECKING, "check the word '%s'", word.c_str());
   try {
      // Don't check number
      if (spell_checker_is_digit(word))
         return true;

      return m_spellcheckerDict->check(word);
   } catch (std::exception& ex) {
      se_dbg_msg(SE_DBG_SPELL_CHECKING, "exception '%s'", ex.what());
   } catch (...) {
      se_dbg_msg(SE_DBG_SPELL_CHECKING, "unknow exception");
   }
   return false;
}

// Returns a list of suggestions from the misspelled word.
std::vector<Glib::ustring> SpellChecker::get_suggest(const Glib::ustring& word) {
   se_dbg_msg(SE_DBG_SPELL_CHECKING, "get suggestion from the word '%s'", word.c_str());

   std::vector<std::string> sugg;
   m_spellcheckerDict->suggest(word, sugg);
   return std::vector<Glib::ustring>(sugg.begin(), sugg.end());
}

// Set the current dictionary. ("en_US", "de", ...)
bool SpellChecker::set_dictionary(const Glib::ustring& name) {
   se_dbg_msg(SE_DBG_SPELL_CHECKING, "try to set dictionary '%s' ...", name.c_str());

   if (name.empty())
      return false;

   try {
      m_spellcheckerDict->request_dict(name);

      cfg::set_string("spell-checker", "lang", name);
      m_signal_dictionary_changed.emit();
      return true;
   } catch (SEEnchantDict::Exception& ex) {
      se_dbg_msg(SE_DBG_SPELL_CHECKING, "Failed to set the dictionary '%s' : %s'", name.c_str(), ex.what());
   }
   return false;
}

// Returns the current dictionary as isocode. ("en_US", "de", ...)
Glib::ustring SpellChecker::get_dictionary() {
   se_dbg(SE_DBG_SPELL_CHECKING);

   return m_spellcheckerDict->get_lang();
}

// Returns a list of the dictionaries available.
std::vector<Glib::ustring> SpellChecker::get_dictionaries() {
   se_dbg(SE_DBG_SPELL_CHECKING);

   std::list<std::string> list_dicts;

   m_spellcheckerDict->get_dictionaries(list_dicts);

   return std::vector<Glib::ustring>(list_dicts.begin(), list_dicts.end());
}

// The current dictionary's changed.
sigc::signal<void>& SpellChecker::signal_dictionary_changed() {
   se_dbg(SE_DBG_SPELL_CHECKING);

   return m_signal_dictionary_changed;
}

// Notes that you replaced 'bad' with 'good', so it's possibly more likely
// that future occurrences of 'bad' will be replaced with 'good'.
// So it might bump 'good' up in the suggestion list.
void SpellChecker::store_replacement(const Glib::ustring& utf8bad, const Glib::ustring& utf8good) {
   se_dbg_msg(SE_DBG_SPELL_CHECKING, "store replacement '%s' to '%s'", utf8bad.c_str(), utf8good.c_str());

   m_spellcheckerDict->store_replacement(utf8bad, utf8good);
}
