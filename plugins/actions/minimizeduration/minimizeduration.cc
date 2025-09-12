// subtitleeditor -- a tool to create or edit subtitle
//
// https://kitone.github.io/subtitleeditor/
// https://github.com/kitone/subtitleeditor/
//
// Copyright @ 2005-2018, kitone
//
// - "the ultimate subtitle fitness center"
// a subtitleeditor plugin by Eltomito <tomaspartl@centrum.cz>
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
#include <utility.h>

class MinimizeDurationPlugin : public Action {
 public:
  MinimizeDurationPlugin() {
    activate();
    update_ui();
  }

  ~MinimizeDurationPlugin() {
    deactivate();
  }

  void activate() {
    se_dbg(SE_DBG_PLUGINS);

    // actions
    action_group = Gtk::ActionGroup::create("MinimizeDurationPlugin");

    action_group->add(
        Gtk::Action::create(
            "minimize-duration", _("Minimize Duration From Start"),
            _("Compact each selected subtitle to its minimum permissible "
              "duration, start time is unchanged.")),
        sigc::mem_fun(
            *this, &MinimizeDurationPlugin::on_minimize_duration_from_start));
    action_group->add(
        Gtk::Action::create("minimize-duration-from-end",
                            _("Minimize Duration From End"),
                            _("Compact each selected subtitle to its minimum "
                              "permissible duration, end time is unchanged.")),
        sigc::mem_fun(*this,
                      &MinimizeDurationPlugin::on_minimize_duration_from_end));

    action_group->add(
        Gtk::Action::create(
            "idealize-duration", _("_Idealize Duration From Start"),
            _("Compact each selected subtitle to its ideal "
              "duration, start time is unchanged.")),
        sigc::mem_fun(
            *this, &MinimizeDurationPlugin::on_idealize_duration_from_start));
    action_group->add(
        Gtk::Action::create("idealize-duration-from-end",
                            _("_Idealize Duration From End"),
                            _("Compact each selected subtitle to its ideal "
                              "duration, end time is unchanged.")),
        sigc::mem_fun(*this,
                      &MinimizeDurationPlugin::on_idealize_duration_from_end));

    // ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui_id = ui->new_merge_id();

    ui->insert_action_group(action_group);

    ui->add_ui(ui_id, "/menubar/menu-timings/idealize-duration",
               "idealize-duration", "idealize-duration");
    ui->add_ui(ui_id, "/menubar/menu-timings/idealize-duration-from-end",
               "idealize-duration-from-end", "idealize-duration-from-end");
    ui->add_ui(ui_id, "/menubar/menu-timings/minimize-duration",
               "minimize-duration", "minimize-duration");
    ui->add_ui(ui_id, "/menubar/menu-timings/minimize-duration-from-end",
               "minimize-duration-from-end", "minimize-duration-from-end");
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

    action_group->get_action("minimize-duration")->set_sensitive(visible);
    action_group->get_action("minimize-duration-from-end")
        ->set_sensitive(visible);
    action_group->get_action("idealize-duration")->set_sensitive(visible);
    action_group->get_action("idealize-duration-from-end")
        ->set_sensitive(visible);
  }

 protected:
  void on_idealize_duration_from_start() {
    se_dbg(SE_DBG_PLUGINS);

    execute(cfg::get_double("timing", "ideal-characters-per-second"),true);
  }

  void on_idealize_duration_from_end() {
    se_dbg(SE_DBG_PLUGINS);

    execute(cfg::get_double("timing", "ideal-characters-per-second"),false);
  }

  void on_minimize_duration_from_start() {
    se_dbg(SE_DBG_PLUGINS);

    execute(cfg::get_double("timing", "max-characters-per-second"),true);
  }

  void on_minimize_duration_from_end() {
    se_dbg(SE_DBG_PLUGINS);

    execute(cfg::get_double("timing", "max-characters-per-second"),false);
  }

  bool execute(double maxcps, bool from_start) {
    se_dbg(SE_DBG_PLUGINS);

    Document *doc = get_current_document();

    g_return_val_if_fail(doc, false);

    Subtitles subtitles = doc->subtitles();

    // NOTE: the selection returned is always sorted regardless of the order the
    // user clicked on the subtitles in or at least it was when I tried it.
    std::vector<Subtitle> selection = subtitles.get_selection();

    if (selection.size() < 1) {
      doc->flash_message(
          _("Minimize Duration needs at least 1 subtitle to work on."));
      return false;
    }

    // get relevant preferences
    SubtitleTime mindur = cfg::get_int("timing", "min-display");

    doc->start_command(_("Minimize Durations"));

    Glib::ustring subtext = "";

    // take each subtitle and set its duration to the permissible minimum
    unsigned long subchars = 0;
    SubtitleTime dur;

    for (auto &sub : selection) {
      subtext = sub.get_text();
      subchars = utility::get_text_length_for_timing(subtext);
      dur.totalmsecs = utility::get_min_duration_msecs(subchars, maxcps);
      // doc->flash_message ( _("duration calculated is 1000 * %i / %i = %i"),
      // (int)subchars, (int)maxcps, (int)dur.totalmsecs ); make sure we have at
      // least the minimum duration
      if (dur < mindur)
        dur = mindur;

      if (from_start) {
        // the start time is fixed, we are changind the end time
        sub.set_duration(dur);
      } else {
        // the end time is fixed, we are changing the start time
        SubtitleTime endtime = sub.get_end();
        sub.set_start_and_end(endtime - dur, endtime);
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

REGISTER_EXTENSION(MinimizeDurationPlugin)
