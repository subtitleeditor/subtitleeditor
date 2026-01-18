// subtitleeditor -- a tool to create or edit subtitle
//
// https://subtitleeditor.github.io/subtitleeditor/
// https://github.com/subtitleeditor/subtitleeditor/
//
// Copyright @ 2005-2018, kitone
//
// - "justice for selected subtitles"
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

class BestFitPlugin : public Action {
  public:
   BestFitPlugin() {
      activate();
      update_ui();
   }

   ~BestFitPlugin() {
      deactivate();
   }

   /*
    */
   void activate() {
      se_dbg(SE_DBG_PLUGINS);

      // actions
      action_group = Gtk::ActionGroup::create("BestFitPlugin");

      action_group->add(
         Gtk::Action::create("best-fit",
                             _("_Best Fit Subtitles"),
                             _("Equalize the speed (in CPS) of the selected subtitles between the start of the first and the end of the "
                               "last (while respecting the minimum gap between subtitles and their minima duration)")),
         sigc::mem_fun(*this, &BestFitPlugin::on_best_fit));

      // ui
      Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

      ui_id = ui->new_merge_id();

      ui->insert_action_group(action_group);

      ui->add_ui(ui_id, "/menubar/menu-timings/best-fit", "best-fit", "best-fit");
   }

   /*
    */
   void deactivate() {
      se_dbg(SE_DBG_PLUGINS);

      Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

      ui->remove_ui(ui_id);
      ui->remove_action_group(action_group);
   }

   /*
    */
   void update_ui() {
      se_dbg(SE_DBG_PLUGINS);

      bool visible = (get_current_document() != NULL);

      action_group->get_action("best-fit")->set_sensitive(visible);
   }

  protected:
   /*
    */
   void on_best_fit() {
      se_dbg(SE_DBG_PLUGINS);

      Document* doc = get_current_document();
      g_return_if_fail(doc);

      // Check if the selection is contiguous and complain if it isn't.
      // Can work on multiple contiguous subtitles
      std::list<std::vector<Subtitle> > contiguous_selection;
      if (get_contiguous_selection(contiguous_selection) == false)
         return;

      doc->start_command(_("Best fit"));

      for (std::list<std::vector<Subtitle> >::iterator it = contiguous_selection.begin(); it != contiguous_selection.end(); ++it) {
         bestfit(*it);
      }

      doc->emit_signal("subtitle-time-changed");
      doc->finish_command();
   }

   /*
    */
   bool get_contiguous_selection(std::list<std::vector<Subtitle> >& contiguous_selection) {
      Document* doc = get_current_document();

      std::vector<Subtitle> selection = doc->subtitles().get_selection();
      if (selection.size() < 2) {
         doc->flash_message(_("Best Fit needs at least 2 subtitles to work on."));
         return false;
      }

      contiguous_selection.push_back(std::vector<Subtitle>());

      guint last_id = 0;

      for (guint i = 0; i < selection.size(); ++i) {
         Subtitle& sub = selection[i];
         // Is the next subtitle?
         if (sub.get_num() == last_id + 1) {
            contiguous_selection.back().push_back(sub);
            ++last_id;
         } else {
            // Create new list only if the previous is empty.
            if (!contiguous_selection.back().empty())
               contiguous_selection.push_back(std::vector<Subtitle>());

            contiguous_selection.back().push_back(sub);

            last_id = sub.get_num();
         }
      }

      // We check if we have at least one contiguous subtitles.
      for (std::list<std::vector<Subtitle> >::iterator it = contiguous_selection.begin(); it != contiguous_selection.end(); ++it) {
         if ((*it).size() >= 2)
            return true;
      }
      doc->flash_message(_("Best Fit only works on an uninterrupted selection of subtitles."));
      return false;
   }

   /*
    */
   void bestfit(std::vector<Subtitle>& subtitles) {
      if (subtitles.size() < 2)
         return;

      // Get relevant preferences
      SubtitleTime gap = cfg::get_int("timing", "min-gap-between-subtitles");
      SubtitleTime minlen = cfg::get_int("timing", "min-display");
      long minmsecs = minlen.totalmsecs;

      // double mincps = cfg::get_double("timing", "min-characters-per-second");
      // long maxcpl = cfg::get_int("timing", "max-characters-per-line");
      // long maxcps = cfg::get_int("timing", "max-characters-per-second");

      SubtitleTime startime = subtitles.front().get_start();
      SubtitleTime endtime = subtitles.back().get_end();
      SubtitleTime grosstime = endtime - startime;
      long allgaps = gap.totalmsecs * (subtitles.size() - 1);
      long nettime = grosstime.totalmsecs - allgaps;

      std::vector<long> durations(subtitles.size());
      std::vector<long> charcounts(subtitles.size());

      // Get the total character count
      long totalchars = 0;
      for (guint i = 0; i < subtitles.size(); ++i) {
         charcounts[i] = utility::get_text_length_for_timing(subtitles[i].get_text());
         durations[i] = 0;
         totalchars += charcounts[i];
      }

      // Avoid divide by zero
      // Fix bug #23151 : Using best fit subtitles on zero-length subtitles crashes subtitleeditor
      if (totalchars == 0)
         return;

      long charsleft = totalchars;
      long timeleft = nettime;
      bool done = false;      // we're done if no subtitle duration hit the minimum limit
      bool hopeless = false;  // if all durations hit the minimum limit, it's hopeless
      while (!done && !hopeless) {
         done = true;
         hopeless = true;
         for (guint i = 0; i < durations.size(); ++i) {
            if (charcounts[i] >= 0) {
               long d = charcounts[i] * timeleft / charsleft;
               if (d < minmsecs) {
                  charsleft -= charcounts[i];
                  timeleft -= minmsecs;
                  durations[i] = minmsecs;
                  charcounts[i] = -1;
                  done = false;
               } else {
                  durations[i] = d;
                  hopeless = false;
               }
            }
         }
      }

      if (hopeless) {
         // forget subtitle minimum duration and just distribute the time evenly
         for (guint i = 0; i < durations.size(); ++i) {
            durations[i] = utility::get_text_length_for_timing(subtitles[i].get_text()) * nettime / totalchars;
         }
      }

      // time subtitles according to calculated durations
      SubtitleTime intime = subtitles[0].get_start();
      for (guint i = 0; i < durations.size(); ++i) {
         subtitles[i].set_start_and_end(intime, intime + durations[i]);
         intime.totalmsecs += durations[i] + gap.totalmsecs;
      }

      // reset the end time of the last time to original
      // in case rounding errors made it drift away
      subtitles[subtitles.size() - 1].set_end(endtime);
   }

  protected:
   Gtk::UIManager::ui_merge_id ui_id;
   Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(BestFitPlugin)
