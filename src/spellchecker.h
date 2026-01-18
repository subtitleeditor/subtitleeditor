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

#include <glibmm.h>

#include <memory>
#include <vector>

class SEEnchantDict;

class SpellChecker {
  public:
   // Return an instance of the SpellChecker.
   static SpellChecker* instance();

   // Add this word to the dictionary only the time of the session.
   void add_word_to_session(const Glib::ustring& word);

   // Add this word to the personal dictionary.
   void add_word_to_personal(const Glib::ustring& word);

   // Spell a word.
   bool check(const Glib::ustring& word);

   // Returns a list of suggestions from the misspelled word.
   std::vector<Glib::ustring> get_suggest(const Glib::ustring& word);

   // Set the current dictionary. ("en_US", "de", ...)
   bool set_dictionary(const Glib::ustring& lang);

   // Returns the current dictionary as isocode. ("en_US", "de", ...)
   Glib::ustring get_dictionary();

   // Returns a list of the dictionaries available.
   std::vector<Glib::ustring> get_dictionaries();

   // The current dictionary's changed.
   sigc::signal<void>& signal_dictionary_changed();

   // Notes that you replaced 'bad' with 'good', so it's possibly more likely
   // that future occurrences of 'bad' will be replaced with 'good'.
   // So it might bump 'good' up in the suggestion list.
   void store_replacement(const Glib::ustring& utf8bad, const Glib::ustring& utf8good);

  protected:
   // Constructor
   SpellChecker();

   // Desctructor
   ~SpellChecker();

   // Setup the default dictionary.
   bool init_dictionary();

  protected:
   std::unique_ptr<SEEnchantDict> m_spellcheckerDict;
   sigc::signal<void> m_signal_dictionary_changed;
};
