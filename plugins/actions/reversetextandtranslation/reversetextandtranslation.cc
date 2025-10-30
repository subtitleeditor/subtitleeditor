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

#include <debug.h>
#include <extension/action.h>
#include <i18n.h>

class ReverseTextAndTranslationPlugin : public Action {
 public:
  ReverseTextAndTranslationPlugin() {
    activate();
    update_ui();
  }

  ~ReverseTextAndTranslationPlugin() {
    deactivate();
  }

  void activate() {
    se_dbg(SE_DBG_PLUGINS);

    // actions
    action_group = Gtk::ActionGroup::create("ReverseTextAndTranslationPlugin");

    action_group->add(
        Gtk::Action::create("reverse-text-and-translation",
                            _("_Reverse Text And Translation"),
                            _("Reverse the text and the translation")),
        sigc::mem_fun(*this, &ReverseTextAndTranslationPlugin::on_execute));

    // ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui_id = ui->new_merge_id();

    ui->insert_action_group(action_group);

    ui->add_ui(ui_id, "/menubar/menu-tools/reverse-text-and-translation",
               "reverse-text-and-translation", "reverse-text-and-translation");
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

    action_group->get_action("reverse-text-and-translation")
        ->set_sensitive(visible);
  }

 protected:
  void on_execute() {
    se_dbg(SE_DBG_PLUGINS);

    execute();
  }

  bool execute() {
    se_dbg(SE_DBG_PLUGINS);

    Document *doc = get_current_document();

    g_return_val_if_fail(doc, false);

    Subtitles subtitles = doc->subtitles();

    Glib::ustring text;
    Glib::ustring translation;

    doc->start_command(_("Reverse Text And Translation"));

    for (Subtitle sub = subtitles.get_first(); sub; ++sub) {
      text = sub.get_text();
      translation = sub.get_translation();

      if (text.empty() && translation.empty())
        continue;

      sub.set_text(translation);
      sub.set_translation(text);
    }

    doc->finish_command();
    doc->flash_message(_("Reverse the text and the translation was applied."));

    return true;
  }

 protected:
  Gtk::UIManager::ui_merge_id ui_id;
  Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(ReverseTextAndTranslationPlugin)
