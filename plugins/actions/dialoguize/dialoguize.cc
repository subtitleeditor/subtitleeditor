// subtitleeditor -- a tool to create or edit subtitle
//
// https://kitone.github.io/subtitleeditor/
// https://github.com/kitone/subtitleeditor/
//
// Copyright @ 2005-2018, kitone
//
// CHANGES:
//  2012-05-03 - Preference dialog added, eltomito
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

#include <memory.h>
#include "debug.h"
#include "extension/action.h"
#include "gtkmm_utility.h"
#include "i18n.h"
#include "utility.h"
#include "widget_config_utility.h"

// =============== PREFERENCES ===============

#define PREF_GROUP "dialoguize"
#define PREF_KEY_DASH "dash"
#define PREF_KEY_DASH_ESCAPED "dash-escaped"
#define PREF_KEY_CUSTOM "custom-prefix"

class DialogDialoguizePreferences : public Gtk::Dialog {
 public:
  // set dash and its escaped version in the preferences
  static void set_dash(Glib::ustring dash) {
    cfg::set_string(PREF_GROUP, PREF_KEY_DASH, dash);
    cfg::set_string(PREF_GROUP, PREF_KEY_DASH_ESCAPED,
                    Glib::Regex::escape_string(dash));
  }

 protected:
  sigc::connection m_button_dash_space_toggled;
  sigc::connection m_button_dash_toggled;
  sigc::connection m_button_custom_toggled;

  Gtk::RadioButton* radiobutton_dash_space;
  Gtk::RadioButton* radiobutton_dash;
  Gtk::RadioButton* radiobutton_custom;

  void on_button_dash_toggled() {
    if (radiobutton_dash->get_active()) {
      set_dash("-");
    }
  }

  void on_button_dash_space_toggled() {
    if (radiobutton_dash_space->get_active()) {
      set_dash("- ");
    }
  }

  void on_button_custom_toggled() {
    if (radiobutton_custom->get_active()) {
      auto customdash = cfg::get_string(PREF_GROUP, PREF_KEY_CUSTOM);
      set_dash(customdash);
    }
  }

  void on_entry_change() {
    auto customdash = cfg::get_string(PREF_GROUP, PREF_KEY_CUSTOM);
    set_dash(customdash);
    radiobutton_custom->set_active(true);
  }

 public:
  DialogDialoguizePreferences(BaseObjectType* cobject,
                              const Glib::RefPtr<Gtk::Builder>& xml)
      : Gtk::Dialog(cobject) {
    // make sure my preferences exist
    if (!cfg::has_key(PREF_GROUP, PREF_KEY_DASH)) {
      set_dash("- ");
    }
    if (!cfg::has_key(PREF_GROUP, PREF_KEY_CUSTOM)) {
      cfg::set_string(PREF_GROUP, PREF_KEY_CUSTOM, "");
    }
    Gtk::Entry* entry = NULL;
    xml->get_widget("entry-custom-dialog-prefix", entry);
    widget_config::read_config_and_connect(entry, "dialoguize",
                                           "custom-prefix");
    // this updates the dash prefix stored in the preferences whenever the entry
    // box changes
    entry->signal_changed().connect(
        sigc::mem_fun(*this, &DialogDialoguizePreferences::on_entry_change));

    radiobutton_dash_space = NULL;
    xml->get_widget("radiobutton-dash-space", radiobutton_dash_space);
    m_button_dash_space_toggled =
        radiobutton_dash_space->signal_toggled().connect(sigc::mem_fun(
            *this, &DialogDialoguizePreferences::on_button_dash_space_toggled));

    radiobutton_dash = NULL;
    xml->get_widget("radiobutton-dash-only", radiobutton_dash);
    m_button_dash_toggled =
        radiobutton_dash->signal_toggled().connect(sigc::mem_fun(
            *this, &DialogDialoguizePreferences::on_button_dash_toggled));

    radiobutton_custom = NULL;
    xml->get_widget("radiobutton-custom", radiobutton_custom);
    m_button_custom_toggled =
        radiobutton_custom->signal_toggled().connect(sigc::mem_fun(
            *this, &DialogDialoguizePreferences::on_button_custom_toggled));

    // now I need to guess from the dash setting which one of the radio buttons
    // is actually active
    Glib::ustring dash = cfg::get_string(PREF_GROUP, PREF_KEY_DASH);
    Glib::ustring custom = cfg::get_string(PREF_GROUP, PREF_KEY_CUSTOM);

    if (dash.empty())
      dash = "- ";

    if (dash == "-") {
      radiobutton_dash->set_active(true);
    } else {
      if (dash == custom) {
        radiobutton_custom->set_active(true);
      } else {
        radiobutton_dash_space->set_active(true);
      }
    }
  }

