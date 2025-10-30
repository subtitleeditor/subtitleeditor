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

class SortSubtitlesPlugin : public Action {
 public:
  SortSubtitlesPlugin() {
    activate();
    update_ui();
  }

  ~SortSubtitlesPlugin() {
    deactivate();
  }

  void activate() {
    se_dbg(SE_DBG_PLUGINS);

    // actions
    action_group = Gtk::ActionGroup::create("SortSubtitlesPlugin");

    action_group->add(
        Gtk::Action::create("sort-subtitles", Gtk::Stock::SORT_ASCENDING,
                            _("S_ort Subtitles"),
                            _("Sort subtitles based on their start time")),
        sigc::mem_fun(*this, &SortSubtitlesPlugin::sort_subtitles));
    // ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();
    ui_id = ui->new_merge_id();
    ui->insert_action_group(action_group);
    ui->add_ui(ui_id, "/menubar/menu-timings/placeholder", "sort-subtitles",
               "sort-subtitles");
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
    action_group->get_action("sort-subtitles")->set_sensitive(visible);
  }

 protected:
  void sort_subtitles() {
    se_dbg(SE_DBG_PLUGINS);

    Document *doc = get_current_document();
    g_return_if_fail(doc);

    Subtitles subtitles = doc->subtitles();
    if (subtitles.size() < 2)
      return;

    guint number_of_sub_reorder = 0;

    doc->start_command("Sort Subtitle");
    number_of_sub_reorder = doc->subtitles().sort_by_time();
    doc->finish_command();
    doc->emit_signal("subtitle-time-changed");

    if (number_of_sub_reorder > 0)
      doc->flash_message(
          ngettext("1 subtitle has been reordered.",
                   "%d subtitles have been reordered.", number_of_sub_reorder),
          number_of_sub_reorder);
    else
      doc->flash_message(_("No need to sort subtitles."));
  }

 protected:
  Gtk::UIManager::ui_merge_id ui_id;
  Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(SortSubtitlesPlugin)
