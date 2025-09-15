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

#include <documents.h>
#include <extension/action.h>
#include <glib.h>
#include <gtkmm_utility.h>
#include <gui/comboboxtextcolumns.h>
#include <gui/dialogutility.h>
#include <utility.h>
#include <widget_config_utility.h>
#include <memory>

// FIXME: gtkmm3
// FIXME:
// Subtitle selected changed caused by no modal window (find)

class MatchInfo {
 public:
  MatchInfo() {
    column = 0;
    found = false;
    start = len = Glib::ustring::npos;
  }

  void reset() {
    text = Glib::ustring();
    replacement = Glib::ustring();
    column = 0;
    found = false;
    start = len = Glib::ustring::npos;
  }

 public:
  int column;
  Glib::ustring text;
  Glib::ustring replacement;
  bool found;
  Glib::ustring::size_type start;
  Glib::ustring::size_type len;
};

enum PatternOptions { USE_REGEX = 1 << 1, IGNORE_CASE = 1 << 2 };

enum ColumnOptions { TEXT = 1 << 1, TRANSLATION = 1 << 2 };

// FaR Find and Replace
class FaR {
 public:
  // Return an instance of the engine.
  static FaR &instance() {
    static FaR engine;
    return engine;
  }

  // Returns the search option flag
  // IGNORE_CASE & USE_REGEX
  int get_pattern_options() {
    int flags = 0;
    if (cfg::get_boolean("find-and-replace", "used-regular-expression")) {
      flags |= USE_REGEX;
    }
    if (cfg::get_boolean("find-and-replace", "ignore-case")) {
      flags |= IGNORE_CASE;
    }
    return flags;
  }

  // Search in which columns?
  // TEXT & TRANSLATION
  int get_columns_options() {
    int flags = 0;
    if (cfg::get_boolean("find-and-replace", "column-text")) {
      flags |= TEXT;
    }
    if (cfg::get_boolean("find-and-replace", "column-translation")) {
      flags |= TRANSLATION;
    }
    return flags;
  }

  // Return the current pattern text.
  Glib::ustring get_pattern() {
    return cfg::get_string("find-and-replace", "pattern");
  }

  // Return the current replacement text.
  Glib::ustring get_replacement() {
    return cfg::get_string("find-and-replace", "replacement");
  }

  // Try to find the pattern in the subtitle.
  // A MatchInfo is used to get information on the match,
  // is stored in matchinfo if not NULL.
  bool find_in_subtitle(const Subtitle &sub, MatchInfo *matchinfo) {
    if (!sub)
      return false;

    int columns_options = get_columns_options();
    int current_column = (matchinfo) ? matchinfo->column : 0;

    if (columns_options & TEXT && current_column <= TEXT) {
      if (find_in_text(sub.get_text(), matchinfo)) {
        if (matchinfo)
          matchinfo->column = TEXT;
        return true;
      }
    }
    if (columns_options & TRANSLATION && current_column <= TRANSLATION) {
      if (find_in_text(sub.get_translation(), matchinfo)) {
        if (matchinfo)
          matchinfo->column = TRANSLATION;
        return true;
      }
    }
    // Nothing found reset values to default
    if (matchinfo)
      matchinfo->reset();

    return false;
  }

  // Replace the current search (MatchInfo) by the replacement text.
  bool replace(Document &doc, Subtitle &sub, MatchInfo &info) {
    if (!sub)
      return false;

    if ((info.start == 0 && info.len == 0) ||
        (info.start == Glib::ustring::npos && info.len == Glib::ustring::npos))
      return false;

    if (info.text.empty())
      return false;

    Glib::ustring text = info.text;
    Glib::ustring replacement = get_replacement();
	
	
    try {
      text.replace(info.start, info.len, replacement);
    } catch (const std::exception &ex) {
      std::cerr << "FindAndReplacePlugin::Exception : " << ex.what()
                << std::endl;
      return false;
    }
    // update length of info
    info.len = replacement.size();

    doc.start_command(_("Replace text"));

    if (info.column == TEXT)
      sub.set_text(text);
    else if (info.column == TRANSLATION)
      sub.set_translation(text);

    doc.subtitles().select(sub);
    doc.finish_command();
    return true;
  }

