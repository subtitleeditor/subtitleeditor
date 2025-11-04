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

#include <extension/action.h>
#include <gtkmm_utility.h>
#include <isocodes.h>
#include <spellchecker.h>
#include <utility.h>

#include <memory>

class DialogSpellChecking : public Gtk::Dialog {
  class ComboBoxLanguages : public Gtk::ComboBox {
    class Column : public Gtk::TreeModel::ColumnRecord {
     public:
      Column() {
        add(label);
        add(isocode);
      }
      Gtk::TreeModelColumn<Glib::ustring> label;
      Gtk::TreeModelColumn<Glib::ustring> isocode;
    };

   public:
    ComboBoxLanguages(BaseObjectType* cobject,
                      const Glib::RefPtr<Gtk::Builder>&)
        : Gtk::ComboBox(cobject) {
      liststore = Gtk::ListStore::create(column);
      set_model(liststore);

      Gtk::CellRendererText* renderer = manage(new Gtk::CellRendererText);
      pack_start(*renderer);
      add_attribute(*renderer, "text", 0);

      liststore->set_sort_column(0, Gtk::SORT_ASCENDING);
    }

    void append_lang(const Glib::ustring& isocode) {
      Gtk::TreeIter it = liststore->append();
      (*it)[column.isocode] = isocode;
      (*it)[column.label] = isocodes::to_name(isocode);
    }

    Glib::ustring get_active_lang() {
      Gtk::TreeIter it = get_active();
      if (it)
        return (*it)[column.isocode];
      return Glib::ustring();
    }

    bool set_active_lang(const Glib::ustring& isocode) {
      Gtk::TreeIter it = liststore->children().begin();
      while (it) {
        if ((*it)[column.isocode] == isocode) {
          set_active(it);
          return true;
        }
        ++it;
      }
      return false;
    }

   protected:
    Column column;
    Glib::RefPtr<Gtk::ListStore> liststore;
  };

  class SuggestionColumn : public Gtk::TreeModel::ColumnRecord {
   public:
    SuggestionColumn() {
      add(string);
    }
    Gtk::TreeModelColumn<Glib::ustring> string;
  };

 public:
  DialogSpellChecking(BaseObjectType* cobject,
                      const Glib::RefPtr<Gtk::Builder>& xml)
      : Gtk::Dialog(cobject),
        m_current_document(NULL),
        m_current_column("text") {
    se_dbg_msg(SE_DBG_SPELL_CHECKING, "create spellchecking dialog...");

    utility::set_transient_parent(*this);

    xml->get_widget_derived("combobox-languages", m_comboLanguages);

    xml->get_widget("textview", m_textview);
    xml->get_widget("entry-replace-with", m_entryReplaceWith);
    xml->get_widget("button-check-word", m_buttonCheckWord);

    xml->get_widget("treeview-suggestions", m_treeviewSuggestions);

    xml->get_widget("button-replace", m_buttonReplace);
    xml->get_widget("button-ignore", m_buttonIgnore);
    xml->get_widget("button-ignore-all", m_buttonIgnoreAll);
    xml->get_widget("button-add-word", m_buttonAddWord);

    setup_languages();
    setup_signals();
    setup_text_view();
    setup_suggestions_view();
  }

  void execute(Document* doc) {
    g_return_if_fail(doc);

    m_current_document = doc;
    // If the current focus is in the translation column,
    // check the translation. default value is "text".
    if (doc->get_current_column_name() == "translation")
      m_current_column = "translation";

    show_column_warning();

    m_current_sub = doc->subtitles().get_first();

    init_text_view_with_subtitle(m_current_sub);
    update_status_from_replace_word();  // init the state of buttons
    check_next_word();

    doc->start_command(_("Spell Checking"));
    run();
    doc->finish_command();
  }

 protected:
  // Information: Display a message about how-to spellcheck
  // the translation column.
  void show_column_warning() {
    // Don't disable this message again option
    if (cfg::has_key("spell-checking", "disable-column-warning")) {
      if (cfg::get_boolean("spell-checking", "disable-column-warning")) {
        return;
      }
    }

    Glib::ustring msg(
        _("The spell check is applied to the column \"text\" as default. "
          "You can check the column \"translation\" by setting the focus "
          "to this column before starting the spell check."));

    Gtk::MessageDialog dialog(msg);

    Gtk::CheckButton checkDisable(_("_Do not show this message again"), true);
    checkDisable.show();
    dialog.get_vbox()->pack_start(checkDisable, false, false);

    dialog.run();
    // Save the status if it's activated
    if (checkDisable.get_active()) {
      cfg::set_boolean("spell-checking", "disable-column-warning", true);
    }
  }

