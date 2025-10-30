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
#include <i18n.h>
#include <isocodes.h>
#include <algorithm>
#include <memory>
#include "automaticspellchecker.h"
#include "spellchecker.h"

AutomaticSpellChecker *AutomaticSpellChecker::create_from_textview(
    Gtk::TextView *view) {
  g_return_val_if_fail(view, NULL);

  AutomaticSpellChecker *asc = new AutomaticSpellChecker(view);
  return asc;
}

// Delete the instance of AutomaticSpellChecker attached to the TextView.
void AutomaticSpellChecker::automatic_spell_checker_destroy(gpointer data) {
  AutomaticSpellChecker *i = reinterpret_cast<AutomaticSpellChecker *>(data);
  delete i;
}

// The instance is attached to the textview,
// it will be destroy himself with the textview.
AutomaticSpellChecker::AutomaticSpellChecker(Gtk::TextView *view)
    : Glib::ObjectBase(typeid(AutomaticSpellChecker)) {
  init(view);
}

AutomaticSpellChecker::~AutomaticSpellChecker() {
}

// Connect signals with the textview and the textbuffer,
// create the tag 'asc-misspelled' ...
void AutomaticSpellChecker::init(Gtk::TextView *view) {
  m_textview = view;

  m_textview->set_data("AutomaticSpellChecker", this,
                       AutomaticSpellChecker::automatic_spell_checker_destroy);

  Glib::RefPtr<Gtk::TextBuffer> m_buffer = view->get_buffer();

  // signals
  m_buffer->signal_insert().connect(
      sigc::mem_fun(*this, &AutomaticSpellChecker::on_insert_text_before),
      false);

  m_buffer->signal_insert().connect(
      sigc::mem_fun(*this, &AutomaticSpellChecker::on_insert_text_after), true);

  m_buffer->signal_erase().connect(
      sigc::mem_fun(*this, &AutomaticSpellChecker::on_erase), true);

  m_buffer->signal_mark_set().connect(
      sigc::mem_fun(*this, &AutomaticSpellChecker::on_mark_set));

  // Create highlight tag
  m_tag_highlight = m_buffer->create_tag("asc-misspelled");
  m_tag_highlight->property_underline() = Pango::UNDERLINE_ERROR;

  // Tag table signals
  Glib::RefPtr<Gtk::TextBuffer::TagTable> tag_table = m_buffer->get_tag_table();

  tag_table->signal_tag_added().connect(
      sigc::mem_fun(*this, &AutomaticSpellChecker::on_tag_added_or_removed));

  tag_table->signal_tag_removed().connect(
      sigc::mem_fun(*this, &AutomaticSpellChecker::on_tag_added_or_removed));

  tag_table->signal_tag_changed().connect(
      sigc::mem_fun(*this, &AutomaticSpellChecker::on_tag_changed));

  // Create marks
  Gtk::TextIter start, end;
  m_buffer->get_bounds(start, end);

  m_mark_insert_start = m_buffer->create_mark("asc-insert-start", start, true);
  m_mark_insert_end = m_buffer->create_mark("asc-insert-end", start, true);
  m_mark_click = m_buffer->create_mark("asc-click", start, true);

  m_deferred_check = false;

  // Attach to the view
  view->signal_button_press_event().connect(
      sigc::mem_fun(*this, &AutomaticSpellChecker::on_button_press_event),
      false);
  view->signal_populate_popup().connect(
      sigc::mem_fun(*this, &AutomaticSpellChecker::on_populate_popup));
  view->signal_popup_menu().connect(
      sigc::mem_fun(*this, &AutomaticSpellChecker::on_popup_menu));
}

Glib::RefPtr<Gtk::TextBuffer> AutomaticSpellChecker::get_buffer() {
  return m_textview->get_buffer();
}

// Insertion works like this:
// - before the text is inserted, we mark the position in the buffer.
// - after the text is inserted, we see where our mark is and use that and
//   the current position to check the entire range of inserted text.
void AutomaticSpellChecker::on_insert_text_before(
    const Gtk::TextBuffer::iterator &pos, const Glib::ustring & /*text*/,
    int /*bytes*/) {
  Glib::RefPtr<Gtk::TextBuffer> m_buffer = get_buffer();
  m_buffer->move_mark(m_mark_insert_start, pos);
}