 protected:
  bool find_in_text(const Glib::ustring &otext, MatchInfo *info) {
    Glib::ustring text = otext;
    Glib::ustring::size_type beginning = Glib::ustring::npos;

    try {
      if (info) {
        if (info->start != Glib::ustring::npos &&
            info->len != Glib::ustring::npos)
          beginning = info->start + info->len;
        // We reset some values
        info->start = info->len = Glib::ustring::npos;
        info->found = false;
        info->text = Glib::ustring();
      }

      if (beginning != Glib::ustring::npos)
        text = text.substr(beginning, text.size());

      if (!find(get_pattern(), get_pattern_options(), text, info))
        return false;

      if (info) {  // Found, update matchinfo values
        info->text = otext;
        if (beginning != Glib::ustring::npos)
          info->start += beginning;  // if we used a substring (start != 0)n we
                                     // need to update the beginning
      }
      return true;
    } catch (std::exception &ex) {
      std::cerr << "# Exception: " << ex.what() << std::endl;
    }
    return false;
  }

  bool find(const Glib::ustring &pattern, int pattern_options,
            const Glib::ustring &text, MatchInfo *info) {
    if (pattern.empty())
      return false;

    bool found = false;
    Glib::ustring::size_type start, len;

    if (pattern_options & USE_REGEX) {  // Search with regular expression
      found = regex_exec(pattern, text, (pattern_options & IGNORE_CASE), start,
                         len, info->replacement);
    } else {  // Without regular expression
      Glib::ustring pat =
          (pattern_options & IGNORE_CASE) ? pattern.lowercase() : pattern;
      Glib::ustring txt =
          (pattern_options & IGNORE_CASE) ? text.lowercase() : text;

      Glib::ustring::size_type res = txt.find(pat);
      if (res != Glib::ustring::npos) {
        found = true;
        start = res;
        len = pattern.size();
      }
    }

    if (found && info) {
      info->found = true;
      info->start = start;
      info->len = len;
    }
    return found;
  }

  // FIXME: Remove Me
  // Waiting the Glib::MatchInfo API in glibmm.
  bool regex_exec(const Glib::ustring &pattern, const Glib::ustring &string,
                  bool caseless, Glib::ustring::size_type &start,
                  Glib::ustring::size_type &len, Glib::ustring &replacement) {
    bool found = false;
    GRegex *regex = NULL;
    GMatchInfo *match_info = NULL;
    GError *error = NULL;
    gboolean references = FALSE;

    int compile_flags = (GRegexMatchFlags)0;
    if (caseless)
      compile_flags |= G_REGEX_CASELESS;
    regex = g_regex_new(pattern.c_str(), (GRegexCompileFlags)compile_flags,
                        (GRegexMatchFlags)0, &error);
    if (error != NULL) {
      std::cerr << "regex_exec error: " << error->message << std::endl;
      g_error_free(error);
      return false;
    }

    if (g_regex_match(regex, string.c_str(), (GRegexMatchFlags)0,
                      &match_info)) {
      if (g_match_info_matches(match_info)) {
        int start_pos, end_pos;
        // check the return
        if (g_match_info_fetch_pos(match_info,
                                   0,  // match_num 0 is full text of the match
                                   &start_pos, &end_pos)) {
          // We need to convert the position from the byte position to a
          // character position.
          start_pos = g_utf8_pointer_to_offset(string.c_str(),
                                               string.c_str() + start_pos);
          end_pos = g_utf8_pointer_to_offset(string.c_str(),
                                             string.c_str() + end_pos);

          start = start_pos;
          len = end_pos - start_pos;
          found = true;
        }

        // Expand any references in the replacement string
        references = TRUE;
        g_regex_check_replacement(replacement.c_str(), &references, &error);
        if (error == NULL && references)
          replacement = g_match_info_expand_references(
              match_info, replacement.c_str(), &error);
      }
    }
    g_match_info_free(match_info);
    g_regex_unref(regex);
    return found;
  }
};

