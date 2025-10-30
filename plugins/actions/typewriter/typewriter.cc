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
#include <algorithm>

class TypewriterPlugin : public Action {
 public:
  TypewriterPlugin() {
    activate();
    update_ui();
  }

  ~TypewriterPlugin() {
    deactivate();
  }

  void activate() {
    se_dbg(SE_DBG_PLUGINS);

    // actions
    action_group = Gtk::ActionGroup::create("TypewriterPlugin");

    action_group->add(Gtk::Action::create("typewriter", _("_Typewriter")));

    action_group->add(
        Gtk::Action::create("typewriter-characters-linear",
                            _("Characters — Linear")),
        sigc::bind(
            sigc::mem_fun(*this, &TypewriterPlugin::split_selected_subtitles),
            CHARACTERS, LINEAR));

    action_group->add(
        Gtk::Action::create("typewriter-characters-random",
                            _("Characters — Random")),
        sigc::bind(
            sigc::mem_fun(*this, &TypewriterPlugin::split_selected_subtitles),
            CHARACTERS, RANDOM));

    action_group->add(
        Gtk::Action::create("typewriter-words-linear", _("Words — Linear")),
        sigc::bind(
            sigc::mem_fun(*this, &TypewriterPlugin::split_selected_subtitles),
            WORDS, LINEAR));

    action_group->add(
        Gtk::Action::create("typewriter-words-random", _("Words — Random")),
        sigc::bind(
            sigc::mem_fun(*this, &TypewriterPlugin::split_selected_subtitles),
            WORDS, RANDOM));

    // ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui->insert_action_group(action_group);

    Glib::ustring submenu = R"(
      <ui>
        <menubar name='menubar'>
          <menu name='menu-extensions' action='menu-extensions'>
            <placeholder name='placeholder'>
              <menu action='typewriter'>
                <menuitem action='typewriter-characters-linear'/>
                <menuitem action='typewriter-characters-random'/>
                <separator/>
                <menuitem action='typewriter-words-linear'/>
                <menuitem action='typewriter-words-random'/>
              </menu>
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

    action_group->get_action("typewriter-characters-linear")
        ->set_sensitive(visible);
    action_group->get_action("typewriter-characters-random")
        ->set_sensitive(visible);
    action_group->get_action("typewriter-words-linear")->set_sensitive(visible);
    action_group->get_action("typewriter-words-random")->set_sensitive(visible);
  }

 protected:
  enum SPLIT_TYPE { CHARACTERS, WORDS };

  enum SPLIT_TIME { LINEAR, RANDOM };

  void split_selected_subtitles(SPLIT_TYPE split_type, SPLIT_TIME split_time) {
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
    // To keep subtitles valid, we start change from the end
    for (std::vector<Subtitle>::reverse_iterator it = selection.rbegin();
         it != selection.rend(); ++it) {
      split(subtitles, *it, split_type, split_time);
    }
    doc->emit_signal("subtitle-time-changed");
    doc->finish_command();
  }

  void split(Subtitles &subtitles, Subtitle &sub, SPLIT_TYPE type,
             SPLIT_TIME time) {
    Glib::ustring text = sub.get_text();
    if (text.empty())
      return;
    // Original values
    SubtitleTime ostart = sub.get_start();
    SubtitleTime oduration = sub.get_duration();

    // Array for new subtitles
    std::vector<Subtitle> newsubs;

    // Array for the split text (characters, word ...)
    std::vector<Glib::ustring> vtext;

    if (type == CHARACTERS)
      vtext = split_by_character(text);
    else if (type == WORDS)
      vtext = split_by_word(text);

    // Create really the subtitles
    newsubs = create_subtitles_from_text_array(subtitles, sub, vtext);

    // Setup time
    if (time == LINEAR)
      setup_time_linear(newsubs, ostart, oduration);
    else if (time == RANDOM)
      setup_time_random(newsubs, ostart, oduration);

    // We add to the selection the new subtitles
    subtitles.select(newsubs);
  }

  std::vector<Subtitle> create_subtitles_from_text_array(
      Subtitles &subtitles, Subtitle &original_subtitle,
      const std::vector<Glib::ustring> &vtext) {
    // Array for new subtitles
    std::vector<Subtitle> newsubs;

    // We can add directly the original subtitle, it will be reused
    // we just need to add other lines (size-1)
    newsubs.push_back(original_subtitle);

    // Create subtitle foreach text in the array
    // start at 1 because we already add the original subtitle in the list
    for (guint c = 1; c < vtext.size(); ++c) {
      Subtitle next = subtitles.insert_after(newsubs[c - 1]);
      original_subtitle.copy_to(next);  // Copy all values (style, note...)
      newsubs.push_back(next);
    }
    // Update subtitles text from vtext
    for (guint i = 0; i < vtext.size(); ++i) {
      newsubs[i].set_text(vtext[i]);
    }
    return newsubs;
  }

  std::vector<Glib::ustring> split_by_character(const Glib::ustring &text) {
    std::vector<Glib::ustring> characters;

    characters.resize(text.size());
    for (guint i = 1; i <= text.size(); ++i) {
      characters[i - 1] = text.substr(0, i);
    }
    return characters;
  }

  std::vector<Glib::ustring> split_by_word(const Glib::ustring &text) {
    std::vector<Glib::ustring> splitted, words;
    // If you have a better idea, fell free to send a patch.
    splitted = Glib::Regex::split_simple("\\s", text);

    for (guint i = 0; i < splitted.size(); ++i) {
      Glib::ustring w;
      for (guint j = 0; j <= i; ++j) {
        if (j > 0)
          w += text.at(w.size());
        w += splitted[j];
      }
      words.push_back(w);
    }

    return words;
  }

  void setup_time_linear(std::vector<Subtitle> &subs, const SubtitleTime &start,
                         const SubtitleTime &duration) {
    // We update the time of each subtitles (linear)
    SubtitleTime s = start;
    SubtitleTime d = duration / static_cast<long>(subs.size());

    for (guint i = 0; i < subs.size(); ++i) {
      subs[i].set_start_and_end(s, s + d);
      s = s + d;  // Update the beginning of the next subtitle
    }
  }

  void setup_time_random(std::vector<Subtitle> &subs, const SubtitleTime &start,
                         const SubtitleTime &duration) {
    std::vector<long> rand_times;
    // Create a random time between 0 and duration
    // and sort the random times
    Glib::Rand rand(start.totalmsecs);
    for (guint i = 0; i < subs.size(); ++i)
      rand_times.push_back(rand.get_int_range(0, duration.totalmsecs));
    std::sort(rand_times.begin(), rand_times.end());

    // We update the time of each subtitles (linear)
    SubtitleTime s = start;

    for (guint i = 0; i < subs.size(); ++i) {
      SubtitleTime e(start.totalmsecs + rand_times[i]);

      subs[i].set_start_and_end(s, e);

      s = e;  // Update the beginning of the next subtitle
    }
  }

 protected:
  Gtk::UIManager::ui_merge_id ui_id;
  Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(TypewriterPlugin)
