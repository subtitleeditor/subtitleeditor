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

class ExtendLengthPlugin : public Action {
 public:
  ExtendLengthPlugin() {
    activate();
    update_ui();
  }

  ~ExtendLengthPlugin() {
    deactivate();
  }

  void activate() {
    se_dbg(SE_DBG_PLUGINS);

    // actions
    action_group = Gtk::ActionGroup::create("ExtendLengthPlugin");

    action_group->add(
        Gtk::Action::create(
            "extend-length", _("_Extend Length Forward"),
            _("Extend the length of the selected subtitles to the "
              "start time of the next subtitle (while respecting minimum gap "
              "between subtitles)")),
        sigc::mem_fun(*this, &ExtendLengthPlugin::on_extend_length_fwd));

    action_group->add(
        Gtk::Action::create(
            "extend-length-bwd", _("E_xtend Length Backwards"),
            _("Extend the length of selected selected subtitles "
              "backwards to the end time of the previous subtitle (while "
              "respecting minimum gap between subtitles)")),
        sigc::mem_fun(*this, &ExtendLengthPlugin::on_extend_length_bwd));
    // ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui_id = ui->new_merge_id();

    ui->insert_action_group(action_group);

    ui->add_ui(ui_id, "/menubar/menu-timings/extend-length", "extend-length",
               "extend-length");
    ui->add_ui(ui_id, "/menubar/menu-timings/extend-length-bwd",
               "extend-length-bwd", "extend-length-bwd");
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

    action_group->get_action("extend-length")->set_sensitive(visible);
    action_group->get_action("extend-length-bwd")->set_sensitive(visible);
  }

 protected:
  void on_extend_length_fwd() {
    se_dbg(SE_DBG_PLUGINS);

    execute(true);
  }

  void on_extend_length_bwd() {
    se_dbg(SE_DBG_PLUGINS);

    execute(false);
  }

  bool execute(bool forward) {
    se_dbg(SE_DBG_PLUGINS);

    Document *doc = get_current_document();

    g_return_val_if_fail(doc, false);

    Subtitles subtitles = doc->subtitles();

    std::vector<Subtitle> selection = subtitles.get_selection();

    if (selection.empty()) {
      doc->flash_message(_("Please select at least 1 subtitle."));
      return false;
    }

    SubtitleTime gap(cfg::get_int("timing", "min-gap-between-subtitles"));

    doc->start_command(_("Extend lenght"));

    if (forward) {  // extend length forward, i.e. keep start and move end
      for (auto &sub : selection) {
        Subtitle next = subtitles.get_next(sub);
        if (next) {
          SubtitleTime time = next.get_start() - gap;
          sub.set_end(time);
        }
      }
    } else {  // extend length backwards, i.e. keep end and move start
      for (int i = selection.size() - 1; i >= 0; --i) {
        Subtitle &sub = selection[i];
        Subtitle prev = subtitles.get_previous(sub);
        if (prev) {
          SubtitleTime endtime = sub.get_end();
          SubtitleTime starttime = prev.get_end() + gap;
          // NOTE: we cannot just call sub.set_start() because it automatically
          // shifts the end to preserve duration
          sub.set_start_and_end(starttime, endtime);
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

REGISTER_EXTENSION(ExtendLengthPlugin)