class ComboBoxEntryHistory : public Gtk::ComboBoxText {
 public:
  ComboBoxEntryHistory(BaseObjectType *cobject,
                       const Glib::RefPtr<Gtk::Builder> &)
      : Gtk::ComboBoxText(cobject) {
  }

  // Initialize the widget with the group and the key for the config.
  // Sets the widget history.
  void initialize(const Glib::ustring &group, const Glib::ustring &key) {
    m_group = group;
    m_key = key;

    load_history();
  }

  // Add the current entry text to the history model.
  void push_to_history() {
    Glib::ustring text = get_entry()->get_text();
    if (!text.empty()) {
      remove_item(text);
      prepend(text);
      clamp_items();
    }
  }

  // Read the history of the widget.
  void load_history() {
    auto keys = cfg::get_keys(m_group);

    Glib::RefPtr<Glib::Regex> re = Glib::Regex::create(m_key + "-(\\d+)");

    for (const auto &k : keys) {
      if (re->match(k)) {
        append(cfg::get_string(m_group, k));
      }
    }
    get_entry()->set_text(cfg::get_string(m_group, m_key));
  }

  // Write the history of the widget to the config.
  void save_history() {
    cfg::set_string(m_group, m_key, get_entry()->get_text());

    get_model()->foreach (
        sigc::mem_fun(*this, &ComboBoxEntryHistory::save_iter));
  }

  bool save_iter(const Gtk::TreePath &path, const Gtk::TreeIter &it) {
    auto key = Glib::ustring::compose("%1-%2", m_key, path.to_string());
    auto value = (*it)[m_cols.m_col_name];
    cfg::set_string(m_group, key, value);
    return false;
  }

  // Remove items equal to text.
  void remove_item(const Glib::ustring &text) {
    Glib::RefPtr<Gtk::ListStore> model =
        Glib::RefPtr<Gtk::ListStore>::cast_dynamic(get_model());

    Gtk::TreeIter it = model->children().begin();
    while (it) {
      if ((*it)[m_cols.m_col_name] == text)
        it = model->erase(it);
      else
        ++it;
    }
  }

  // Clamp items to maximum
  void clamp_items() {
    Glib::RefPtr<Gtk::ListStore> model =
        Glib::RefPtr<Gtk::ListStore>::cast_dynamic(get_model());
    while (model->children().size() > 10) {
      Gtk::TreeIter it = model->get_iter("10");
      if (it)
        model->erase(it);
    }
  }

 protected:
  Glib::ustring m_group;
  Glib::ustring m_key;
  ComboBoxTextColumns m_cols;
};

// Dialog Find And Replace
class DialogFindAndReplace : public DialogActionMultiDoc {
 public:
  // like to.ui file
  enum RESPONSE { FIND = 1, REPLACE = 2, REPLACE_ALL = 3 };