// Insertion works like this:
// - before the text is inserted, we mark the position in the buffer.
// - after the text is inserted, we see where our mark is and use that and
//   the current position to check the entire range of inserted text.
void AutomaticSpellChecker::on_insert_text_after(
    const Gtk::TextBuffer::iterator &pos, const Glib::ustring & /*text*/,
    int /*bytes*/) {
  Glib::RefPtr<Gtk::TextBuffer> m_buffer = get_buffer();

  Gtk::TextIter start;
  // we need to check a range of text
  start = m_buffer->get_iter_at_mark(m_mark_insert_start);

  check_range(start, pos, false);

  m_buffer->move_mark(m_mark_insert_end, pos);
}

// Deleting is more simple:  we're given the range of deleted text.
// After deletion, the start and end iters should be at the same position
// (because all of the text between them was deleted!).
// This means we only really check the words immediately bounding the deletion.
void AutomaticSpellChecker::on_erase(const Gtk::TextBuffer::iterator &start,
                                     const Gtk::TextBuffer::iterator &end) {
  check_range(start, end, false);
}

void AutomaticSpellChecker::on_mark_set(
    const Gtk::TextBuffer::iterator & /*location*/,
    const Glib::RefPtr<Gtk::TextBuffer::Mark> &mark) {
  Glib::RefPtr<Gtk::TextBuffer> m_buffer = get_buffer();

  // if the cursor has moved and there is a deferred check so handle it now
  if ((mark == m_buffer->get_insert()) && m_deferred_check)
    check_deferred_range(false);
}

// Set the tag 'highlight' as priority.
void AutomaticSpellChecker::tag_table_changed() {
  Glib::RefPtr<Gtk::TextBuffer> m_buffer = get_buffer();

  Glib::RefPtr<Gtk::TextBuffer::TagTable> tag_table = m_buffer->get_tag_table();
  m_tag_highlight->set_priority(tag_table->get_size() - 1);
}

// Update the tag 'highlight' priority.
void AutomaticSpellChecker::on_tag_changed(
    const Glib::RefPtr<Gtk::TextTag> & /*tag*/, bool /*size_changed*/) {
  tag_table_changed();
}

// Update the tag 'highlight' priority.
void AutomaticSpellChecker::on_tag_added_or_removed(
    const Glib::RefPtr<Gtk::TextTag> & /*tag*/) {
  tag_table_changed();
}

// Set iterators with the word from mark.
void AutomaticSpellChecker::get_word_extents_from_mark(
    const Glib::RefPtr<Gtk::TextBuffer::Mark> &mark, Gtk::TextIter &start,
    Gtk::TextIter &end) {
  Glib::RefPtr<Gtk::TextBuffer> m_buffer = get_buffer();

  start = m_buffer->get_iter_at_mark(mark);

  if (!start.starts_word())
    iter_backward_word_start(start);

  end = start;

  if (end.inside_word())
    iter_forward_word_end(end);
}

// Check the word delimited by the iterators and if it's misspell tag it.
void AutomaticSpellChecker::check_word(Gtk::TextIter start, Gtk::TextIter end) {
  Glib::RefPtr<Gtk::TextBuffer> m_buffer = get_buffer();

  Glib::ustring word = m_buffer->get_text(start, end, false);

  if (!SpellChecker::instance()->check(word)) {
    m_buffer->apply_tag(m_tag_highlight, start, end);
  }
}

void AutomaticSpellChecker::check_deferred_range(bool force_all) {
  Glib::RefPtr<Gtk::TextBuffer> m_buffer = get_buffer();

  Gtk::TextIter start, end;
  start = m_buffer->get_iter_at_mark(m_mark_insert_start);
  end = m_buffer->get_iter_at_mark(m_mark_insert_end);

  check_range(start, end, force_all);
}