  void setup_languages() {
    se_dbg_msg(SE_DBG_SPELL_CHECKING, "setup languages dictionaries");

    // Add dictionaries
    auto dicts = SpellChecker::instance()->get_dictionaries();
    for (const auto& dictionary : dicts) {
      m_comboLanguages->append_lang(dictionary);
    }
    // Set the current dictionary
    m_comboLanguages->set_active_lang(
        SpellChecker::instance()->get_dictionary());

    m_comboLanguages->signal_changed().connect(
        sigc::mem_fun(*this, &DialogSpellChecking::on_combo_languages_changed));
  }

  void setup_signals() {
    se_dbg_msg(SE_DBG_SPELL_CHECKING, "setup signals (buttons ...)");

    m_buttonCheckWord->signal_clicked().connect(
        sigc::mem_fun(*this, &DialogSpellChecking::on_check_word));

    m_buttonReplace->signal_clicked().connect(
        sigc::mem_fun(*this, &DialogSpellChecking::on_replace));

    m_buttonIgnore->signal_clicked().connect(
        sigc::mem_fun(*this, &DialogSpellChecking::on_ignore));

    m_buttonIgnoreAll->signal_clicked().connect(
        sigc::mem_fun(*this, &DialogSpellChecking::on_ignore_all));

    m_buttonAddWord->signal_clicked().connect(
        sigc::mem_fun(*this, &DialogSpellChecking::on_add_word_to_dictionary));

    //
    m_entryReplaceWith->signal_changed().connect(sigc::mem_fun(
        *this, &DialogSpellChecking::update_status_from_replace_word));

    m_entryReplaceWith->signal_activate().connect(
        sigc::mem_fun(*this, &DialogSpellChecking::on_replace));
  }

  void setup_text_view() {
    se_dbg_msg(SE_DBG_SPELL_CHECKING,
               "setup textview, create highlight tag and marks");

    m_textview->set_editable(false);
    m_textview->set_sensitive(false);

    m_buffer = m_textview->get_buffer();
    // create misspelled tag
    m_tag_highlight = m_buffer->create_tag("misspelled");
    m_tag_highlight->property_foreground() = "red";
    m_tag_highlight->property_weight() = Pango::WEIGHT_BOLD;
    // create start/end marks
    m_mark_start = m_buffer->create_mark("mark-start", m_buffer->begin(), true);
    m_mark_end = m_buffer->create_mark("mark-end", m_buffer->begin(), true);
  }

  // Create a model and a column "Suggestions" and connect
  // the signal selection-changed to update the text value
  // of the widget m_entryReplaceWith.
  void setup_suggestions_view() {
    se_dbg_msg(SE_DBG_SPELL_CHECKING,
               "create model and view for the suggestions");

    SuggestionColumn column;
    m_listSuggestions = Gtk::ListStore::create(column);
    m_treeviewSuggestions->set_model(m_listSuggestions);

    Gtk::TreeViewColumn* col =
        manage(new Gtk::TreeViewColumn(_("Suggestions")));
    Gtk::CellRendererText* renderer = manage(new Gtk::CellRendererText);

    col->pack_start(*renderer, false);
    col->add_attribute(renderer->property_text(), column.string);
    m_treeviewSuggestions->append_column(*col);

    m_treeviewSuggestions->get_selection()->signal_changed().connect(
        sigc::mem_fun(*this,
                      &DialogSpellChecking::on_suggestions_selection_changed));

    m_treeviewSuggestions->signal_row_activated().connect(sigc::mem_fun(
        *this, &DialogSpellChecking::on_suggestions_row_activated));
  }

