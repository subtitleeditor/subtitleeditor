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

class InsertSubtitlePlugin : public Action {
  public:
   InsertSubtitlePlugin() {
      activate();
      update_ui();
   }

   ~InsertSubtitlePlugin() {
      deactivate();
   }

   void activate() {
      se_dbg(SE_DBG_PLUGINS);

      // actions
      action_group = Gtk::ActionGroup::create("InsertSubtitlePlugin");

      action_group->add(
         Gtk::Action::create(
            "insert-subtitle-before", Gtk::Stock::GO_UP, _("Insert _Before"), _("Insert a blank subtitle before the first selected subtitle")),
         Gtk::AccelKey("<Control>Insert"),
         sigc::mem_fun(*this, &InsertSubtitlePlugin::on_insert_subtitle_before));

      action_group->add(
         Gtk::Action::create(
            "insert-subtitle-after", Gtk::Stock::GO_DOWN, _("Insert _After"), _("Insert a blank subtitle after the first selected subtitle")),
         Gtk::AccelKey("Insert"),
         sigc::mem_fun(*this, &InsertSubtitlePlugin::on_insert_subtitle_after));

      // ui
      Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

      ui->insert_action_group(action_group);

      Glib::ustring submenu = R"(
      <ui>
        <menubar name='menubar'>
          <menu name='menu-edit' action='menu-edit'>
            <placeholder name='insert-subtitle'>
              <menuitem action='insert-subtitle-before'/>
              <menuitem action='insert-subtitle-after'/>
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

      action_group->get_action("insert-subtitle-before")->set_sensitive(visible);
      action_group->get_action("insert-subtitle-after")->set_sensitive(visible);
   }

  protected:
   void on_insert_subtitle_before() {
      se_dbg(SE_DBG_PLUGINS);

      execute(BEFORE);
   }

   void on_insert_subtitle_after() {
      se_dbg(SE_DBG_PLUGINS);

      execute(AFTER);
   }

   enum POSITION { BEFORE, AFTER };

   bool execute(POSITION pos) {
      se_dbg(SE_DBG_PLUGINS);

      Document* doc = get_current_document();

      g_return_val_if_fail(doc, false);

      doc->start_command(_("Insert Subtitle"));

      Subtitles subtitles = doc->subtitles();

      std::vector<Subtitle> selection = subtitles.get_selection();

      Subtitle sub, newsub;

      // s'il existe des sous-titres
      // et s'il y a une selection on utilise le premier sous-titre
      // sinon on utilise le premier ou le dernier sous-titre
      if (!selection.empty())
         sub = selection[0];
      else if (subtitles.size() != 0)
         sub = (pos == BEFORE) ? subtitles.get_first() : subtitles.get_last();

      if (sub)
         newsub = (pos == BEFORE) ? subtitles.insert_before(sub) : subtitles.insert_after(sub);
      else
         newsub = subtitles.append();

      if (newsub) {
         subtitles.select(newsub);
         // set default time
         set_time_between_subtitles(newsub, subtitles.get_previous(newsub), subtitles.get_next(newsub));
      }

      doc->finish_command();

      return true;
   }

   void set_time_between_subtitles(Subtitle& sub, const Subtitle& before, const Subtitle& after) {
      se_dbg(SE_DBG_PLUGINS);

      SubtitleTime gap = cfg::get_int("timing", "min-gap-between-subtitles");
      SubtitleTime min = cfg::get_int("timing", "min-display");

      SubtitleTime start, end;

      if (before) {
         start = before.get_end() + gap;
      }

      sub.set_start(start);

      if (after) {
         end = after.get_start() - gap;
         // si le sous-titre est trop petit on ne respect pas le gap
         if (end < start) {
            end = after.get_start();
         }

         // il est possible d'avoir un sous-titre negative
         // dans ce cas on passe par dessus le suivant
         if (end < start)
            end = start + min;

         sub.set_end(end);
      } else {
         sub.set_duration(min);
      }
   }

  protected:
   Gtk::UIManager::ui_merge_id ui_id;
   Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(InsertSubtitlePlugin)