// Check words delimited by the iterators.
void AutomaticSpellChecker::check_range(Gtk::TextIter start, Gtk::TextIter end,
                                        bool force_all) {
  // we need to "split" on word boundaries.
  // luckily, Pango knows what "words" are
  // so we don't have to figure it out.
  Gtk::TextIter wstart, wend, cursor, precursor;
  bool highlight;

  if (end.inside_word())
    iter_forward_word_end(end);

  if (!start.starts_word()) {
    if (start.inside_word() || start.ends_word()) {
      iter_backward_word_start(start);
    } else {
      // if we're neither at the beginning nor inside a word,
      // me must be in some spaces.
      // skip forward to the beginning of the next word.
      if (iter_forward_word_end(start))
        iter_backward_word_start(start);
    }
  }

  Glib::RefPtr<Gtk::TextBuffer> m_buffer = get_buffer();

  cursor = m_buffer->get_iter_at_mark(m_buffer->get_insert());

  precursor = cursor;
  precursor.backward_char();

  highlight =
      cursor.has_tag(m_tag_highlight) || precursor.has_tag(m_tag_highlight);

  m_buffer->remove_tag(m_tag_highlight, start, end);

  // Fix a corner case when replacement occurs at beginning of buffer:
  // An iter at offset 0 seems to always be inside a word,
  // even if it's not.  Possibly a pango bug.
  if (start.get_offset() == 0) {
    iter_forward_word_end(start);
    iter_backward_word_start(start);
  }

  wstart = start;

  while (wstart.compare(end) < 0) {
    // move wend to the end of the current word.
    wend = wstart;
    iter_forward_word_end(wend);

    bool inword = (wstart.compare(cursor) < 0) && (cursor.compare(wend) < 0);

    if (inword && !force_all) {
      // this word is being actively edited,
      // only check if it's already highlighted,
      // otherwise defer this check until later.
      if (highlight)
        check_word(wstart, wend);
      else
        m_deferred_check = true;
    } else {
      check_word(wstart, wend);
      m_deferred_check = false;
    }

    // now move wend to the beginning of the next word,
    iter_forward_word_end(wend);
    iter_backward_word_start(wend);

    // make sure we've actually advanced
    // (we don't advance in some corner cases),
    if (wstart.compare(wend) == 0)
      break;  // we're done in these cases..

    // and then pick this as the new next word beginning.
    wstart = wend;
  }
}

// Recheck all the textbuffer.
void AutomaticSpellChecker::recheck_all() {
  Gtk::TextIter start, end;

  get_buffer()->get_bounds(start, end);

  check_range(start, end, true);
}

// Update the mark click.
bool AutomaticSpellChecker::on_popup_menu() {
  Gtk::TextIter iter;
  Glib::RefPtr<Gtk::TextBuffer> m_buffer = get_buffer();

  // handle deferred check if it exists
  if (m_deferred_check)
    check_deferred_range(true);

  iter = m_buffer->get_iter_at_mark(m_buffer->get_insert());
  m_buffer->move_mark(m_mark_click, iter);
  return false;
}

// Build spell check menu.
// The menu 'languages' is always added to allow the user to change the
// dictionary without misspell word unlike the menu 'suggestions' added only if
// the word is misspell.
void AutomaticSpellChecker::on_populate_popup(Gtk::Menu *menu) {
  Gtk::Image *img;
  Gtk::MenuItem *mi;
  Gtk::TextIter start, end;
  Glib::ustring word;

  // menu separator comes first.
  mi = manage(new Gtk::MenuItem);
  mi->show();
  menu->prepend(*mi);

  // the languages menu.
  img = manage(new Gtk::Image(Gtk::Stock::SPELL_CHECK, Gtk::ICON_SIZE_MENU));
  mi = manage(new Gtk::ImageMenuItem(*img, _("_Languages"), true));
  mi->set_submenu(*build_languages_menu());
  mi->show_all();
  menu->prepend(*mi);

  // we need to figure out if they picked a misspelled word.
  get_word_extents_from_mark(m_mark_click, start, end);

  // if our highlight algorithm ever messes up,
  // this isn't correct, either.
  if (!start.has_tag(m_tag_highlight))
    return;  // word wasn't misspelled

  // build suggestions from the misspelled word
  word = get_buffer()->get_text(start, end, false);

  // mi->set_submenu(*build_suggestion_menu(word));
  build_suggestion_menu(word, menu);
}

