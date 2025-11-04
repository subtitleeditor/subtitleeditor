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

class ApplyTranslationPlugin : public Action {
 public:
  ApplyTranslationPlugin() {
    activate();
    update_ui();
  }

  ~ApplyTranslationPlugin() {
    deactivate();
  }

  void activate() {
    se_dbg(SE_DBG_PLUGINS);

    // actions
    action_group = Gtk::ActionGroup::create("ApplyTranslationPlugin");

    action_group->add(
        Gtk::Action::create(
            "apply-translation", Gtk::Stock::APPLY, _("Apply _Translation"),
            _("Replace the text of all subtitles by the translation")),
        sigc::mem_fun(*this, &ApplyTranslationPlugin::on_execute));

    // ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui_id = ui->new_merge_id();

    ui->insert_action_group(action_group);

    ui->add_ui(ui_id, "/menubar/menu-tools/apply-translation",
               "apply-translation", "apply-translation");
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

    action_group->get_action("apply-translation")->set_sensitive(visible);
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

    Glib::ustring translation;

    doc->start_command(_("Apply translation"));

    for (Subtitle sub = subtitles.get_first(); sub; ++sub) {
      translation = sub.get_translation();

      if (!translation.empty()) {
        sub.set_text(translation);
      }
    }

    doc->finish_command();
    doc->flash_message(_("The translation was applied."));

    return true;
  }

 protected:
  Gtk::UIManager::ui_merge_id ui_id;
  Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(ApplyTranslationPlugin)