  // Initialize the textview with the current subtitle text (text or
  // translation); update the mark to the beginning.
  bool init_text_view_with_subtitle(const Subtitle& sub) {
    if (!sub) {
      se_dbg_msg(SE_DBG_SPELL_CHECKING, "Subtitle is not valid");
      return false;
    }
    // Check the translation or the text column.
    Glib::ustring text = (m_current_column == "translation")
                             ? sub.get_translation()
                             : sub.get_text();

    se_dbg_msg(SE_DBG_SPELL_CHECKING,
               "Update the textview with (%s column): '%s'",
               m_current_column.c_str(), text.c_str());

    m_buffer->set_text(text);
    m_textview->set_sensitive(!text.empty());
    // move the marks to the beginning
    Gtk::TextIter begin = m_buffer->begin();
    m_buffer->move_mark(m_mark_start, begin);
    m_buffer->move_mark(m_mark_end, begin);
    return true;
  }

  // Update the subtitle text (or translation)
  // from the current text in the textview.
  void update_subtitle_from_text_view() {
    if (!m_current_sub)
      return;

    Glib::ustring text = m_buffer->get_text();

    se_dbg_msg(SE_DBG_SPELL_CHECKING,
               "the subtitle (%s) is update with the text '%s'",
               m_current_column.c_str(), text.c_str());

    if (m_current_column == "translation") {
      if (m_current_sub.get_translation() != text)
        m_current_sub.set_translation(text);
    } else {  // "text"
      if (m_current_sub.get_text() != text)
        m_current_sub.set_text(text);
    }
  }

  // Initialize the list with the suggestions array.
  void init_suggestions(const Glib::ustring& word) {
    se_dbg_msg(SE_DBG_SPELL_CHECKING,
               "initialize the suggestion with the word '%s'", word.c_str());

    m_entryReplaceWith->set_text("");
    m_listSuggestions->clear();

    if (word.empty())
      return;

    auto suggs = SpellChecker::instance()->get_suggest(word);

    SuggestionColumn column;

    for (const auto& suggesion : suggs) {
      Gtk::TreeIter it = m_listSuggestions->append();
      (*it)[column.string] = suggesion;

      se_dbg_msg(SE_DBG_SPELL_CHECKING, "suggested word: '%s'",
                 suggesion.c_str());
    }
  }

  bool next_check() {
    if (check_next_word())
      return true;
    return check_next_subtitle();
  }

  bool check_next_subtitle() {
    if (!m_current_sub || !(++m_current_sub)) {
      completed_spell_changed();
      return false;
    }
    init_text_view_with_subtitle(m_current_sub);
    return next_check();
  }

