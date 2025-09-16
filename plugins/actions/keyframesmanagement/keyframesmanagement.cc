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

#include <extension/action.h>
#include <gui/dialogfilechooser.h>
#include <keyframes.h>
#include <player.h>
#include <utility.h>

// declared in keyframesgenerator.cc
Glib::RefPtr<KeyFrames> generate_keyframes_from_file(const Glib::ustring &uri);
Glib::RefPtr<KeyFrames> generate_keyframes_from_file_using_frame(
    const Glib::ustring &uri);

class KeyframesManagementPlugin : public Action {
 public:
  KeyframesManagementPlugin() {
    activate();
    update_ui();
  }

  ~KeyframesManagementPlugin() {
    deactivate();
  }

  void activate() {
    se_dbg(SE_DBG_PLUGINS);

    // actions
    action_group = Gtk::ActionGroup::create("KeyframesManagementPlugin");

    // Open
    action_group->add(
        Gtk::Action::create("keyframes/open", Gtk::Stock::OPEN,
                            _("Open Keyframes"),
                            _("Open keyframes from a file")),
        Gtk::AccelKey("<Control>K"),
        sigc::mem_fun(*this, &KeyframesManagementPlugin::on_open));
    // Save
    action_group->add(
        Gtk::Action::create("keyframes/save", Gtk::Stock::SAVE,
                            _("Save Keyframes"),
                            _("Save keyframes to the file")),
        Gtk::AccelKey("<Shift><Control>K"),
        sigc::mem_fun(*this, &KeyframesManagementPlugin::on_save));
    // Generate
    action_group->add(
        Gtk::Action::create("keyframes/generate", Gtk::Stock::EXECUTE,
                            _("Generate Keyframes From Video"),
                            _("Generate keyframes from the current video")),
        sigc::mem_fun(*this, &KeyframesManagementPlugin::on_generate));
    action_group->add(
        Gtk::Action::create("keyframes/generate-using-frame",
                            Gtk::Stock::EXECUTE,
                            _("Generate Keyframes From Video (Using Frame)"),
                            _("Generate keyframes from the current video")),
        sigc::mem_fun(*this,
                      &KeyframesManagementPlugin::on_generate_using_frame));
    // Close
    action_group->add(
        Gtk::Action::create("keyframes/close", Gtk::Stock::CLOSE,
                            _("Close the keyframes"), _("FIXME")),
        Gtk::AccelKey("<Alt><Control>K"),
        sigc::mem_fun(*this, &KeyframesManagementPlugin::on_close));
    // Seek
    action_group->add(
        Gtk::Action::create("keyframes/seek-to-previous",
                            Gtk::Stock::MEDIA_PREVIOUS,
                            _("Seek To Previous Keyframe"), _("FIXME")),
        sigc::mem_fun(*this, &KeyframesManagementPlugin::on_seek_previous));

    action_group->add(
        Gtk::Action::create("keyframes/seek-to-next", Gtk::Stock::MEDIA_NEXT,
                            _("Seek To Next Keyframe"), _("FIXME")),
        sigc::mem_fun(*this, &KeyframesManagementPlugin::on_seek_next));
    // Snap Start
    action_group->add(
        Gtk::Action::create("keyframes/snap-start-to-previous",
                            Gtk::Stock::GOTO_FIRST,
                            _("Snap Start To Previous Keyframe"), _("FIXME")),
        sigc::mem_fun(*this,
                      &KeyframesManagementPlugin::on_snap_start_to_previous));

    action_group->add(
        Gtk::Action::create("keyframes/snap-start-to-next",
                            Gtk::Stock::GOTO_LAST,
                            _("Snap Start To Next Keyframe"), _("FIXME")),
        sigc::mem_fun(*this,
                      &KeyframesManagementPlugin::on_snap_start_to_next));
    // Snap End
    action_group->add(
        Gtk::Action::create("keyframes/snap-end-to-previous",
                            Gtk::Stock::GOTO_FIRST,
                            _("Snap End To Previous Keyframe"), _("FIXME")),
        sigc::mem_fun(*this,
                      &KeyframesManagementPlugin::on_snap_end_to_previous));

    action_group->add(
        Gtk::Action::create("keyframes/snap-end-to-next", Gtk::Stock::GOTO_LAST,
                            _("Snap End To Next Keyframe"), _("FIXME")),
        sigc::mem_fun(*this, &KeyframesManagementPlugin::on_snap_end_to_next));

    // Recent files
    Glib::RefPtr<Gtk::RecentAction> recentAction =
        Gtk::RecentAction::create("keyframes/recent-files", _("_Recent Files"));

    Glib::RefPtr<Gtk::RecentFilter> filter = Gtk::RecentFilter::create();
    filter->set_name("subtitleeditor");
    filter->add_group("subtitleeditor-keyframes");
    recentAction->set_filter(filter);
    recentAction->set_show_icons(false);
    recentAction->set_show_numbers(true);
    recentAction->set_show_tips(true);
    // recentAction->set_show_not_found(false);
    recentAction->set_sort_type(Gtk::RECENT_SORT_MRU);
    recentAction->signal_item_activated().connect(sigc::mem_fun(
        *this, &KeyframesManagementPlugin::on_recent_item_activated));
    action_group->add(recentAction);

    // ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui_id = ui->new_merge_id();

    ui->insert_action_group(action_group);

    Glib::ustring submenu = R"(
      <ui>
        <menubar name='menubar'>
          <menu name='menu-keyframes' action='menu-keyframes'>
            <placeholder name='placeholder'>
              <menuitem action='keyframes/open'/>
              <menuitem action='keyframes/recent-files'/>
              <menuitem action='keyframes/save'/>
              <menuitem action='keyframes/generate'/>
              <menuitem action='keyframes/generate-using-frame'/>
              <menuitem action='keyframes/close'/>
              <separator/>
              <menuitem action='keyframes/seek-to-previous'/>
              <menuitem action='keyframes/seek-to-next'/>
              <separator/>
              <menuitem action='keyframes/snap-start-to-previous'/>
              <menuitem action='keyframes/snap-start-to-next'/>
              <menuitem action='keyframes/snap-end-to-previous'/>
              <menuitem action='keyframes/snap-end-to-next'/>
            </placeholder>
          </menu>
        </menubar>
      </ui>
    )";

