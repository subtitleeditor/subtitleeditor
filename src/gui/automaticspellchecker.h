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

class AutomaticSpellChecker : public Glib::ObjectBase {
 public:
  static AutomaticSpellChecker *create_from_textview(Gtk::TextView *view);

 protected:
  // The instance is attached to the textview,
  // it will be destroy himself with the textview.
  explicit AutomaticSpellChecker(Gtk::TextView *textview);

  virtual ~AutomaticSpellChecker();

  // Connect signals with the textview and the textbuffer,
  // create the tag 'asc-misspelled' ...
  void init(Gtk::TextView *view);

  Glib::RefPtr<Gtk::TextBuffer> get_buffer();

  // TextBuffer callback

  // Insertion works like this:
  // - before the text is inserted, we mark the position in the buffer.
  // - after the text is inserted, we see where our mark is and use that and
  //   the current position to check the entire range of inserted text.
  void on_insert_text_before(const Gtk::TextBuffer::iterator &pos,
                             const Glib::ustring &text, int bytes);

  // Insertion works like this:
  // - before the text is inserted, we mark the position in the buffer.
  // - after the text is inserted, we see where our mark is and use that and
  //   the current position to check the entire range of inserted text.
  void on_insert_text_after(const Gtk::TextBuffer::iterator &pos,
                            const Glib::ustring &text, int bytes);

  // Deleting is more simple:  we're given the range of deleted text.
  // After deletion, the start and end iters should be at the same position
  // (because all of the text between them was deleted!).
  // This means we only really check the words immediately bounding the
  // deletion.
  void on_erase(const Gtk::TextBuffer::iterator &start,
                const Gtk::TextBuffer::iterator &end);

  void on_mark_set(const Gtk::TextBuffer::iterator &location,
                   const Glib::RefPtr<Gtk::TextBuffer::Mark> &mark);

  // TagTable callback

  // Set the tag 'highlight' as priority.
  void tag_table_changed();

  // Update the tag 'highlight' priority.
  void on_tag_changed(const Glib::RefPtr<Gtk::TextTag> &tag, bool size_changed);

  // Update the tag 'highlight' priority.
  void on_tag_added_or_removed(const Glib::RefPtr<Gtk::TextTag> &tag);

  // check and utils functions.

  // Set iterators with the word from mark.
  void get_word_extents_from_mark(
      const Glib::RefPtr<Gtk::TextBuffer::Mark> &mark, Gtk::TextIter &start,
      Gtk::TextIter &end);

  // Check the word delimited by the iterators and if it's misspell tag it.
  void check_word(Gtk::TextIter start, Gtk::TextIter end);

  void check_deferred_range(bool force_all);

  // Check words delimited by the iterators.
  void check_range(Gtk::TextIter start, Gtk::TextIter end, bool force_all);

  // Recheck all the textbuffer.
  void recheck_all();

  // Widget events (popup, click ...)

  // Update the mark click.
  bool on_popup_menu();

  // Build spell check menu.
  // The menu 'languages' is always added to allow the user to change the
  // dictionary without misspell word unlike the menu 'suggestions' added only
  // if the word is misspell.
  void on_populate_popup(Gtk::Menu *popup);

  // When the user right-clicks on a word, they want to check that word.
  // Here, we do NOT  move the cursor to the location of the clicked-upon word
  // since that prevents the use of edit functions on the context menu.
  bool on_button_press_event(GdkEventButton *ev);

  // Suggestions menu

  // Build a suggestions menu from misspelled word, 'menu' is directly used to
  // add menu items.
  // 'Ignore All' and 'Add "%" to Dictionary" are always created.
  // Create suggestions list, if they are more than ten, create submenu
  // 'More...' for they.
  void build_suggestion_menu(const Glib::ustring &word, Gtk::Menu *menu);

  // We get the word from the 'mark click' and we replace it by the 'newword'.
  // Store the replacement in the SpellChecker, that future occurrences of the
  // word will be replaced with the 'newword'.
  // It's call by the item spell suggestions.
  void on_replace_word(const Glib::ustring &newword);

  // We get the word from the 'mark click' and we add the word to
  // the session of the spell checker.
  // It will be ignored in the future the time of the session.
  // It's call by the suggestions item "Ignore all".
  void on_ignore_all();

  // We get the word from the 'mark click' and we add the word to
  // the personal dictionary.
  // It will be ignored in the future.
  // It's call by the suggestions item "Add %s to Dictionary".
  void on_add_to_dictionary();

  // Languages menu
  // Manage spellchecker dictionary

  // Create and return a languages menu.
  Gtk::Menu *build_languages_menu();

  // Update the spellchecker with a new dictionary.
  void on_set_current_language(const Glib::ustring &isocode);

  // Delete the instance of AutomaticSpellChecker attached to the TextView.
  static void automatic_spell_checker_destroy(gpointer data);

  // heuristic:
  // if we're on an singlequote/apostrophe and
  // if the next letter is alphanumeric, this is an apostrophe.
  bool iter_forward_word_end(Gtk::TextIter &i);

  // heuristic:
  // if we're on an singlequote/apostrophe and
  // if the next letter is alphanumeric, this is an apostrophe.
  bool iter_backward_word_start(Gtk::TextIter &i);

 protected:
  Gtk::TextView *m_textview;
  Glib::RefPtr<Gtk::TextBuffer::Mark> m_mark_insert_start;
  Glib::RefPtr<Gtk::TextBuffer::Mark> m_mark_insert_end;
  Glib::RefPtr<Gtk::TextBuffer::Tag> m_tag_highlight;
  Glib::RefPtr<Gtk::TextBuffer::Mark> m_mark_click;
  bool m_deferred_check;
};