  bool iter_forward_word_end(Gtk::TextIter& i) {
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

  bool iter_backward_word_start(Gtk::TextIter& i) {
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

  // return True if there is misspelled word.
  bool check_next_word() {
    Gtk::TextIter start = m_buffer->begin();
    Gtk::TextIter end = m_buffer->end();

    m_buffer->remove_tag(m_tag_highlight, start, end);

    Gtk::TextIter wstart, wend;

    // Start at the mark_end, go to the next word
    wstart = m_mark_end->get_iter();
    if (!iter_forward_word_end(wstart) || !iter_backward_word_start(wstart))
      return check_next_subtitle();

    while (wstart.compare(end) < 0) {  // && wstart.compare(end) != 0)
      // move wend to the end of the current word
      wend = wstart;
      iter_forward_word_end(wend);

      // Check the word
      if (is_misspelled(wstart, wend))
        return true;  // misspelled word

      // Good word
      // so we move wend to the beginning of the next word
      iter_forward_word_end(wend);
      iter_backward_word_start(wend);

      if (wstart.compare(wend) == 0)
        break;

      // and then pick this as the new next word beginning.
      wstart = wend;
    }
    return check_next_subtitle();
  }

  // Check the word (start, end)
  // Return true if the word is misspelled.
  bool is_misspelled(Gtk::TextIter start, Gtk::TextIter end) {
    Glib::ustring word = m_textview->get_buffer()->get_text(start, end, false);

    se_dbg_msg(SE_DBG_SPELL_CHECKING, "check the word : '%s'", word.c_str());

    if (SpellChecker::instance()->check(word)) {
      se_dbg_msg(SE_DBG_SPELL_CHECKING, "the word '%s' is not misspelled",
                 word.c_str());
      return false;
    }
    se_dbg_msg(SE_DBG_SPELL_CHECKING, "the word '%s' is misspelled",
               word.c_str());

    m_buffer->apply_tag(m_tag_highlight, start, end);
    m_buffer->move_mark(m_mark_start, start);
    m_buffer->move_mark(m_mark_end, end);

    // Update the suggestions list
    init_suggestions(word);
    // Select the subtitle if it's not already select
    if (!m_current_document->subtitles().is_selected(m_current_sub))
      m_current_document->subtitles().select(m_current_sub);
    return true;
  }

  // Return the current word misspelled (the mark tags are used)
  Glib::ustring get_current_word() {
    Gtk::TextIter start = m_mark_start->get_iter();
    Gtk::TextIter end = m_mark_end->get_iter();

    Glib::ustring word = m_textview->get_buffer()->get_text(start, end, false);

    se_dbg_msg(SE_DBG_SPELL_CHECKING, "the current word is '%s'", word.c_str());

    return word;
  }

  // Callbacks

  // Used by the button "Check Word" (m_buttonCheckWord)
  // Check the word from the entry "Replace With:" (m_entryReplaceWith)
  // and display show the suggestions.
  void on_check_word() {
    se_dbg(SE_DBG_SPELL_CHECKING);

    Glib::ustring newword = m_entryReplaceWith->get_text();
    init_suggestions(newword);
    // Because init_suggestions clean the widget with empty text,
    // we need to reset it with the neword
    m_entryReplaceWith->set_text(newword);
  }

  // Replace the word by the selected suggestion (m_entryReplaceWith)
  void on_replace() {
    se_dbg(SE_DBG_SPELL_CHECKING);

    Glib::ustring newword = m_entryReplaceWith->get_text();
    if (newword.empty())
      return;

    Gtk::TextIter start = m_mark_start->get_iter();
    Gtk::TextIter end = m_mark_end->get_iter();

    Glib::ustring oldword = m_buffer->get_text(start, end, false);

    se_dbg_msg(SE_DBG_SPELL_CHECKING,
               "replace the word '%s' by the new word '%s'", oldword.c_str(),
               newword.c_str());

    m_buffer->begin_user_action();
    start = m_buffer->erase(start, end);
    end = m_buffer->insert(start, newword);
    m_buffer->end_user_action();

    // update the mark_end, the mark_start doesn't move
    m_buffer->move_mark(m_mark_end, end);

    SpellChecker::instance()->store_replacement(oldword, newword);

    // we update the subtitle with the text changed
    update_subtitle_from_text_view();

    next_check();
  }

  // Ignore the word and just go to the next word.
  void on_ignore() {
    se_dbg_msg(SE_DBG_SPELL_CHECKING, "ignore the word '%s'",
               get_current_word().c_str());

    next_check();
  }

  // Ignore the word the time of the session.
  void on_ignore_all() {
    Glib::ustring word = get_current_word();

    se_dbg_msg(SE_DBG_SPELL_CHECKING,
               "ignore all the word '%s' by adding the word to the session",
               word.c_str());

    SpellChecker::instance()->add_word_to_session(word);
    next_check();
  }

  // Add the misspelled word to the personal dictionary
  void on_add_word_to_dictionary() {
    Glib::ustring word = get_current_word();

    se_dbg_msg(SE_DBG_SPELL_CHECKING,
               "add the word '%s' to the personal dictionary", word.c_str());

    SpellChecker::instance()->add_word_to_personal(word);

    next_check();
  }

  // Update the entry "Replace With:" with the
  // current selected suggestion.
  void on_suggestions_selection_changed() {
    Gtk::TreeIter it = m_treeviewSuggestions->get_selection()->get_selected();
    if (it) {
      SuggestionColumn col;
      Glib::ustring word = (*it)[col.string];

      m_entryReplaceWith->set_text(word);
    }
  }

  // Double click on the suggestion replace with the word activated
  void on_suggestions_row_activated(const Gtk::TreeModel::Path& path,
                                    Gtk::TreeViewColumn*) {
    Gtk::TreeIter it = m_listSuggestions->get_iter(path);
    if (it) {
      SuggestionColumn col;
      Glib::ustring word = (*it)[col.string];

      m_entryReplaceWith->set_text(word);

      on_replace();
    }
  }

  // The user change the current dictionary.
  // We need to recheck the current word after we have setting a new dictionary.
  void on_combo_languages_changed() {
    Glib::ustring lang = m_comboLanguages->get_active_lang();
    if (lang == SpellChecker::instance()->get_dictionary())
      return;

    SpellChecker::instance()->set_dictionary(lang);
    // recheck the current word and if it's not misspelled check the next word
    if (!is_misspelled(m_mark_start->get_iter(), m_mark_end->get_iter()))
      next_check();
  }

  // Update the sensitivity of some buttons
  // from the status of the entry "Replace With:" (empty or not)
  void update_status_from_replace_word() {
    bool state = !m_entryReplaceWith->get_text().empty();

    se_dbg_msg(SE_DBG_SPELL_CHECKING, "set sensitive to %s",
               (state) ? "true" : "false");

    m_buttonCheckWord->set_sensitive(state);
    m_buttonReplace->set_sensitive(state);
  }

  // Disable the interface and display a message:
  // "Completed spell checking."
  void completed_spell_changed() {
    se_dbg_msg(SE_DBG_SPELL_CHECKING,
               "completed spell checking, disable the ui.");

    m_comboLanguages->set_sensitive(false);

    m_textview->set_sensitive(false);
    m_buffer->set_text(_("Completed spell checking."));

    m_entryReplaceWith->set_sensitive(false);
    m_buttonCheckWord->set_sensitive(false);

    m_treeviewSuggestions->set_sensitive(false);
    m_buttonIgnore->set_sensitive(false);
    m_buttonIgnoreAll->set_sensitive(false);
    m_buttonReplace->set_sensitive(false);
    m_buttonAddWord->set_sensitive(false);
  }

 protected:
  ComboBoxLanguages* m_comboLanguages;

  Gtk::TextView* m_textview;
  Glib::RefPtr<Gtk::TextBuffer> m_buffer;
  Glib::RefPtr<Gtk::TextMark> m_mark_start;
  Glib::RefPtr<Gtk::TextMark> m_mark_end;
  Glib::RefPtr<Gtk::TextTag> m_tag_highlight;

  Gtk::Entry* m_entryReplaceWith;
  Gtk::TreeView* m_treeviewSuggestions;
  Glib::RefPtr<Gtk::ListStore> m_listSuggestions;

  Gtk::Button* m_buttonCheckWord;
  Gtk::Button* m_buttonReplace;
  Gtk::Button* m_buttonIgnore;
  Gtk::Button* m_buttonIgnoreAll;
  Gtk::Button* m_buttonAddWord;

  Document* m_current_document;
  Glib::ustring m_current_column;
  Subtitle m_current_sub;
};

class SpellCheckingPlugin : public Action {
 public:
  SpellCheckingPlugin() {
    activate();
    update_ui();
  }

  ~SpellCheckingPlugin() {
    deactivate();
  }

  void activate() {
    se_dbg(SE_DBG_PLUGINS);

    // actions
    action_group = Gtk::ActionGroup::create("SpellCheckingPlugin");

    action_group->add(
        Gtk::Action::create("spell-checking", Gtk::Stock::SPELL_CHECK,
                            _("_Spell Check"),
                            _("Launch the spellchecking dialog")),
        Gtk::AccelKey("F7"),
        sigc::mem_fun(*this, &SpellCheckingPlugin::on_execute));

    // ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui_id = ui->new_merge_id();

    ui->insert_action_group(action_group);

    ui->add_ui(ui_id, "/menubar/menu-tools/checking", "spell-checking",
               "spell-checking");
  }

  void deactivate() {
    se_dbg(SE_DBG_PLUGINS);

    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui->remove_ui(ui_id);
    ui->remove_action_group(action_group);
  }

  void update_ui() {
    se_dbg(SE_DBG_PLUGINS);

    bool visible = (get_current_document() != NULL);

    action_group->get_action("spell-checking")->set_sensitive(visible);
  }

 protected:
  void on_execute() {
    se_dbg(SE_DBG_PLUGINS);

    Document* doc = get_current_document();
    g_return_if_fail(doc);

    // create dialog
    std::unique_ptr<DialogSpellChecking> dialog(
        gtkmm_utility::get_widget_derived<DialogSpellChecking>(
            SE_DEV_VALUE(SE_PLUGIN_PATH_UI, SE_PLUGIN_PATH_DEV),
            "dialog-spell-checking.ui", "dialog-spell-checking"));

    dialog->execute(doc);
  }

 protected:
  Gtk::UIManager::ui_merge_id ui_id;
  Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(SpellCheckingPlugin)
