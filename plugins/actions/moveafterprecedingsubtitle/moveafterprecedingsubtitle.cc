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

class MoveAfterPrecedingSubtitlePlugin : public Action {
 public:
  MoveAfterPrecedingSubtitlePlugin() {
    activate();
    update_ui();
  }

  ~MoveAfterPrecedingSubtitlePlugin() {
    deactivate();
  }

  void activate() {
    se_dbg(SE_DBG_PLUGINS);

    // actions
    action_group = Gtk::ActionGroup::create("MoveAfterPrecedingSubtitlePlugin");

    action_group->add(
        Gtk::Action::create("move-after-preceding-subtitle",
                            _("_Move After Preceding"),
                            _("Move subtitle after the preceding with the "
                              "respect of the minimum gap between subtitles")),
        sigc::mem_fun(*this,
                      &MoveAfterPrecedingSubtitlePlugin::on_execute_after));
    action_group->add(
        Gtk::Action::create("move-before-next-subtitle", _("_Move Before Next"),
                            _("Move subtitle just before the next one")),
        sigc::mem_fun(*this,
                      &MoveAfterPrecedingSubtitlePlugin::on_execute_before));

    // ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui_id = ui->new_merge_id();

    ui->insert_action_group(action_group);

    ui->add_ui(ui_id, "/menubar/menu-timings/move-after-preceding-subtitle",
               "move-after-preceding-subtitle",
               "move-after-preceding-subtitle");
    ui->add_ui(ui_id, "/menubar/menu-timings/move-before-next-subtitle",
               "move-before-next-subtitle", "move-before-next-subtitle");
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

    action_group->get_action("move-after-preceding-subtitle")
        ->set_sensitive(visible);
    action_group->get_action("move-before-next-subtitle")
        ->set_sensitive(visible);
  }

 protected:
  void on_execute_after() {
    se_dbg(SE_DBG_PLUGINS);

    execute(true);
  }

  void on_execute_before() {
    se_dbg(SE_DBG_PLUGINS);

    execute(false);
  }

  bool execute(bool after_preceding) {
    se_dbg(SE_DBG_PLUGINS);

    Document *doc = get_current_document();

    g_return_val_if_fail(doc, false);

    Subtitles subtitles = doc->subtitles();

    std::vector<Subtitle> selection = subtitles.get_selection();

    if (selection.empty()) {
      doc->flash_message(_("Please select at least 1 subtitle."));
      return false;
    }

    SubtitleTime gap = cfg::get_int("timing", "min-gap-between-subtitles");
    SubtitleTime min_display = cfg::get_int("timing", "min-display");

    if (after_preceding) {  // move after preceding
      doc->start_command(_("Move After Preceding"));

      for (auto &sub : selection) {
        Subtitle previous = subtitles.get_previous(sub);

        if (previous) {
          SubtitleTime previous_end = previous.get_end();
          SubtitleTime duration = sub.get_duration();
          if (duration.totalmsecs == 0)
            duration = min_display;

          sub.set_start(previous_end + gap);
          sub.set_duration(duration);
        }
      }
    } else {  // move before next
      doc->start_command(_("Before Next Preceding"));
      for (int i = selection.size() - 1; i >= 0; --i) {
        Subtitle sub = selection[i];

        Subtitle next = subtitles.get_next(sub);

        if (next) {
          SubtitleTime next_start = next.get_start();
          SubtitleTime duration = sub.get_duration();
          if (duration.totalmsecs == 0)
            duration = min_display;

          sub.set_start_and_end(next_start - (gap + duration),
                                next_start - gap);
        }
      }
    }
    doc->emit_signal("subtitle-time-changed");
    doc->finish_command();

    return true;
  }

 protected:
  Gtk::UIManager::ui_merge_id ui_id;
  Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(MoveAfterPrecedingSubtitlePlugin)