  // Constructor
  DialogFindAndReplace(BaseObjectType *cobject,
                       const Glib::RefPtr<Gtk::Builder> &xml)
      : DialogActionMultiDoc(cobject, xml), m_document(NULL) {
    utility::set_transient_parent(*this);

    xml->get_widget("label-current-column", m_labelCurrentColumn);
    xml->get_widget("textview", m_textview);

    xml->get_widget_derived("comboboxentry-pattern", m_comboboxPattern);
    xml->get_widget_derived("comboboxentry-replacement", m_comboboxReplacement);
    xml->get_widget("check-ignore-case", m_checkIgnoreCase);
    xml->get_widget("check-used-regular-expression",
                    m_checkUsedRegularExpression);
    xml->get_widget("button-replace", m_buttonReplace);
    xml->get_widget("button-replace-all", m_buttonReplaceAll);
    xml->get_widget("button-find", m_buttonFind);

    xml->get_widget("check-column-text", m_checkColumnText);
    xml->get_widget("check-column-translation", m_checkColumnTranslation);

    m_comboboxPattern->initialize("find-and-replace", "pattern");
    m_comboboxReplacement->initialize("find-and-replace", "replacement");

    // Connect entry of the combobox
    widget_config::read_config_and_connect(m_comboboxPattern->get_entry(),
                                           "find-and-replace", "pattern");
    widget_config::read_config_and_connect(m_comboboxReplacement->get_entry(),
                                           "find-and-replace", "replacement");

    widget_config::read_config_and_connect(m_checkIgnoreCase,
                                           "find-and-replace", "ignore-case");
    widget_config::read_config_and_connect(m_checkUsedRegularExpression,
                                           "find-and-replace",
                                           "used-regular-expression");

    widget_config::read_config_and_connect(m_checkColumnText,
                                           "find-and-replace", "column-text");
    widget_config::read_config_and_connect(
        m_checkColumnTranslation, "find-and-replace", "column-translation");

    m_comboboxPattern->grab_focus();
    m_comboboxPattern->get_entry()->signal_activate().connect(sigc::bind(
        sigc::mem_fun(*this, &DialogFindAndReplace::on_response), FIND));

    set_default_response(Gtk::RESPONSE_CLOSE);

    // Create tag found
    Glib::RefPtr<Gtk::TextTag> found =
        m_textview->get_buffer()->create_tag("found");
    found->property_weight() = Pango::WEIGHT_BOLD;
    found->property_foreground() = "blue";
    found->property_underline() = Pango::UNDERLINE_SINGLE;
    found->property_underline_set() = true;

    hide();
  }

  // Create a single instance of the dialog.
  static void create() {
    if (m_instance == nullptr) {
      m_instance = gtkmm_utility::get_widget_derived<DialogFindAndReplace>(
          SE_DEV_VALUE(SE_PLUGIN_PATH_UI, SE_PLUGIN_PATH_DEV),
          "dialog-find-and-replace.ui", "dialog-find-and-replace");
    }
    m_instance->show();
    m_instance->present();
  }

  // Return a unique instance of the dialog.
  static DialogFindAndReplace *instance() {
    return m_instance;
  }

  // Initialize the ui with this document
  void init_with_document(Document *doc) {
    if (m_connection_subtitle_deleted)
      m_connection_subtitle_deleted.disconnect();

    m_document = doc;
    bool has_doc = (doc != NULL);
    // Update the sensitivity of widgets
    m_buttonReplace->set_sensitive(has_doc);
    m_buttonReplaceAll->set_sensitive(has_doc);
    m_buttonFind->set_sensitive(has_doc);

    m_comboboxPattern->set_sensitive(has_doc);
    m_comboboxReplacement->set_sensitive(has_doc);

    m_checkIgnoreCase->set_sensitive(has_doc);
    m_checkUsedRegularExpression->set_sensitive(has_doc);

    // Reset values
    m_subtitle = Subtitle();
    m_info.reset();

    if (doc == NULL)
      return;

    Subtitles subtitles = doc->subtitles();
    if (subtitles.size() == 0) {
      doc->flash_message(_("The document is empty"));
    } else {
      m_subtitle = subtitles.get_first_selected();
      if (!m_subtitle)
        m_subtitle = subtitles.get_first();
      update_search_ui();
    }

    m_connection_subtitle_deleted =
        doc->get_signal("subtitle-deleted")
            .connect(sigc::mem_fun(*this,
                                   &DialogFindAndReplace::on_subtitle_deleted));
  }

  // The current document has changed. We need do update the ui.
  void on_current_document_changed(Document *newdoc) {
    if (newdoc != m_document) {
      m_document = newdoc;

      init_with_document(newdoc);
      update_search_ui();
    }
  }

