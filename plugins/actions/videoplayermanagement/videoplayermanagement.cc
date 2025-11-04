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

#include <extension/action.h>
#include <gui/dialogfilechooser.h>
#include <player.h>
#include <utility.h>

// Video Player Management
class VideoPlayerManagement : public Action {
 public:
  VideoPlayerManagement() {
    activate();
    update_ui();
  }

  ~VideoPlayerManagement() {
    deactivate();
  }

  void activate() {
    se_dbg(SE_DBG_PLUGINS);

    // actions
    action_group = Gtk::ActionGroup::create("VideoPlayerManagement");

    // Already create in MenuBar.cc
    action_group->add(Gtk::Action::create("menu-video", _("_Video")));

    action_group->add(
        Gtk::Action::create("video-player/open", Gtk::Stock::OPEN,
                            "",  // _("_Open Media"),
                            _("Open an audio file or a video file")),
        Gtk::AccelKey("<Shift><Control>M"),
        sigc::mem_fun(*this, &VideoPlayerManagement::on_open));

    action_group->add(
        Gtk::Action::create("video-player/close", Gtk::Stock::CLOSE,
                            "",  // _("_Close Media"),
                            _("Close an audio file or a video file")),
        Gtk::AccelKey("<Shift><Control>C"),
        sigc::mem_fun(*this, &VideoPlayerManagement::on_close));

    action_group->add(
        Gtk::Action::create("video-player/play", Gtk::Stock::MEDIA_PLAY,
                            _("Play"), _("Play audio or video")),
        sigc::mem_fun(*this, &VideoPlayerManagement::on_play));

    action_group->add(
        Gtk::Action::create("video-player/pause", Gtk::Stock::MEDIA_PAUSE,
                            _("Pause"), _("Pause audio or video")),
        sigc::mem_fun(*this, &VideoPlayerManagement::on_pause));

    action_group->add(
        Gtk::Action::create("video-player/play-pause", _("_Play / Pause"),
                            _("Play or pause audio or video")),
        sigc::mem_fun(*this, &VideoPlayerManagement::on_play_pause));

    // Seek Backwards
    // FIXME Ideally the amount by which we skip would be in the tooltip and
    // there would be a configuration dialogue for it
    action_group->add(Gtk::Action::create(
        "video-player/menu-skip-backwards", Gtk::Stock::MEDIA_REWIND,
        _("Skip _Backwards"), _("Skip Backwards")));

    action_group->add(
        Gtk::Action::create("video-player/skip-backwards-frame",
                            Gtk::Stock::MEDIA_REWIND, _("Frame"),
                            _("Skip backwards by a frame")),
        sigc::bind(
            sigc::mem_fun(*this, &VideoPlayerManagement::on_skip_backwards),
            FRAME));

    action_group->add(
        Gtk::Action::create(
            "video-player/skip-backwards-tiny", Gtk::Stock::MEDIA_REWIND,
            _("Tiny"),
            _("Tiny skip backwards — Configure amount in miliseconds in your "
              "text configuration file under section [video-player]; the "
              "configuration file is usually located at "
              "~/.config/subtitleeditor/default/config")),
        sigc::bind(
            sigc::mem_fun(*this, &VideoPlayerManagement::on_skip_backwards),
            TINY));

    action_group->add(
        Gtk::Action::create(
            "video-player/skip-backwards-very-short", Gtk::Stock::MEDIA_REWIND,
            _("Very Short"),
            _("Very short skip backwards — Configure amount in seconds in your "
              "text configuration file under section [video-player]; the "
              "configuration file is usually located at "
              "~/.config/subtitleeditor/default/config")),
        sigc::bind(
            sigc::mem_fun(*this, &VideoPlayerManagement::on_skip_backwards),
            VERY_SHORT));

    action_group->add(
        Gtk::Action::create(
            "video-player/skip-backwards-short", Gtk::Stock::MEDIA_REWIND,
            _("Short"),
            _("Short skip backwards — Configure amount in seconds in your text "
              "configuration file under section [video-player]; the "
              "configuration file is usually located at "
              "~/.config/subtitleeditor/default/config")),
        sigc::bind(
            sigc::mem_fun(*this, &VideoPlayerManagement::on_skip_backwards),
            SHORT));

    action_group->add(
        Gtk::Action::create(
            "video-player/skip-backwards-medium", Gtk::Stock::MEDIA_REWIND,
            _("Medium"),
            _("Medium skip backwards — Configure amount in seconds in your "
              "text configuration file under section [video-player]; the "
              "configuration file is usually located at "
              "~/.config/subtitleeditor/default/config")),
        sigc::bind(
            sigc::mem_fun(*this, &VideoPlayerManagement::on_skip_backwards),
            MEDIUM));

    action_group->add(
        Gtk::Action::create(
            "video-player/skip-backwards-long", Gtk::Stock::MEDIA_REWIND,
            _("Long"),
            _("Long skip backwards — Configure amount in seconds in your text "
              "configuration file under section [video-player]; the "
              "configuration file is usually located at "
              "~/.config/subtitleeditor/default/config")),
        sigc::bind(
            sigc::mem_fun(*this, &VideoPlayerManagement::on_skip_backwards),
            LONG));

    // Seek Forward
    action_group->add(Gtk::Action::create(
        "video-player/menu-skip-forward", Gtk::Stock::MEDIA_FORWARD,
        _("Skip _Forward"), _("Skip Forward")));

    action_group->add(
        Gtk::Action::create("video-player/skip-forward-frame",
                            Gtk::Stock::MEDIA_FORWARD, _("Frame"),
                            _("Skip forward by a frame")),
        sigc::bind(
            sigc::mem_fun(*this, &VideoPlayerManagement::on_skip_forward),
            FRAME));

    action_group->add(
        Gtk::Action::create(
            "video-player/skip-forward-tiny", Gtk::Stock::MEDIA_FORWARD,
            _("Tiny"),
            _("Tiny skip forward — Configure amount in miliseconds in your "
              "text configuration file under section [video-player]; the "
              "configuration file is usually located at "
              "~/.config/subtitleeditor/default/config")),
        sigc::bind(
            sigc::mem_fun(*this, &VideoPlayerManagement::on_skip_forward),
            TINY));

    action_group->add(
        Gtk::Action::create(
            "video-player/skip-forward-very-short", Gtk::Stock::MEDIA_FORWARD,
            _("Very Short"),
            _("Very short skip forward — Configure amount in seconds in your "
              "text configuration file under section [video-player]; the "
              "configuration file is usually located at "
              "~/.config/subtitleeditor/default/config")),
        sigc::bind(
            sigc::mem_fun(*this, &VideoPlayerManagement::on_skip_forward),
            VERY_SHORT));

    action_group->add(
        Gtk::Action::create(
            "video-player/skip-forward-short", Gtk::Stock::MEDIA_FORWARD,
            _("Short"),
            _("Short skip forward — Configure amount in seconds in your text "
              "configuration file under section [video-player]; the "
              "configuration file is usually located at "
              "~/.config/subtitleeditor/default/config")),
        sigc::bind(
            sigc::mem_fun(*this, &VideoPlayerManagement::on_skip_forward),
            SHORT));

    action_group->add(
        Gtk::Action::create(
            "video-player/skip-forward-medium", Gtk::Stock::MEDIA_FORWARD,
            _("Medium"),
            _("Medium skip forward — Configure amount in seconds in your text "
              "configuration file under section [video-player]; the "
              "configuration file is usually located at "
              "~/.config/subtitleeditor/default/config")),
        sigc::bind(
            sigc::mem_fun(*this, &VideoPlayerManagement::on_skip_forward),
            MEDIUM));

    action_group->add(
        Gtk::Action::create(
            "video-player/skip-forward-long", Gtk::Stock::MEDIA_FORWARD,
            _("Long"),
            _("Long skip forward — Configure amount in seconds in your text "
              "configuration file under section [video-player]; the "
              "configuration file is usually located at "
              "~/.config/subtitleeditor/default/config")),
        sigc::bind(
            sigc::mem_fun(*this, &VideoPlayerManagement::on_skip_forward),
            LONG));

    // Rate Slower & Faster
    action_group->add(Gtk::Action::create("video-player/menu-rate", _("Rate"),
                                          _("Set the playback rate")));

    action_group->add(
        Gtk::Action::create(
            "video-player/rate-slower", _("_Slower"),
            _("Decrease the playback rate by 0.1 (in range 0.1—1.5)")),
        sigc::mem_fun(*this, &VideoPlayerManagement::on_playback_rate_slower));

    action_group->add(
        Gtk::Action::create(
            "video-player/rate-faster", _("_Faster"),
            _("Increase the playback rate by 0.1 (in range 0.1—1.5)")),
        sigc::mem_fun(*this, &VideoPlayerManagement::on_playback_rate_faster));

    action_group->add(
        Gtk::Action::create("video-player/rate-normal", _("_Normal"),
                            _("Set the playback rate to normal (1.0)")),
        sigc::mem_fun(*this, &VideoPlayerManagement::on_playback_rate_normal));

    // Seek to Selection
    action_group->add(
        Gtk::Action::create(
            "video-player/seek-to-selection", _("_Seek To Selection"),
            _("Seek to the start of the first selected subtitle")),
        sigc::mem_fun(*this, &VideoPlayerManagement::on_seek_to_selection));

    // Seek to Selection End
    action_group->add(
        Gtk::Action::create("video-player/seek-to-selection-end",
                            _("_Seek To Selection End"),
                            _("Seek to the end of the last selected subtitle")),
        sigc::mem_fun(*this, &VideoPlayerManagement::on_seek_to_selection_end));

    // Repeat
    bool video_repeat_state = cfg::get_boolean("video-player", "repeat");

    action_group->add(
        Gtk::ToggleAction::create("video-player/repeat", _("_Repeat"),
                                  _("Enable or disable the repeat mode"),
                                  video_repeat_state),
        sigc::mem_fun(*this,
                      &VideoPlayerManagement::on_video_player_repeat_toggled));

    // Play Subtitle
    action_group->add(
        Gtk::Action::create("video-player/play-previous-subtitle",
                            Gtk::Stock::MEDIA_PREVIOUS,
                            _("Play _Previous Subtitle"),
                            _("Play the subtitle that is previous to the first "
                              "selected subtitle")),
        sigc::mem_fun(*this,
                      &VideoPlayerManagement::on_play_previous_subtitle));

    action_group->add(
        Gtk::Action::create("video-player/play-current-subtitle",
                            Gtk::Stock::MEDIA_PLAY, _("Play _Selection"),
                            _("Play the selected subtitle")),
        sigc::mem_fun(*this, &VideoPlayerManagement::on_play_current_subtitle));

    action_group->add(
        Gtk::Action::create(
            "video-player/play-next-subtitle", Gtk::Stock::MEDIA_NEXT,
            _("Play _Next Subtitle"),
            _("Play the subtitle that is next to the first selected subtitle")),
        sigc::mem_fun(*this, &VideoPlayerManagement::on_play_next_subtitle));

    // Play Second
    action_group->add(
        Gtk::Action::create(
            "video-player/play-previous-second", _("Play Previous Second"),
            _("Play the second preceding the first selected subtitle")),
        sigc::mem_fun(*this, &VideoPlayerManagement::on_play_previous_second));

    action_group->add(
        Gtk::Action::create(
            "video-player/play-first-second", _("Play First Second"),
            _("Play the first second of the subtitle currently selected")),
        sigc::mem_fun(*this, &VideoPlayerManagement::on_play_first_second));

    action_group->add(
        Gtk::Action::create(
            "video-player/play-last-second", _("Play Last Second"),
            _("Play the last second of the subtitle currently selected")),
        sigc::mem_fun(*this, &VideoPlayerManagement::on_play_last_second));

    action_group->add(
        Gtk::Action::create(
            "video-player/play-next-second", _("Play Next Second"),
            _("Play the second following the subtitle currently selected")),
        sigc::mem_fun(*this, &VideoPlayerManagement::on_play_next_second));

    // Display Video Player
    bool vp_display_state = cfg::get_boolean("video-player", "display");

    action_group->add(
        Gtk::ToggleAction::create(
            "video-player/display", _("_Video Player"),
            _("Show or hide the video player in the current window"),
            vp_display_state),
        sigc::mem_fun(*this,
                      &VideoPlayerManagement::on_video_player_display_toggled));

    action_group->add(Gtk::Action::create("menu-audio-track", _("Audio Track"),
                                          _("Choose an audio track")));

    // Recent files
    Glib::RefPtr<Gtk::RecentAction> recentAction =
        Gtk::RecentAction::create("video-player/recent-files",
                                  _("_Recent Files"), _("Open a recent file"));

    Glib::RefPtr<Gtk::RecentFilter> filter = Gtk::RecentFilter::create();
    filter->set_name("subtitleeditor");
    filter->add_group("subtitleeditor-video-player");
    recentAction->set_filter(filter);
    recentAction->set_show_icons(false);
    recentAction->set_show_numbers(true);
    recentAction->set_show_tips(true);
    // recentAction->set_show_not_found(false);
    recentAction->set_sort_type(Gtk::RECENT_SORT_MRU);

    recentAction->signal_item_activated().connect(
        sigc::mem_fun(*this, &VideoPlayerManagement::on_recent_item_activated));
    action_group->add(recentAction);

    // ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui->insert_action_group(action_group);

    Glib::ustring submenu = R"(
      <ui>
        <menubar name='menubar'>
          <menu name='menu-video' action='menu-video'>
            <placeholder name='video-player-management'>
              <menuitem action='video-player/open'/>
              <menuitem action='video-player/recent-files'/>
              <menuitem action='video-player/close'/>
              <separator/>
              <menu action='menu-audio-track'>
                <placeholder name='audio-track-placeholder'/>
              </menu>
              <separator/>
              <menuitem action='video-player/play'/>
              <menuitem action='video-player/pause'/>
              <menuitem action='video-player/play-pause'/>
              <separator/>
              <menu action='video-player/menu-skip-forward'>
                <menuitem action='video-player/skip-forward-frame'/>
                <menuitem action='video-player/skip-forward-tiny'/>
                <menuitem action='video-player/skip-forward-very-short'/>
                <menuitem action='video-player/skip-forward-short'/>
                <menuitem action='video-player/skip-forward-medium'/>
                <menuitem action='video-player/skip-forward-long'/>
              </menu>
              <menu action='video-player/menu-skip-backwards'>
                <menuitem action='video-player/skip-backwards-frame'/>
                <menuitem action='video-player/skip-backwards-tiny'/>
                <menuitem action='video-player/skip-backwards-very-short'/>
                <menuitem action='video-player/skip-backwards-short'/>
                <menuitem action='video-player/skip-backwards-medium'/>
                <menuitem action='video-player/skip-backwards-long'/>
              </menu>
              <menu action='video-player/menu-rate'>
                <menuitem action='video-player/rate-slower'/>
                <menuitem action='video-player/rate-faster'/>
                <menuitem action='video-player/rate-normal'/>
              </menu>
              <separator/>
              <menuitem action='video-player/seek-to-selection'/>
              <menuitem action='video-player/seek-to-selection-end'/>
              <separator/>
              <menuitem action='video-player/play-current-subtitle'/>
              <menuitem action='video-player/play-next-subtitle'/>
              <menuitem action='video-player/play-previous-subtitle'/>
              <menuitem action='video-player/repeat'/>
              <separator/>
              <menuitem action='video-player/play-previous-second'/>
              <menuitem action='video-player/play-first-second'/>
              <menuitem action='video-player/play-last-second'/>
              <menuitem action='video-player/play-next-second'/>
            </placeholder>
          </menu>
        </menubar>
      </ui>
    )";