// When the user right-clicks on a word, they want to check that word.
// Here, we do NOT  move the cursor to the location of the clicked-upon word
// since that prevents the use of edit functions on the context menu.
bool AutomaticSpellChecker::on_button_press_event(GdkEventButton *ev) {
  if (ev->button == 3) {
    gint x, y;
    Gtk::TextIter iter;

    Glib::RefPtr<Gtk::TextBuffer> m_buffer = get_buffer();

    // handle deferred check if it exists
    if (m_deferred_check)
      check_deferred_range(true);

    m_textview->window_to_buffer_coords(Gtk::TEXT_WINDOW_TEXT,
                                        static_cast<int>(ev->x),
                                        static_cast<int>(ev->y), x, y);
    m_textview->get_iter_at_location(iter, x, y);

    m_buffer->move_mark(m_mark_click, iter);
  }
  // return false: let gtk process this event, too.
  // we don't want to eat any events.
  return false;
}

// Build a suggestions menu from misspelled word, 'menu' is directly used to add
// menu items.
// 'Ignore All' and 'Add "%" to Dictionary" are always created.
// Create suggestions list, if they are more than ten, create submenu 'More...'
// for they.
void AutomaticSpellChecker::build_suggestion_menu(const Glib::ustring &word,
                                                  Gtk::Menu *menu) {
  Gtk::MenuItem *mi;

  // separator
  mi = manage(new Gtk::MenuItem);
  mi->show();
  menu->prepend(*mi);
  // ignore all
  mi = manage(new Gtk::ImageMenuItem(
      *manage(new Gtk::Image(Gtk::Stock::REMOVE, Gtk::ICON_SIZE_MENU)),
      _("_Ignore all"), true));
  mi->signal_activate().connect(
      sigc::mem_fun(*this, &AutomaticSpellChecker::on_ignore_all));
  mi->show();
  menu->prepend(*mi);
  // add to dictionary
  mi = manage(new Gtk::ImageMenuItem(
      *manage(new Gtk::Image(Gtk::Stock::ADD, Gtk::ICON_SIZE_MENU)),
      Glib::ustring::compose(_("_Add \"%1\" to Dictionary"), word), true));
  mi->signal_activate().connect(
      sigc::mem_fun(*this, &AutomaticSpellChecker::on_add_to_dictionary));
  mi->show();
  menu->prepend(*mi);

  // suggestions
  std::vector<Glib::ustring> suggestions =
      SpellChecker::instance()->get_suggest(word);

  if (suggestions.empty()) {
    Gtk::Label *label = manage(new Gtk::Label);
    label->set_text(_("(no suggested words)"));
    label->set_alignment(Gtk::ALIGN_START, Gtk::ALIGN_CENTER);

    mi = manage(new Gtk::MenuItem);
    mi->set_sensitive(false);
    mi->add(*label);
    mi->show_all();
    menu->prepend(*mi);
  } else {
    // Need to reverse the list as we add items using preprend to
    // respects the order of the suggestions
    std::reverse(suggestions.begin(), suggestions.end());

    Gtk::Menu *me = menu;

    for (unsigned int i = 0; i < suggestions.size(); ++i) {
      if (i != 0 && i % 10 == 0) {
        // separator
        mi = manage(new Gtk::MenuItem);
        me->prepend(*mi);
        // menu more
        mi = manage(new Gtk::ImageMenuItem(
            *manage(
                new Gtk::Image(Gtk::Stock::SPELL_CHECK, Gtk::ICON_SIZE_MENU)),
            _("_More..."), true));
        mi->show_all();

        me->prepend(*mi);
        // create and add submenu to 'More...'
        me = manage(new Gtk::Menu);
        me->show();
        mi->set_submenu(*me);
      }

      Gtk::Label *label = manage(new Gtk::Label);
      label->set_text(Glib::ustring::compose("<b>%1</b>", suggestions[i]));
      label->set_alignment(Gtk::ALIGN_START, Gtk::ALIGN_CENTER);
      label->set_use_markup(true);

      mi = manage(new Gtk::MenuItem);
      mi->signal_activate().connect(sigc::bind(
          sigc::mem_fun(*this, &AutomaticSpellChecker::on_replace_word),
          suggestions[i]));
      mi->add(*label);
      mi->show_all();
      me->prepend(*mi);
    }
  }
}