  void on_subtitle_deleted() {
    // Reset values
    m_subtitle = Subtitle();
    m_info.reset();

    Subtitles subtitles = m_document->subtitles();
    if (subtitles.size() > 0) {
      m_subtitle = subtitles.get_first_selected();
      if (!m_subtitle)
        m_subtitle = subtitles.get_first();
    }
    update_search_ui();
  }
  // Update the label of the current column and sets the sensitivity.
  void update_column_label() {
    m_labelCurrentColumn->set_sensitive(m_info.found);

    if (m_info.column == TEXT)
      m_labelCurrentColumn->set_text(_("Text"));
    else if (m_info.column == TRANSLATION)
      m_labelCurrentColumn->set_text(_("Translation"));
  }

  // Update some widgets from the current info search.
  void update_search_ui() {
    m_textview->set_sensitive(m_info.found);
    m_buttonReplace->set_sensitive(m_info.found);

    update_column_label();

    if (m_info.found && m_info.start != Glib::ustring::npos &&
        m_info.len != Glib::ustring::npos) {
      Glib::RefPtr<Gtk::TextBuffer> buffer = m_textview->get_buffer();

      buffer->set_text(m_info.text);

      Gtk::TextIter ins = buffer->get_iter_at_offset(m_info.start);
      Gtk::TextIter bound =
          buffer->get_iter_at_offset(m_info.start + m_info.len);

      buffer->apply_tag_by_name("found", ins, bound);
      buffer->select_range(ins, bound);
    } else {
      m_textview->get_buffer()->set_text("");
    }
  }

  // Response handler for signals:
  // FIND, REPLACE, REPLACE_ALL and (RESPONSE_CLOSE & RESPONSE_DELETE_EVENT)
  void on_response(int response) {
    if (response == FIND) {
      if (find_forwards(m_subtitle, &m_info)) {
        m_document->subtitles().select(m_subtitle);
        m_comboboxPattern->push_to_history();
      } else {
        // Failed to find from last position to the end of the document.
        // If the option 'all documents' is activated, check with the next
        // document.
        if (apply_to_all_documents()) {
          m_document = get_next_document();
          set_current_document(m_document);
        }
        // We try to search from the beginning of the document (new or not)
        m_document->subtitles().unselect_all();
        m_info.reset();
        m_subtitle = m_document->subtitles().get_first();

        if (find_forwards(m_subtitle, &m_info)) {
          m_document->subtitles().select(m_subtitle);
          m_comboboxPattern->push_to_history();
        }
      }
      update_search_ui();
    } else if (response == REPLACE) {
      if (FaR::instance().replace(*m_document, m_subtitle, m_info))
        m_comboboxReplacement->push_to_history();
      // next
      Gtk::Dialog::response(FIND);
    } else if (response == REPLACE_ALL) {
      replace_all();
    } else if (response == Gtk::RESPONSE_CLOSE ||
               response == Gtk::RESPONSE_DELETE_EVENT) {
      m_comboboxPattern->save_history();
      m_comboboxReplacement->save_history();
      m_connection_subtitle_deleted.disconnect();

      delete m_instance;
      m_instance = nullptr;
    }
  }

  // Find the next pattern from the current subtitle and the current info.
  // Recrusive function.
  bool find_forwards(Subtitle &sub, MatchInfo *info) {
    se_dbg(SE_DBG_SEARCH);

    if (!sub)
      return false;

    // search again in the subtitle
    if (FaR::instance().find_in_subtitle(sub, info))
      return true;

    if (info)
      info->reset();

    ++sub;  // next subtitle

    if (!sub)
      return false;

    return find_forwards(sub, info);
  }

