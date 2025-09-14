// subtitleeditor -- a tool to create or edit subtitle
//
// https://kitone.github.io/subtitleeditor/
// https://github.com/kitone/subtitleeditor/
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
#include <utility.h>

class SplitSelectedSubtitlesPlugin : public Action {
 public:
  SplitSelectedSubtitlesPlugin() {
    activate();
    update_ui();
  }

  ~SplitSelectedSubtitlesPlugin() {
    deactivate();
  }

  void activate() {
    se_dbg(SE_DBG_PLUGINS);

    // actions
    action_group = Gtk::ActionGroup::create("SplitSelectedSubtitlesPlugin");

    action_group->add(
        Gtk::Action::create("split-selected-subtitles", _("_Split"),
                            _("Split the selected subtitles")),
        sigc::mem_fun(*this,
                      &SplitSelectedSubtitlesPlugin::split_selected_subtitles));

    // ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui_id = ui->new_merge_id();

    ui->insert_action_group(action_group);

    ui->add_ui(ui_id, "/menubar/menu-edit/split-selected-subtitles",
               "split-selected-subtitles", "split-selected-subtitles");
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

    action_group->get_action("split-selected-subtitles")
        ->set_sensitive(visible);
  }

  void split_selected_subtitles() {
    se_dbg(SE_DBG_PLUGINS);

    Document *doc = get_current_document();

    g_return_if_fail(doc);

    Subtitles subtitles = doc->subtitles();

    std::vector<Subtitle> selection = subtitles.get_selection();

    if (selection.empty()) {
      doc->flash_message(_("Please select at least one subtitle."));
      return;
    }

    doc->start_command(_("Split subtitles"));

    for (auto it = selection.rbegin(); it != selection.rend(); ++it) {
      split(subtitles, *it);
    }

    doc->emit_signal("subtitle-time-changed");
    doc->finish_command();
  }

  void split(Subtitles &subtitles, Subtitle &sub) {
    Glib::RefPtr<Glib::Regex> re = Glib::Regex::create("\\n");

    Glib::ustring text = sub.get_text();
    Glib::ustring translation = sub.get_translation();

    std::vector<Glib::ustring> lines = re->split(text);
    std::vector<Glib::ustring> translation_lines = re->split(translation);

    // If there's not at least two lines, it's not necessary to split
    if (lines.size() < 2)
      return;

    fix_multiline_tag(lines);

    // Original values
    Glib::ustring otext = text;
    SubtitleTime ostart = sub.get_start();
    SubtitleTime oduration = sub.get_duration();

    // Array for new subtitles
    std::vector<Subtitle> newsubs;
    // Represente the total number of characters in the original subtitles
    // (without tag)
    unsigned int total_chars = 0;

    // We can add directly the original subtitle, it will be reused
    // we just need to add other lines (size-1)
    newsubs.push_back(sub);

    for (unsigned int i = 1; i < lines.size(); ++i) {
      Subtitle next = subtitles.insert_after(newsubs[i - 1]);
      sub.copy_to(next);  // Copy all values (style, note, ...)
      newsubs.push_back(next);
    }

    // Updated subtitles text with each line
    for (unsigned int i = 0; i < newsubs.size(); ++i) {
      newsubs[i].set_text(lines[i]);
	  // This will split translation if it exists; if it has less lines, 
	  // the last lines of new translation will be empty
	  if (i < translation_lines.size()) {
        newsubs[i].set_translation(translation_lines[i]);
	  } else {
        newsubs[i].set_translation("");
	  }
      // We take the loop to calculate the total number of characters
      total_chars += utility::get_stripped_text(lines[i]).size();
    }

    // Now we set the time for each subtitle
    // Divides the original duration by using the ratio to the number of
    // characters per line If the total number of characters is equal to 0
    // (empty lines) we divide by the number of lines

    SubtitleTime start = ostart;
    SubtitleTime dur;

    for (unsigned int i = 0; i < newsubs.size(); ++i) {
      if (total_chars > 0) {
        auto l_size = static_cast<double>(lines[i].size());
        auto t_chars = static_cast<double>(total_chars);
        dur = oduration * (l_size / t_chars);
      } else {
        dur = oduration / static_cast<long>(newsubs.size());
      }
      newsubs[i].set_start_and_end(start, start + dur);

      start = start + dur;  // Update the beginning of the next subtitle
    }

    // Try to apply timing prefs like gap between subs...
    try_to_respect_timing_preferences(newsubs);

    // We add to the selection the new subtitles
    subtitles.select(newsubs);
  }

  // Try to apply some timing preferences
  // - minimum gap between subtitles
  // - minimum display time (TODO ?)
  void try_to_respect_timing_preferences(std::vector<Subtitle> &subs) {
    // int min_display = cfg::get_int("timing", "min-display");
    int min_gap = cfg::get_int("timing", "min-gap-between-subtitles");

    SubtitleTime gap = SubtitleTime(min_gap) * 0.5;
    SubtitleTime tmp;
    for (unsigned int i = 0; i < subs.size(); ++i) {
      SubtitleTime start = subs[i].get_start();
      SubtitleTime end = subs[i].get_end();

      if (i > 0)  // Move the beginning if it's not the first subtitle
        start = start + gap;

      if (i < subs.size() - 1)  // Move the end if it's not the last subtitle
        end = end - gap;

      subs[i].set_start_and_end(start, end);
    }
  }

  void fix_multiline_tag(std::vector<Glib::ustring> &lines) {
    Glib::RefPtr<Glib::Regex> re_tag_open = Glib::Regex::create("<(\\w+)>");

    for (auto it = lines.begin(); it != lines.end(); ++it) {
      // Is there a tag open ?
      if (re_tag_open->match(*it)) {
        std::vector<Glib::ustring> matches = re_tag_open->split(*it);

        Glib::ustring tag = matches[1];

        Glib::RefPtr<Glib::Regex> re_tag_close =
            Glib::Regex::create(Glib::ustring::compose("</(%1)>", tag));

        // The tag open is closed on the same line?
        if (re_tag_close->match(*it) == false) {
          // No, we close the tag at the end of this line
          // and add an opened tag to the start of the next.
          *it = Glib::ustring::compose("%1</%2>", *it, tag);

          std::vector<Glib::ustring>::iterator it_next = it;
          ++it_next;
          if (it_next != lines.end()) {  // Only if there is a next line
            *it_next = Glib::ustring::compose("<%1>%2", tag, *it_next);
          }
        }
      }
    }
  }

 protected:
  Gtk::UIManager::ui_merge_id ui_id;
  Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(SplitSelectedSubtitlesPlugin)
