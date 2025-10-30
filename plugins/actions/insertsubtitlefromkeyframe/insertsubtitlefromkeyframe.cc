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
#include <keyframes.h>
#include <player.h>

class InsertSubtitleFromKeyframePlugin : public Action {
 public:
  InsertSubtitleFromKeyframePlugin() {
    activate();
    update_ui();
  }

  ~InsertSubtitleFromKeyframePlugin() {
    deactivate();
  }

  void activate() {
    se_dbg(SE_DBG_PLUGINS);

    // actions
    action_group = Gtk::ActionGroup::create("InsertSubtitleFromKeyframePlugin");

    action_group->add(
        Gtk::Action::create("insert-subtitle-between-keyframes",
                            Gtk::Stock::ADD,
                            _("Create Subtitle From Player And Keyframes"),
                            _("Create subtitle automatically according to "
                              "keyframes around the position of the player.")),
        sigc::mem_fun(*this, &InsertSubtitleFromKeyframePlugin::
                                 on_insert_subtitle_between_keyframes));

    action_group->add(
        Gtk::Action::create(
            "insert-subtitle-between-each-keyframes", Gtk::Stock::ADD,
            _("Create Subtitles According Keyframes"),
            _("Create subtitles automatically according to keyframes")),
        sigc::mem_fun(*this, &InsertSubtitleFromKeyframePlugin::
                                 on_insert_subtitle_between_each_keyframes));

    // ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui->insert_action_group(action_group);

    Glib::ustring submenu = R"(
      <ui>
        <menubar name='menubar'>
          <menu name='menu-keyframes' action='menu-keyframes'>
            <placeholder name='placeholder-2'>
              <separator/>
              <menuitem action='insert-subtitle-between-keyframes'/>
              <menuitem action='insert-subtitle-between-each-keyframes'/>
              <separator/>
            </placeholder>
          </menu>
        </menubar>
      </ui>
    )";

    ui_id = ui->add_ui_from_string(submenu);

    // connect the player state signals
    player()->signal_message().connect(sigc::mem_fun(
        *this, &InsertSubtitleFromKeyframePlugin::on_player_message));
  }

  void deactivate() {
    se_dbg(SE_DBG_PLUGINS);

    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui->remove_ui(ui_id);
    ui->remove_action_group(action_group);
  }

  void update_ui() {
    se_dbg(SE_DBG_PLUGINS);

    bool has_doc = (get_current_document() != NULL);
    bool has_kf = static_cast<bool>(player()->get_keyframes());
    bool has_media = player()->get_state() != Player::NONE;

    action_group->get_action("insert-subtitle-between-keyframes")
        ->set_sensitive(has_doc && has_kf && has_media);
    action_group->get_action("insert-subtitle-between-each-keyframes")
        ->set_sensitive(has_doc && has_kf);
  }

  void on_player_message(Player::Message msg) {
    // only if the player is enable or disable
    // don't update if is playing or paused
    if (msg == Player::STREAM_READY || msg == Player::STATE_NONE)
      update_ui();
    else if (msg == Player::KEYFRAME_CHANGED)
      update_ui();
  }

 protected:
  Player *player() {
    return get_subtitleeditor_window()->get_player();
  }

  void on_insert_subtitle_between_keyframes() {
    se_dbg(SE_DBG_PLUGINS);

    long start = 0, end = 0;

    if (!get_keyframes_from_player(start, end))
      return;

    Document *doc = get_current_document();
    g_return_if_fail(doc);

    doc->start_command(_("Insert Subtitle Between Keyframes"));

    Subtitles subtitles = doc->subtitles();

    Subtitle sub = subtitles.append();

    sub.set_start_and_end(start, end);

    doc->subtitles().select(sub);
    doc->subtitles().sort_by_time();

    doc->finish_command();
    doc->emit_signal("subtitle-time-changed");
  }

  // Sets the value of the keyframes positions around the player position and
  // return true.
  bool get_keyframes_from_player(long &start, long &end) {
    Glib::RefPtr<KeyFrames> keyframes = player()->get_keyframes();
    g_return_val_if_fail(keyframes, false);

    long pos = player()->get_position();

    auto prev = keyframes->begin();
    for (auto it = keyframes->begin(); it != keyframes->end(); ++it) {
      if (*it > pos) {
        if (*it != *prev) {
          start = *prev;
          end = *it;
          return true;
        }
      }
      prev = it;
    }
    return false;
  }

  void on_insert_subtitle_between_each_keyframes() {
    Document *doc = get_current_document();
    g_return_if_fail(doc);

    Player *player = get_subtitleeditor_window()->get_player();

    Glib::RefPtr<KeyFrames> keyframes = player->get_keyframes();
    g_return_if_fail(keyframes);

    if (keyframes->size() < 2) {
      doc->flash_message(
          _("Can't insert subtitle between keyframes, not enough keyframes."));
      return;
    }

    int min_display = cfg::get_int("timing", "min-display");
    int count = 0;

    doc->start_command(_("Insert Subtitle Between Each Keyframes"));

    Subtitles subtitles = doc->subtitles();

    KeyFrames::const_iterator it = keyframes->begin();
    KeyFrames::const_iterator prev = it;

    for (++it; it != keyframes->end(); ++it) {
      // Only if the min display is respected
      if ((*it - *prev) >= min_display) {
        Subtitle sub = subtitles.append();
        sub.set_start_and_end(*prev, *it);
        ++count;
      }
      prev = it;
    }
    doc->subtitles().sort_by_time();
    doc->finish_command();
    doc->emit_signal("subtitle-time-changed");

    doc->flash_message(ngettext("1 subtitle has been inserted.",
                                "%d subtitles have been inserted.", count),
                       count);
  }

 protected:
  Gtk::UIManager::ui_merge_id ui_id;
  Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(InsertSubtitleFromKeyframePlugin)