  // Start with the beginning of all documents and try to replace all.
  bool replace_all() {
    DocumentList docs;

    if (apply_to_all_documents())
      docs = get_sort_documents();
    else
      docs.push_back(m_document);

    for (const auto &doc : docs) {
      set_current_document(doc);
      // List of the modified subtitles
      std::list<Subtitle> selection;

      m_subtitle = m_document->subtitles().get_first();
      m_info.reset();

      while (m_subtitle) {
        while (find_forwards(m_subtitle, &m_info)) {
          if (FaR::instance().replace(*m_document, m_subtitle, m_info))
            selection.push_back(m_subtitle);
        }
      }
      // We select the modified subtitles
      m_document->subtitles().select(selection);
    }
    update_search_ui();
    return true;
  }

  // Return a sorted documents list from the current to the last.
  DocumentList get_sort_documents() {
    DocumentList list = get_documents_to_apply();

    DocumentList::iterator it_cur = list.end();
    // First we get the current document iterator
    for (DocumentList::iterator it = list.begin(); it != list.end(); ++it) {
      if (*it == m_document) {
        it_cur = it;
        break;
      }
    }
    // We move the previous document to the last
    if (it_cur != list.end()) {
      DocumentList previous(list.begin(), it_cur);
      it_cur = list.erase(list.begin(), it_cur);
      list.insert(list.end(), previous.begin(), previous.end());
    }
    return list;
  }

  // Return the next document. This function make a loop:
  //               (m_document)
  // +-- previous -> current -> next ----+
  // |                                   |
  // +---<------------------------<------+
  Document *get_next_document() {
    DocumentList list = get_documents_to_apply();

    Document *cur = m_document;
    for (auto it = list.begin(); it != list.end(); ++it) {
      if (*it == cur) {
        ++it;
        if (it != list.end())
          return *it;
        else
          return list.front();
      }
    }
    return m_document;
  }

  // Sets the current document an update ui.
  void set_current_document(Document *doc) {
    m_document = doc;
    se::documents::active(doc);
    // Update ui
    while (Gtk::Main::events_pending()) Gtk::Main::iteration();
  }

 protected:
  Document *m_document;
  Subtitle m_subtitle;
  MatchInfo m_info;

  Gtk::Label *m_labelCurrentColumn;
  Gtk::TextView *m_textview;
  ComboBoxEntryHistory *m_comboboxPattern;
  ComboBoxEntryHistory *m_comboboxReplacement;
  Gtk::CheckButton *m_checkIgnoreCase;
  Gtk::CheckButton *m_checkUsedRegularExpression;
  Gtk::Button *m_buttonReplace;
  Gtk::Button *m_buttonReplaceAll;
  Gtk::Button *m_buttonFind;

  Gtk::CheckButton *m_checkColumnText;
  Gtk::CheckButton *m_checkColumnTranslation;

  sigc::connection m_connection_subtitle_deleted;

  static DialogFindAndReplace *m_instance;
};

// Static instance of the dialog
DialogFindAndReplace *DialogFindAndReplace::m_instance = nullptr;

// Plugin
class FindAndReplacePlugin : public Action {
 public:
  FindAndReplacePlugin() {
    activate();
    update_ui();
  }

  ~FindAndReplacePlugin() {
    deactivate();
  }

  void activate() {
    se_dbg(SE_DBG_PLUGINS);

    // actions
    action_group = Gtk::ActionGroup::create("FindAndReplacePlugin");

    action_group->add(
        Gtk::Action::create("find-and-replace", Gtk::Stock::FIND_AND_REPLACE,
                            _("_Find And Replace"),
                            _("Search and replace text")),
        Gtk::AccelKey("<Control>F"),
        sigc::mem_fun(*this, &FindAndReplacePlugin::on_search_and_replace));

    action_group->add(
        Gtk::Action::create("find-next", _("Find Ne_xt"),
                            _("Search forwards for the same text")),
        Gtk::AccelKey("<Control>G"),
        sigc::mem_fun(*this, &FindAndReplacePlugin::on_find_next));
    action_group->add(
        Gtk::Action::create("find-previous", _("Find Pre_vious"),
                            _("Search backwards for the same text")),
        Gtk::AccelKey("<Shift><Control>G"),
        sigc::mem_fun(*this, &FindAndReplacePlugin::on_find_previous));

    // ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui->insert_action_group(action_group);

    Glib::ustring submenu = R"(
      <ui>
        <menubar name='menubar'>
          <menu name='menu-tools' action='menu-tools'>
            <placeholder name='find-and-replace'>
              <menuitem action='find-and-replace'/>
              <menuitem action='find-next'/>
              <menuitem action='find-previous'/>
            </placeholder>
          </menu>
        </menubar>
      </ui>
    )";