    ui_id = ui->add_ui_from_string(submenu);
    ui_id_audio = ui->new_merge_id();

    // Show/Hide video player
    ui->add_ui(ui_id, "/menubar/menu-view/display-placeholder",
               "video-player/display", "video-player/display");

    //
    player()->signal_message().connect(
        sigc::mem_fun(*this, &VideoPlayerManagement::on_player_message));

    cfg::signal_changed("video-player")
        .connect(sigc::mem_fun(
            *this, &VideoPlayerManagement::on_config_video_player_changed));
  }

  void deactivate() {
    se_dbg(SE_DBG_PLUGINS);

    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    remove_menu_audio_track();
    ui->remove_ui(ui_id);
    ui->remove_action_group(action_group);
  }

  // Update the user interface with the state of subtitle (has document)
  // and the state of the player (has media)
  void update_ui() {
    se_dbg(SE_DBG_PLUGINS);

    bool has_doc = (get_current_document() != NULL);
    bool has_media = player()->get_state() != Player::NONE;

#define SET_SENSITIVE(action, state)                                  \
  {                                                                   \
    Glib::RefPtr<Gtk::Action> act = action_group->get_action(action); \
    if (act)                                                          \
      act->set_sensitive(state);                                      \
    else                                                              \
      g_warning(action);                                              \
  }

    SET_SENSITIVE("video-player/play", has_media);
    SET_SENSITIVE("video-player/close", has_media);
    SET_SENSITIVE("video-player/pause", has_media);
    SET_SENSITIVE("video-player/play-pause", has_media);

    SET_SENSITIVE("video-player/rate-slower", has_media);
    SET_SENSITIVE("video-player/rate-faster", has_media);
    SET_SENSITIVE("video-player/rate-normal", has_media);

    SET_SENSITIVE("video-player/skip-forward-frame", has_media);
    SET_SENSITIVE("video-player/skip-forward-tiny", has_media);
    SET_SENSITIVE("video-player/skip-forward-very-short", has_media);
    SET_SENSITIVE("video-player/skip-forward-short", has_media);
    SET_SENSITIVE("video-player/skip-forward-medium", has_media);
    SET_SENSITIVE("video-player/skip-forward-long", has_media);

    SET_SENSITIVE("video-player/skip-backwards-frame", has_media);
    SET_SENSITIVE("video-player/skip-backwards-tiny", has_media);
    SET_SENSITIVE("video-player/skip-backwards-very-short", has_media);
    SET_SENSITIVE("video-player/skip-backwards-short", has_media);
    SET_SENSITIVE("video-player/skip-backwards-medium", has_media);
    SET_SENSITIVE("video-player/skip-backwards-long", has_media);

    SET_SENSITIVE("video-player/repeat", has_media);

    SET_SENSITIVE("video-player/seek-to-selection", has_media && has_doc);
    SET_SENSITIVE("video-player/seek-to-selection-end", has_media && has_doc);

    SET_SENSITIVE("video-player/play-previous-subtitle", has_media && has_doc);
    SET_SENSITIVE("video-player/play-current-subtitle", has_media && has_doc);
    SET_SENSITIVE("video-player/play-next-subtitle", has_media && has_doc);

    SET_SENSITIVE("video-player/play-previous-second", has_media && has_doc);
    SET_SENSITIVE("video-player/play-first-second", has_media && has_doc);
    SET_SENSITIVE("video-player/play-last-second", has_media && has_doc);
    SET_SENSITIVE("video-player/play-next-second", has_media && has_doc);

#undef SET_SENSITIVE
  }

  // Check the state of the player.
  // Display the video player if need and update the menu.
  void on_player_message(Player::Message msg) {
    if (msg == Player::STATE_NONE || msg == Player::STREAM_READY) {
      // only if the player is enable or disable
      // don't update if is playing or paused

      if (msg == Player::STATE_NONE) {
        remove_menu_audio_track();
      } else if (msg == Player::STREAM_READY) {
        build_menu_audio_track();
        add_in_recent_manager(player()->get_uri());
      }
      update_ui();
      // If need, ask to display the video player
      if (msg == Player::STREAM_READY) {
        if (cfg::get_boolean("video-player", "display") == false) {
          cfg::set_boolean("video-player", "display", true);
        }
      }
    } else if (msg == Player::STREAM_AUDIO_CHANGED) {
      // The player emit the signal audio changed.
      // We update the current audio if need.
      update_audio_track_from_player();
    }
  }

  // Show or hide the video player, update hte config.
  void on_video_player_display_toggled() {
    Glib::RefPtr<Gtk::ToggleAction> action =
        Glib::RefPtr<Gtk::ToggleAction>::cast_static(
            action_group->get_action("video-player/display"));
    if (action) {
      bool state = action->get_active();
      if (cfg::get_boolean("video-player", "display") != state) {
        cfg::set_boolean("video-player", "display", state);
      }
    }
  }

  // The state of reapet has changed, update the config.
  void on_video_player_repeat_toggled() {
    Glib::RefPtr<Gtk::ToggleAction> action =
        Glib::RefPtr<Gtk::ToggleAction>::cast_static(
            action_group->get_action("video-player/repeat"));
    if (action) {
      bool state = action->get_active();
      if (cfg::get_boolean("video-player", "repeat") != state) {
        cfg::set_boolean("video-player", "repeat", state);
      }
    }
  }

  // The video player config has changed.
  // Update the menu.
  void on_config_video_player_changed(const Glib::ustring &key,
                                      const Glib::ustring &value) {
    if (key == "display") {
      bool state = utility::string_to_bool(value);

      Glib::RefPtr<Gtk::ToggleAction> action =
          Glib::RefPtr<Gtk::ToggleAction>::cast_static(
              action_group->get_action("video-player/display"));
      if (action) {
        if (action->get_active() != state)
          action->set_active(state);
      }
    } else if (key == "repeat") {
      bool state = utility::string_to_bool(value);

      Glib::RefPtr<Gtk::ToggleAction> action =
          Glib::RefPtr<Gtk::ToggleAction>::cast_static(
              action_group->get_action("video-player/repeat"));
      if (action) {
        if (action->get_active() != state)
          action->set_active(state);
      }
    }
  }

  // We remove the ActionGroup "VideoPlayerManagementAudioTrack"
  // and the ui.
  void remove_menu_audio_track() {
    se_dbg(SE_DBG_PLUGINS);
    if (action_group_audio) {
      get_ui_manager()->remove_ui(ui_id_audio);
      get_ui_manager()->remove_action_group(action_group_audio);
      action_group_audio.reset();
    }
  }

  // Remove old menu items (tracks) and actions
  // and create a new one.
  void build_menu_audio_track() {
    se_dbg(SE_DBG_PLUGINS);
    // We clean the old audio menu
    remove_menu_audio_track();
    // Create audio actions
    action_group_audio =
        Gtk::ActionGroup::create("VideoPlayerManagementAudioTrack");
    get_ui_manager()->insert_action_group(action_group_audio);

    Gtk::RadioButtonGroup group;
    // A default track "Auto"
    add_audio_track_entry(group, "audio-track-auto", _("Auto"), -1);
    // Now we build an entry for each audio track
    gint n_audio = player()->get_n_audio();
    for (gint i = 0; i < n_audio; ++i) {
      Glib::ustring track = Glib::ustring::compose("audio-track-%1", i);
      Glib::ustring track_name = Glib::ustring::compose("Track %1", i + 1);

      add_audio_track_entry(group, track, track_name, i);
    }
    // active the good track
    update_audio_track_from_player();
  }

  // Update the radio item with the current audio track
  // from the player.
  void update_audio_track_from_player() {
    se_dbg(SE_DBG_PLUGINS);

    if (!action_group_audio)
      return;

    gint current_audio = player()->get_n_audio();
    // If it's < -1, we choose the track "Auto"
    Glib::ustring track_action =
        (current_audio < 0)
            ? "audio-track-auto"
            : Glib::ustring::compose("audio-track-%1", current_audio);

    Glib::RefPtr<Gtk::ToggleAction> action =
        Glib::RefPtr<Gtk::ToggleAction>::cast_static(
            action_group_audio->get_action(track_action));
    if (action) {
      if (action->get_active() == false)  // Only if need
        action->set_active(true);
    }
  }

  // Create a new track entry (action and menu item).
  void add_audio_track_entry(Gtk::RadioButtonGroup &group,
                             const Glib::ustring &track_action,
                             const Glib::ustring &track_label,
                             gint track_number) {
    // action
    Glib::RefPtr<Gtk::RadioAction> action =
        Gtk::RadioAction::create(group, track_action, track_label);
    action_group_audio->add(
        action,
        sigc::bind(
            sigc::mem_fun(*this, &VideoPlayerManagement::set_current_audio),
            track_number, action));
    // menuitem
    get_ui_manager()->add_ui(ui_id_audio,
                             "/menubar/menu-video/video-player-management/"
                             "menu-audio-track/audio-track-placeholder",
                             track_action, track_action, Gtk::UI_MANAGER_AUTO,
                             false);
    // update
    get_ui_manager()->ensure_update();
  }

  // The user choose a new track from the track menu,
  // we update the player.
  void set_current_audio(gint track, Glib::RefPtr<Gtk::RadioAction> action) {
    se_dbg(SE_DBG_PLUGINS);
    // Switching a toggle button launch two signal,
    // one for the button toggle to unactivated and an other to activated.
    // We need to check only for the signal activate.
    if (!action->get_active())
      return;
    player()->set_current_audio(track);
  }

 protected:
  // Return the GStreamer Player.
  Player *player() {
    return get_subtitleeditor_window()->get_player();
  }

  // Open the dialog "Open Video" and initialize the player with the new uri.
  void on_open() {
    DialogOpenVideo ui;

    if (ui.run() == Gtk::RESPONSE_OK) {
      ui.hide();

      Glib::ustring uri = ui.get_uri();

      player()->open(uri);

      add_in_recent_manager(uri);
    }
  }

  // Close the player
  void on_close() {
    player()->close();
  }

  // Reinitialize the current position for disable the repeat method
  // and sets the player state to playing.
  void on_play() {
    player()->seek(player()->get_position());
    player()->play();
  }

  // Sets the player state to paused.
  void on_pause() {
    player()->pause();
  }

  // Toggled the player state.
  // Paused to playing or playing to paused.
  void on_play_pause() {
    if (player()->is_playing()) {
      player()->pause();
    } else {
      player()->seek(player()->get_position());
      player()->play();
    }
  }

  // Skip type, look the config for the value of the time.
  enum SkipType { FRAME, TINY, VERY_SHORT, SHORT, MEDIUM, LONG };

  long get_skip_as_msec(SkipType skip) {
    if (skip == FRAME) {
      int numerator = 0, denominator = 0;
      if (player()->get_framerate(&numerator, &denominator) > 0)
        return denominator * 1000 / numerator;
    } else if (skip == TINY) {
      return cfg::get_int("video-player", "skip-tiny");
    } else if (skip == VERY_SHORT) {
      return cfg::get_int("video-player", "skip-very-short") * 1000;
    } else if (skip == SHORT) {
      return cfg::get_int("video-player", "skip-short") * 1000;
    } else if (skip == MEDIUM) {
      return cfg::get_int("video-player", "skip-medium") * 1000;
    } else if (skip == LONG) {
      return cfg::get_int("video-player", "skip-long") * 1000;
    }
    return 0;
  }

  // Make a skip backwards depending on the type.
  void on_skip_backwards(SkipType skip) {
    long newpos = player()->get_position() - get_skip_as_msec(skip);

    player()->seek(newpos);
  }

  // make a skip forward depending on the type.
  void on_skip_forward(SkipType skip) {
    long newpos = player()->get_position() + get_skip_as_msec(skip);

    player()->seek(newpos);
  }

  // Increase the playback rate.
  void on_playback_rate_faster() {
    double rate = player()->get_playback_rate();

    rate += 0.1;

    player()->set_playback_rate(rate);
  }

  // Decreases the playback rate.
  void on_playback_rate_slower() {
    double rate = player()->get_playback_rate();

    rate -= 0.1;

    player()->set_playback_rate(rate);
  }

  // Sets the playback rate to 1.0 (default).
  void on_playback_rate_normal() {
    player()->set_playback_rate(1.0);
  }

  // Seek to start of the first selected subtitle.
  // The state of the player isn't modified.
  void on_seek_to_selection() {
    Document *doc = get_current_document();

    Subtitle selected = doc->subtitles().get_first_selected();
    if (selected) {
      player()->seek(selected.get_start().totalmsecs);
    }
  }

  // Seek to the last selected subtitle.
  // The state of the player isn't modified.
  void on_seek_to_selection_end() {
    Document *doc = get_current_document();

    Subtitle selected = doc->subtitles().get_last_selected();
    if (selected) {
      player()->seek(selected.get_end().totalmsecs);
    }
  }

  // Method for playing subtitle.

  // Select and play the previous subtitle.
  // Repeat is supported.
  void on_play_previous_subtitle() {
    Document *doc = get_current_document();

    Subtitle selected = doc->subtitles().get_first_selected();
    if (selected) {
      Subtitle previous = doc->subtitles().get_previous(selected);
      if (previous) {
        doc->subtitles().select(previous);
        player()->play_subtitle(previous);
      }
    }
  }

  // Play the current subtitle.
  // Repeat is supported.
  void on_play_current_subtitle() {
    Document *doc = get_current_document();

    Subtitle selected = doc->subtitles().get_first_selected();
    if (selected) {
      player()->play_subtitle(selected);
    }
  }

  // Select and play the next subtitle.
  // Repeat is supported.
  void on_play_next_subtitle() {
    Document *doc = get_current_document();

    Subtitle selected = doc->subtitles().get_first_selected();
    if (selected) {
      Subtitle next = doc->subtitles().get_next(selected);
      if (next) {
        doc->subtitles().select(next);
        player()->play_subtitle(next);
      }
    }
  }

  // Method for playing second.

  // Play the second preceding the first selected subtitle.
  void on_play_previous_second() {
    Document *doc = get_current_document();

    Subtitle selected = doc->subtitles().get_first_selected();
    if (selected) {
      SubtitleTime start = selected.get_start() - SubtitleTime(0, 0, 1, 0);
      SubtitleTime end = selected.get_start();

      player()->play_segment(start, end);
    }
  }

  // Play the first second of the subtitle currently selected.
  void on_play_first_second() {
    Document *doc = get_current_document();

    Subtitle selected = doc->subtitles().get_first_selected();
    if (selected) {
      SubtitleTime start = selected.get_start();
      SubtitleTime end = selected.get_start() + SubtitleTime(0, 0, 1, 0);

      player()->play_segment(start, end);
    }
  }

  // Play the last second of the subtitle currently selected
  void on_play_last_second() {
    Document *doc = get_current_document();

    Subtitle selected = doc->subtitles().get_first_selected();
    if (selected) {
      SubtitleTime start = selected.get_end() - SubtitleTime(0, 0, 1, 0);
      SubtitleTime end = selected.get_end();

      player()->play_segment(start, end);
    }
  }

  // Play the second following the subtitle currently selected
  void on_play_next_second() {
    Document *doc = get_current_document();

    Subtitle selected = doc->subtitles().get_first_selected();
    if (selected) {
      SubtitleTime start = selected.get_end();
      SubtitleTime end = selected.get_end() + SubtitleTime(0, 0, 1, 0);

      player()->play_segment(start, end);
    }
  }

  void add_in_recent_manager(const Glib::ustring &uri) {
    se_dbg_msg(SE_DBG_PLUGINS, "uri=%s", uri.c_str());

    Gtk::RecentManager::Data data;
    // data.mime_type = "subtitle/";
    data.app_name = Glib::get_application_name();
    data.app_exec = Glib::get_prgname();
    data.groups.push_back("subtitleeditor-video-player");
    data.is_private = false;
    Gtk::RecentManager::get_default()->add_item(uri, data);
  }

  // Open a recent video
  void on_recent_item_activated() {
    se_dbg(SE_DBG_PLUGINS);

    Glib::RefPtr<Gtk::Action> action =
        action_group->get_action("video-player/recent-files");

    Glib::RefPtr<Gtk::RecentAction> recentAction =
        Glib::RefPtr<Gtk::RecentAction>::cast_static(action);

    Glib::RefPtr<Gtk::RecentInfo> cur = recentAction->get_current_item();
    if (cur) {
      se_dbg_msg(SE_DBG_PLUGINS, "uri=%s", cur->get_uri().c_str());

      player()->open(cur->get_uri());
    }
  }

 protected:
  Gtk::UIManager::ui_merge_id ui_id;
  Gtk::UIManager::ui_merge_id ui_id_audio;
  Glib::RefPtr<Gtk::ActionGroup> action_group;
  Glib::RefPtr<Gtk::ActionGroup> action_group_audio;
};

REGISTER_EXTENSION(VideoPlayerManagement)