    ui_id = ui->add_ui_from_string(submenu);

    // connect the player state signals
    player()->signal_message().connect(
        sigc::mem_fun(*this, &KeyframesManagementPlugin::on_player_message));
  }

  void deactivate() {
    se_dbg(SE_DBG_PLUGINS);

    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui->remove_ui(ui_id);
    ui->remove_action_group(action_group);
  }

  void on_player_message(Player::Message msg) {
    // only if the player is enable or disable
    // don't update if is playing or paused
    if (msg == Player::STREAM_READY || msg == Player::STATE_NONE)
      update_ui();
    else if (msg == Player::KEYFRAME_CHANGED)
      on_keyframes_changed();
  }

  void on_keyframes_changed() {
	se_dbg(SE_DBG_PLUGINS);
    Glib::RefPtr<KeyFrames> kf = player()->get_keyframes();
    Glib::ustring uri;
	if (kf){
      uri = kf->get_uri();
      // When waveform is first created, there is a wf object and the condition 
      // evaluates to true, but the file is not yet saved, so the uri is empty.
      // In such a case, do not add to recent files, as it added an empty uri, making it unsuable
      if (!uri.empty())
        add_in_recent_manager(uri);
    }
    update_ui();
  }
  void update_ui() {
    se_dbg(SE_DBG_PLUGINS);

    bool has_doc = (get_current_document() != NULL);
    bool has_kf = static_cast<bool>(player()->get_keyframes());
    bool has_media = player()->get_state() != Player::NONE;

#define SET_SENSITIVE(action, state)                                  \
  {                                                                   \
    Glib::RefPtr<Gtk::Action> act = action_group->get_action(action); \
    if (act)                                                          \
      act->set_sensitive(state);                                      \
    else                                                              \
      g_warning(action);                                              \
  }

    SET_SENSITIVE("keyframes/save", has_kf);
    SET_SENSITIVE("keyframes/close", has_kf);
    SET_SENSITIVE("keyframes/generate", has_media);
    SET_SENSITIVE("keyframes/generate-using-frame", has_media);
    // Update state from keyframes and player
    SET_SENSITIVE("keyframes/seek-to-previous", has_kf && has_media);
    SET_SENSITIVE("keyframes/seek-to-next", has_kf && has_media);
    // Update state from document and keyframes
    SET_SENSITIVE("keyframes/snap-start-to-previous", has_doc && has_kf);
    SET_SENSITIVE("keyframes/snap-start-to-next", has_doc && has_kf);
    SET_SENSITIVE("keyframes/snap-end-to-previous", has_doc && has_kf);
    SET_SENSITIVE("keyframes/snap-end-to-next", has_doc && has_kf);

#undef SET_SENSITIVE
  }

 protected:
  Player *player() {
    return get_subtitleeditor_window()->get_player();
  }

  void on_open() {
    DialogOpenKeyframe ui;
    if (ui.run() == Gtk::RESPONSE_OK) {
      ui.hide();
      Glib::RefPtr<KeyFrames> kf = KeyFrames::create_from_file(ui.get_uri());
      if (!kf)
        // FIXME: until old code is FIXED, use by default the frame method
        kf = generate_keyframes_from_file_using_frame(ui.get_uri());

      if (kf) {
        player()->set_keyframes(kf);
        add_in_recent_manager(kf->get_uri());
      }
    }
  }

  void on_save() {
    Glib::RefPtr<KeyFrames> kf = player()->get_keyframes();
    if (kf) {
      Gtk::FileChooserDialog ui(_("Save Keyframes"),
                                Gtk::FILE_CHOOSER_ACTION_SAVE);
      ui.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
      ui.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
      ui.set_default_response(Gtk::RESPONSE_OK);

      set_default_filename_from_video(&ui, kf->get_video_uri(), "kf");

      if (ui.run() == Gtk::RESPONSE_OK) {
        Glib::ustring uri = ui.get_uri();

        // FIXME check return value
        kf->save(uri);
        add_in_recent_manager(kf->get_uri());
      }
    }
  }

  void add_in_recent_manager(const Glib::ustring &uri) {
    se_dbg_msg(SE_DBG_PLUGINS, "uri=%s", uri.c_str());

    Gtk::RecentManager::Data data;
    data.app_name = Glib::get_application_name();
    data.app_exec = Glib::get_prgname();
    data.groups.push_back("subtitleeditor-keyframes");
    data.is_private = false;
    Gtk::RecentManager::get_default()->add_item(uri, data);
  }

  // Open a recent keyframes
  void on_recent_item_activated() {
    se_dbg(SE_DBG_PLUGINS);

    Glib::RefPtr<Gtk::Action> action =
        action_group->get_action("keyframes/recent-files");

    Glib::RefPtr<Gtk::RecentAction> recentAction =
        Glib::RefPtr<Gtk::RecentAction>::cast_static(action);

    Glib::RefPtr<Gtk::RecentInfo> cur = recentAction->get_current_item();
    if (cur) {
      se_dbg_msg(SE_DBG_PLUGINS, "uri=%s", cur->get_uri().c_str());

      Glib::RefPtr<KeyFrames> kf = KeyFrames::create_from_file(cur->get_uri());
      if (kf)
        player()->set_keyframes(kf);
    }
  }

  void set_default_filename_from_video(Gtk::FileChooser *fc,
                                       const Glib::ustring &video_uri,
                                       const Glib::ustring &ext) {
    try {
      Glib::ustring videofn = Glib::filename_from_uri(video_uri);
      Glib::ustring pathname = Glib::path_get_dirname(videofn);
      Glib::ustring basename = Glib::path_get_basename(videofn);

      Glib::RefPtr<Glib::Regex> re = Glib::Regex::create("^(.*)(\\.)(.*)$");
      if (re->match(basename))
        basename =
            re->replace(basename, 0, "\\1." + ext, Glib::RegexMatchFlags(0));
      else
        basename = Glib::ustring::compose("%1.%2", basename, ext);

      fc->set_current_folder(pathname);  // set_current_folder_uri ?
      fc->set_current_name(basename);
    } catch (const Glib::Exception &ex) {
      std::cerr << "set_default_filename_from_video failed : " << ex.what()
                << std::endl;
    }
  }

  void on_generate() {
    Glib::ustring uri = get_subtitleeditor_window()->get_player()->get_uri();
    if (uri.empty())
      return;

    Glib::RefPtr<KeyFrames> kf = generate_keyframes_from_file(uri);
    if (kf) {
      player()->set_keyframes(kf);
      on_save();
    }
  }

  void on_generate_using_frame() {
    Glib::ustring uri = get_subtitleeditor_window()->get_player()->get_uri();
    if (uri.empty())
      return;

    Glib::RefPtr<KeyFrames> kf = generate_keyframes_from_file_using_frame(uri);
    if (kf) {
      player()->set_keyframes(kf);
      on_save();
    }
  }

  void on_close() {
    player()->set_keyframes(Glib::RefPtr<KeyFrames>(NULL));
  }

  void on_seek_next() {
    Glib::RefPtr<KeyFrames> keyframes = player()->get_keyframes();
    g_return_if_fail(keyframes);

    long pos = player()->get_position();

    for (auto it = keyframes->begin(); it != keyframes->end(); ++it) {
      if (*it > pos) {
        player()->seek(*it);
        return;
      }
    }
  }

  void on_seek_previous() {
    Glib::RefPtr<KeyFrames> keyframes = player()->get_keyframes();
    g_return_if_fail(keyframes);

    long pos = player()->get_position();

    for (auto it = keyframes->rbegin(); it != keyframes->rend(); ++it) {
      if (*it < pos) {
        player()->seek(*it);
        return;
      }
    }
  }

  bool get_previous_keyframe(const long pos, long &prev) {
    Glib::RefPtr<KeyFrames> keyframes = player()->get_keyframes();
    if (!keyframes)
      return false;

    for (auto it = keyframes->rbegin(); it != keyframes->rend(); ++it) {
      if (*it < pos) {
        prev = *it;
        return true;
      }
    }
    return false;
  }

  bool get_next_keyframe(const long pos, long &next) {
    Glib::RefPtr<KeyFrames> keyframes = player()->get_keyframes();
    if (!keyframes)
      return false;

    for (auto it = keyframes->begin(); it != keyframes->end(); ++it) {
      if (*it > pos) {
        next = *it;
        return true;
      }
    }
    return false;
  }

  bool snap_start_to_keyframe(bool previous) {
    Document *doc = get_current_document();
    g_return_val_if_fail(doc, false);

    Subtitle sub = doc->subtitles().get_first_selected();
    g_return_val_if_fail(sub, false);

    long pos = sub.get_start().totalmsecs;
    long kf = 0;
    bool ret = (previous) ? get_previous_keyframe(pos, kf)
                          : get_next_keyframe(pos, kf);
    if (!ret)
      return false;

    doc->start_command(_("Snap Start to Keyframe"));
    sub.set_start(SubtitleTime(kf));
    doc->emit_signal("subtitle-time-changed");
    doc->finish_command();
    return true;
  }

  bool snap_end_to_keyframe(bool previous) {
    Document *doc = get_current_document();
    g_return_val_if_fail(doc, false);

    Subtitle sub = doc->subtitles().get_first_selected();
    g_return_val_if_fail(sub, false);

    long pos = sub.get_end().totalmsecs;
    long kf = 0;
    bool ret = (previous) ? get_previous_keyframe(pos, kf)
                          : get_next_keyframe(pos, kf);
    if (!ret)
      return false;
    doc->start_command(_("Snap End to Keyframe"));
    sub.set_end(SubtitleTime(kf));
    doc->emit_signal("subtitle-time-changed");
    doc->finish_command();
    return true;
  }

  void on_snap_start_to_previous() {
    snap_start_to_keyframe(true);
  }

  void on_snap_start_to_next() {
    snap_start_to_keyframe(false);
  }

  void on_snap_end_to_previous() {
    snap_end_to_keyframe(true);
  }

  void on_snap_end_to_next() {
    snap_end_to_keyframe(false);
  }

 protected:
  Gtk::UIManager::ui_merge_id ui_id;
  Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(KeyframesManagementPlugin)