    ui_id = ui->add_ui_from_string(submenu);

    check_default_values();
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

    action_group->get_action("find-and-replace")->set_sensitive(visible);
    action_group->get_action("find-next")->set_sensitive(visible);
    action_group->get_action("find-previous")->set_sensitive(visible);

    DialogFindAndReplace *instance = DialogFindAndReplace::instance();
    if (instance)
      instance->on_current_document_changed(get_current_document());
  }

 protected:
  void check_default_values() {
    if (!cfg::has_key("find-and-replace", "column-text"))
      cfg::set_boolean("find-and-replace", "column-text", true);
    if (!cfg::has_key("find-and-replace", "column-translation"))
      cfg::set_boolean("find-and-replace", "column-translation", true);
    if (!cfg::has_key("find-and-replace", "ignore-case"))
      cfg::set_boolean("find-and-replace", "ignore-case", false);
    if (!cfg::has_key("find-and-replace", "used-regular-expression"))
      cfg::set_boolean("find-and-replace", "used-regular-expression", false);
  }

  void on_search_and_replace() {
    se_dbg(SE_DBG_PLUGINS);

    DialogFindAndReplace::create();
    DialogFindAndReplace::instance()->init_with_document(
        get_current_document());
  }

  void on_find_next() {
    se_dbg(SE_DBG_PLUGINS);
    find_sub(false);
  }

  void on_find_previous() {
    se_dbg(SE_DBG_PLUGINS);
    find_sub(true);
  }

  void find_sub(bool backwards) {
    se_dbg(SE_DBG_PLUGINS);

    Document *doc = get_current_document();

    Subtitles subtitles = doc->subtitles();

    if (subtitles.size() == 0) {
      doc->flash_message(_("The document is empty"));
      return;
    }

    Subtitle sub;
    if (search_from_current_position(sub, backwards) ||
        search_from_beginning(sub, backwards)) {
      subtitles.select(sub);
    } else {
      subtitles.unselect_all();
      doc->flash_message(_("Not found"));
    }
  }

  // Start the search from the previous/next subtitle
  bool search_from_current_position(Subtitle &res, bool backwards) {
    se_dbg(SE_DBG_PLUGINS);

    Subtitles subtitles = get_current_document()->subtitles();

    Subtitle sub = subtitles.get_first_selected();
    // selection empty? return invalid subtitle
    if (!sub)
      return false;

    // Start from the previous/next subtitle
    sub = (backwards) ? subtitles.get_previous(sub) : subtitles.get_next(sub);
    while (sub) {
      if (FaR::instance().find_in_subtitle(sub, NULL)) {
        res = sub;
        return true;
      }
      sub = (backwards) ? subtitles.get_previous(sub) : subtitles.get_next(sub);
    }
    return false;
  }

  bool search_from_beginning(Subtitle &res, bool backwards) {
    se_dbg(SE_DBG_PLUGINS);

    Subtitles subtitles = get_current_document()->subtitles();
    Subtitle sub = (backwards) ? subtitles.get_last() : subtitles.get_first();
    while (sub) {
      if (FaR::instance().find_in_subtitle(sub, NULL)) {
        res = sub;
        return true;
      }
      sub = (backwards) ? subtitles.get_previous(sub) : subtitles.get_next(sub);
    }
    return false;
  }

 protected:
  Gtk::UIManager::ui_merge_id ui_id;
  Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(FindAndReplacePlugin)