// We get the word from the 'mark click' and we replace it by the 'newword'.
// Store the replacement in the SpellChecker, that future occurrences of the
// word will be replaced with the 'newword'.
// It's call by the item spell suggestions.
void AutomaticSpellChecker::on_replace_word(const Glib::ustring &newword) {
  if (newword.empty())
    return;

  Gtk::TextIter start, end;

  Glib::RefPtr<Gtk::TextBuffer> m_buffer = get_buffer();

  get_word_extents_from_mark(m_mark_click, start, end);

  Glib::ustring oldword = m_buffer->get_text(start, end, false);

  m_buffer->begin_user_action();
  start = m_buffer->erase(start, end);
  m_buffer->insert(start, newword);
  m_buffer->end_user_action();

  SpellChecker::instance()->store_replacement(oldword, newword);
}

// We get the word from the 'mark click' and we add the word to
// the session of the spell checker.
// It will be ignored in the future the time of the session.
// It's call by the suggestions item "Ignore all".
void AutomaticSpellChecker::on_ignore_all() {
  Gtk::TextIter start, end;

  Glib::RefPtr<Gtk::TextBuffer> m_buffer = get_buffer();

  get_word_extents_from_mark(m_mark_click, start, end);

  Glib::ustring word = m_buffer->get_text(start, end, false);

  SpellChecker::instance()->add_word_to_session(word);

  recheck_all();
}

// We get the word from the 'mark click' and we add the word to
// the personal dictionary.
// It will be ignored in the future.
// It's call by the suggestions item "Add %s to Dictionary".
void AutomaticSpellChecker::on_add_to_dictionary() {
  Gtk::TextIter start, end;

  Glib::RefPtr<Gtk::TextBuffer> m_buffer = get_buffer();

  get_word_extents_from_mark(m_mark_click, start, end);

  Glib::ustring word = m_buffer->get_text(start, end, false);

  SpellChecker::instance()->add_word_to_personal(word);

  recheck_all();
}

// Create and return a languages menu.
Gtk::Menu *AutomaticSpellChecker::build_languages_menu() {
  Gtk::Menu *menu;
  Gtk::MenuItem *mi;

  menu = manage(new Gtk::Menu);

  // Get dictionaries available
  std::vector<Glib::ustring> dictionaries =
      SpellChecker::instance()->get_dictionaries();

  // Transform isocode to human label ex: 'fr_fr' to 'French (France)'
  std::map<Glib::ustring, Glib::ustring> languages;

  for (unsigned int i = 0; i < dictionaries.size(); ++i) {
    Glib::ustring isocode = dictionaries[i];
    Glib::ustring name = isocodes::to_name(isocode);

    languages[name] = isocode;
  }

  Glib::ustring current = SpellChecker::instance()->get_dictionary();

  // Create menu items
  for (const auto &lang_map : languages) {
    if (lang_map.second == current) {
      mi = manage(new Gtk::ImageMenuItem(
          *manage(new Gtk::Image(Gtk::Stock::APPLY, Gtk::ICON_SIZE_MENU)),
          lang_map.first, true));
    } else {
      mi = manage(new Gtk::MenuItem(lang_map.first));
    }
    mi->signal_activate().connect(sigc::bind(
        sigc::mem_fun(*this, &AutomaticSpellChecker::on_set_current_language),
        lang_map.second));
    menu->append(*mi);
  }

  menu->show_all();
  return menu;
}

// Update the spellchecker with a new dictionary.
void AutomaticSpellChecker::on_set_current_language(
    const Glib::ustring &isocode) {
  SpellChecker::instance()->set_dictionary(isocode);
  // FIXME: connect to SpellChecker ?
  recheck_all();
}

// heuristic:
// if we're on an singlequote/apostrophe and
// if the next letter is alphanumeric, this is an apostrophe.
bool AutomaticSpellChecker::iter_forward_word_end(Gtk::TextIter &i) {
  if (!i.forward_word_end())
    return false;
  if (i.get_char() != '\'')
    return true;

  Gtk::TextIter iter = i;
  if (iter.forward_char())
    if (g_unichar_isalpha(iter.get_char()))
      return i.forward_word_end();
  return true;
}

// heuristic:
// if we're on an singlequote/apostrophe and
// if the next letter is alphanumeric, this is an apostrophe.
bool AutomaticSpellChecker::iter_backward_word_start(Gtk::TextIter &i) {
  if (!i.backward_word_start())
    return false;

  Gtk::TextIter iter = i;
  if (iter.backward_char())
    if (iter.get_char() == '\'')
      if (iter.backward_char())
        if (g_unichar_isalpha(iter.get_char()))
          return i.backward_word_start();
  return true;
}
