// subtitleeditor -- a tool to create or edit subtitle
//
// https://subtitleeditor.github.io/subtitleeditor/
// https://github.com/subtitleeditor/subtitleeditor/
//
// Copyright @ 2005-2018, kitone
//
// - "Stack selected subtitles as close together as possible"
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

class StackSubtitlesPlugin : public Action {
  public:
   StackSubtitlesPlugin() {
      activate();
      update_ui();
   }

   ~StackSubtitlesPlugin() {
      deactivate();
   }

   void activate() {
      se_dbg(SE_DBG_PLUGINS);

      // actions
      action_group = Gtk::ActionGroup::create("StackSubtitlesPlugin");

      action_group->add(Gtk::Action::create("stack-subtitles",
                                            _("Stack Subtitles From Start"),
                                            _("Stack selected subtitles after the first one as "
                                              "close together as possible.")),
                        sigc::mem_fun(*this, &StackSubtitlesPlugin::on_stack_subtitles_from_start));

      action_group->add(Gtk::Action::create("stack-subtitles-from-end",
                                            _("Stack Subtitles From End"),
                                            _("Stack selected subtitles before the last one as "
                                              "close together as possible.")),
                        sigc::mem_fun(*this, &StackSubtitlesPlugin::on_stack_subtitles_from_end));

      // ui
      Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

      ui_id = ui->new_merge_id();

      ui->insert_action_group(action_group);

      ui->add_ui(ui_id, "/menubar/menu-timings/stack-subtitles", "stack-subtitles", "stack-subtitles");
      ui->add_ui(ui_id, "/menubar/menu-timings/stack-subtitles-from-end", "stack-subtitles-from-end", "stack-subtitles-from-end");
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

      action_group->get_action("stack-subtitles")->set_sensitive(visible);
      action_group->get_action("stack-subtitles-from-end")->set_sensitive(visible);
   }

  protected:
   void on_stack_subtitles_from_start() {
      se_dbg(SE_DBG_PLUGINS);

      execute(true);
   }

   void on_stack_subtitles_from_end() {
      se_dbg(SE_DBG_PLUGINS);

      execute(false);
   }

   bool execute(bool from_start) {
      se_dbg(SE_DBG_PLUGINS);

      Document* doc = get_current_document();
      g_return_val_if_fail(doc, false);

      Subtitles subtitles = doc->subtitles();

      // We can work on multiple contiguous subtitles
      std::list<std::vector<Subtitle> > contiguous_selection;
      if (get_contiguous_selection(contiguous_selection) == false)
         return false;

      doc->start_command(_("Stack Subtitles"));

      for (auto& subtitle : contiguous_selection) {
         stacksubtitles(subtitle, from_start);
      }

      doc->emit_signal("subtitle-time-changed");
      doc->finish_command();
      return true;
   }

   void stacksubtitles(std::vector<Subtitle>& subtitles, bool from_start) {
      int subcnt = subtitles.size();

      if (subcnt < 2)
         return;

      SubtitleTime gap = cfg::get_int("timing", "min-gap-between-subtitles");
      // SubtitleTime mindur = cfg::get_int("timing", "min-display");
      // long maxcps = cfg::get_int("timing", "max-characters-per-second");

      if (from_start) {
         // take each subtitle and snap it after the one before.
         Subtitle* sub = &subtitles[0];
         SubtitleTime endtime = sub->get_end();
         SubtitleTime dur, starttime;

         for (int i = 1; i < subcnt; ++i) {
            sub = &subtitles[i];
            dur = sub->get_duration();
            starttime = endtime + gap;
            endtime = starttime + dur;
            sub->set_start_and_end(starttime, endtime);
         }
      } else {  // from_start == false
         // take each subtitle from last to first and
         // snap it before the one after it
         Subtitle* sub = &subtitles[subcnt - 1];
         SubtitleTime starttime = sub->get_start();
         SubtitleTime dur, endtime;

         for (int i = subcnt - 2; i >= 0; --i) {
            sub = &subtitles[i];
            dur = sub->get_duration();
            endtime = starttime - gap;
            starttime = endtime - dur;
            sub->set_start_and_end(starttime, endtime);
         }
      }
      return;
   }

   bool get_contiguous_selection(std::list<std::vector<Subtitle> >& contiguous_selection) {
      Document* doc = get_current_document();

      std::vector<Subtitle> selection = doc->subtitles().get_selection();
      if (selection.size() < 2) {
         doc->flash_message(_("Stack Subtitles needs at least 2 subtitles to work on."));
         return false;
      }

      contiguous_selection.push_back(std::vector<Subtitle>());

      guint last_id = 0;

      for (const auto& sub : selection) {
         // Is the next subtitle?
         if (sub.get_num() == last_id + 1) {
            contiguous_selection.back().push_back(sub);
            ++last_id;
         } else {
            // Create new list only if the previous is empty.
            if (!contiguous_selection.back().empty()) {
               contiguous_selection.push_back(std::vector<Subtitle>());
            }
            contiguous_selection.back().push_back(sub);

            last_id = sub.get_num();
         }
      }

      // We check if we have at least one contiguous subtitles.
      for (const auto& subs : contiguous_selection) {
         if (subs.size() >= 2) {
            return true;
         }
      }
      doc->flash_message(_("Stack Subtitles only works on a continuous selection."));
      return false;
   }

  protected:
   Gtk::UIManager::ui_merge_id ui_id;
   Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(StackSubtitlesPlugin)