  static void create() {
    std::unique_ptr<DialogDialoguizePreferences> dialog(
        gtkmm_utility::get_widget_derived<DialogDialoguizePreferences>(
            SE_DEV_VALUE(SE_PLUGIN_PATH_UI, SE_PLUGIN_PATH_DEV),
            "dialog-dialoguize-preferences.ui",
            "dialog-dialoguize-preferences"));

    dialog->run();
  }
};

// ==================== ACTION =====================

class DialoguizeSelectedSubtitlesPlugin : public Action {
 public:
  DialoguizeSelectedSubtitlesPlugin() {
    activate();
    update_ui();
  }

  ~DialoguizeSelectedSubtitlesPlugin() {
    deactivate();
  }

  void activate() {
    se_dbg(SE_DBG_PLUGINS);

    // actions
    action_group =
        Gtk::ActionGroup::create("DialoguizeSelectedSubtitlesPlugin");

    action_group->add(
        Gtk::Action::create("dialoguize-selected-subtitles", _("_Dialogue"),
                            _("Add or remove dialogue line")),
        Gtk::AccelKey("D"),
        sigc::mem_fun(*this, &DialoguizeSelectedSubtitlesPlugin::on_execute));

    // ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui->insert_action_group(action_group);

    Glib::ustring submenu = R"(
      <ui>
        <menubar name='menubar'>
          <menu name='menu-edit' action='menu-edit'>
            <placeholder name='text-formatting'>
              <menuitem action='dialoguize-selected-subtitles'/>
            </placeholder>
          </menu>
        </menubar>
      </ui>
    )";

    ui_id = ui->add_ui_from_string(submenu);
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

    action_group->get_action("dialoguize-selected-subtitles")
        ->set_sensitive(visible);
  }

  bool is_configurable() {
    return true;
  }

  void create_configure_dialog() {
    DialogDialoguizePreferences::create();
  }

 protected:
  void on_execute() {
    se_dbg(SE_DBG_PLUGINS);

    execute();
  }

  enum class TextType { TEXT, TRANSLATION };

  bool execute() {
    se_dbg(SE_DBG_PLUGINS);
    Document* doc = get_current_document();
    g_return_val_if_fail(doc, false);
    Subtitles subtitles = doc->subtitles();
    std::vector<Subtitle> selection = subtitles.get_selection();
    if (selection.empty()) {
      doc->flash_message(_("Please select at least a subtitle."));
      return false;
    }

    // just in case the preferences haven't been set yet
    if (!cfg::has_key(PREF_GROUP, PREF_KEY_DASH)) {
      DialogDialoguizePreferences::set_dash("- ");
    }

    doc->start_command(_("Dialoguize"));

    Glib::ustring dash = cfg::get_string(PREF_GROUP, PREF_KEY_DASH);
    Glib::ustring dash_escaped = cfg::get_string(PREF_GROUP, PREF_KEY_DASH_ESCAPED);
    Glib::ustring dash_regex = "^" + dash_escaped + "\\s*";

    std::vector<TextType> text_types = {TextType::TEXT, TextType::TRANSLATION};

    for (TextType type : text_types) {
      bool should_add_dashes = !partial_match(selection, dash_regex, type);
      global_replace(selection, dash_regex, "", type);  // Remove existing
      if (should_add_dashes) {
        global_replace(selection, "^", dash, type);     // Add if needed
      }
    }

    doc->finish_command();
    return true;
  }

  bool partial_match(std::vector<Subtitle>& subs, const std::string& pattern, TextType type) {
    se_dbg(SE_DBG_PLUGINS);
    Glib::RefPtr<Glib::Regex> re = Glib::Regex::create(pattern);

    for (const auto& subtitle : subs) {
      Glib::ustring text = (type == TextType::TEXT) ? subtitle.get_text() : subtitle.get_translation();
      if (re->match(text))
        return true;
    }
    return false;
  }

  void global_replace(std::vector<Subtitle>& subs, const std::string& pattern,
                    const std::string& replace, TextType type) {
    se_dbg(SE_DBG_PLUGINS);

    Glib::RefPtr<Glib::Regex> re = Glib::Regex::create(pattern, Glib::REGEX_MULTILINE);
    for (auto& subtitle : subs) {
        Glib::ustring text = (type == TextType::TEXT) ? subtitle.get_text() : subtitle.get_translation();
        text = re->replace_literal(text, 0, replace, (Glib::RegexMatchFlags)0);
        if (type == TextType::TEXT) {
            subtitle.set_text(text);
        } else {
            subtitle.set_translation(text);
        }
    }
  }

 protected:
  Gtk::UIManager::ui_merge_id ui_id;
  Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(DialoguizeSelectedSubtitlesPlugin)
