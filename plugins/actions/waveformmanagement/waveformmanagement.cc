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
#include <gtkmm.h>
#include <gui/dialogfilechooser.h>
#include <player.h>
#include <utility.h>
#include <waveformmanager.h>

// Declared in waveformgenerator.cc
Glib::RefPtr<Waveform> generate_waveform_from_file(const Glib::ustring& uri);

class WaveformManagement : public Action {
 public:
  WaveformManagement() {
    activate();
    update_ui();
    update_ui_from_player(Player::STATE_NONE);
  }

  ~WaveformManagement() {
    deactivate();
  }

  void activate() {
    se_dbg(SE_DBG_PLUGINS);

    // actions
    action_group = Gtk::ActionGroup::create("WaveformManagement");

    // Already create in MenuBar.cc
    action_group->add(Gtk::Action::create("menu-waveform", _("_Waveform")));

    // open & save
    action_group->add(
        Gtk::Action::create(
            "waveform/open", Gtk::Stock::OPEN, _("_Open Waveform From File"),
            _("Open waveform from a file or create from a video")),
        Gtk::AccelKey("<Control><Alt>O"),
        sigc::mem_fun(*this, &WaveformManagement::on_open_waveform));

    action_group->add(
        Gtk::Action::create(
            "waveform/generate-from-player-file",
            _("_Generate Waveform From Video"),
            _("Generate the waveform from the current video file")),
        sigc::mem_fun(*this,
                      &WaveformManagement::on_generate_from_player_file));

    action_group->add(
        Gtk::Action::create("waveform/generate-dummy",
                            _("_Generate Dummy Waveform"),
                            _("Generate an dummy waveform (sine)")),
        sigc::mem_fun(*this, &WaveformManagement::on_generate_dummy));

    action_group->add(
        Gtk::Action::create("waveform/save", Gtk::Stock::SAVE,
                            _("_Save Waveform"), _("Save waveform to file")),
        Gtk::AccelKey("<Control><Alt>S"),
        sigc::mem_fun(*this, &WaveformManagement::on_save_waveform));

    action_group->add(
        Gtk::Action::create("waveform/close", Gtk::Stock::CLOSE,
                            _("_Close Waveform"), _("Close waveform")),
        sigc::mem_fun(*this, &WaveformManagement::on_close_waveform));

    // zoom
    action_group->add(
        Gtk::Action::create("waveform/zoom-in", Gtk::Stock::ZOOM_IN,
                            _("Zoom _In"), _("FIXME")),
        sigc::mem_fun(*this, &WaveformManagement::on_zoom_in));

    action_group->add(
        Gtk::Action::create("waveform/zoom-out", Gtk::Stock::ZOOM_OUT,
                            _("Zoom _Out"), _("FIXME")),
        sigc::mem_fun(*this, &WaveformManagement::on_zoom_out));

    action_group->add(
        Gtk::Action::create("waveform/zoom-selection", Gtk::Stock::ZOOM_FIT,
                            _("Zoom _Selection"), _("FIXME")),
        sigc::mem_fun(*this, &WaveformManagement::on_zoom_selection));

    action_group->add(
        Gtk::Action::create("waveform/zoom-all", Gtk::Stock::ZOOM_100,
                            _("Zoom _All"), _("FIXME")),
        sigc::mem_fun(*this, &WaveformManagement::on_zoom_all));

    // center
    action_group->add(
        Gtk::Action::create("waveform/center-with-selected-subtitle",
                            _("_Center With Selected Subtitle"), _("FIXME")),
        sigc::mem_fun(*this,
                      &WaveformManagement::on_center_with_selected_subtitle));

    // scrolling with player
    bool scroll_with_player_state =
        cfg::get_boolean("waveform", "scrolling-with-player");

    action_group->add(
        Gtk::ToggleAction::create("waveform/scrolling-with-player",
                                  _("Scrolling With _Player"), _("FIXME"),
                                  scroll_with_player_state),
        sigc::mem_fun(*this, &WaveformManagement::on_scrolling_with_player));

    // select with player
	// FIXME: Properly speaking, this should probably be in the video menu
	// for it should work without a waveform
    bool select_with_player_state =
        cfg::get_boolean("waveform", "select-with-player");

    action_group->add(
        Gtk::ToggleAction::create("waveform/select-with-player",
                                  _("Select With _Player"), _("FIXME"),
                                  select_with_player_state),
        sigc::mem_fun(*this, &WaveformManagement::on_select_with_player));

    // scrolling with selection
    bool scroll_with_selection_state =
        cfg::get_boolean("waveform", "scrolling-with-selection");

    action_group->add(
        Gtk::ToggleAction::create("waveform/scrolling-with-selection",
                                  _("Scrolling With _Selection"), _("FIXME"),
                                  scroll_with_selection_state),
        sigc::mem_fun(*this, &WaveformManagement::on_scrolling_with_selection));

    // Respect the timing
    bool respect_timing_state = cfg::get_boolean("waveform", "respect-timing");

    action_group->add(
        Gtk::ToggleAction::create(
            "waveform/respect-timing", _("_Respect The Timing"),
            _("Try to respect the timing preferences"), respect_timing_state),
        sigc::mem_fun(*this, &WaveformManagement::on_respect_timing));

    // Waveform Display
    bool waveform_display_state = cfg::get_boolean("waveform", "display");

    action_group->add(
        Gtk::ToggleAction::create(
            "waveform/display", _("_Waveform"),
            _("Show or hide the waveform in the current window"),
            waveform_display_state),
        sigc::mem_fun(*this, &WaveformManagement::on_waveform_display));

    // Recent files
    Glib::RefPtr<Gtk::RecentAction> recentAction =
        Gtk::RecentAction::create("waveform/recent-files", _("_Recent Files"));

    Glib::RefPtr<Gtk::RecentFilter> filter = Gtk::RecentFilter::create();
    filter->set_name("subtitleeditor");
    filter->add_group("subtitleeditor-waveform");
    recentAction->set_filter(filter);
    recentAction->set_show_icons(false);
    recentAction->set_show_numbers(true);
    recentAction->set_show_tips(true);
    // recentAction->set_show_not_found(false);
    recentAction->set_sort_type(Gtk::RECENT_SORT_MRU);

    recentAction->signal_item_activated().connect(
        sigc::mem_fun(*this, &WaveformManagement::on_recent_item_activated));
    action_group->add(recentAction);

    // ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui->insert_action_group(action_group);

    Glib::ustring submenu = R"(
      <ui>
        <menubar name='menubar'>
          <menu name='menu-waveform' action='menu-waveform'>
            <placeholder name='waveform-management'>
              <menuitem action='waveform/open'/>
              <menuitem action='waveform/recent-files'/>
              <menuitem action='waveform/generate-from-player-file'/>
              <menuitem action='waveform/generate-dummy'/>
              <menuitem action='waveform/save'/>
              <menuitem action='waveform/close'/>
              <separator/>
              <menuitem action='waveform/zoom-in'/>
              <menuitem action='waveform/zoom-out'/>
              <menuitem action='waveform/zoom-selection'/>
              <menuitem action='waveform/zoom-all'/>
              <separator/>
              <menuitem action='waveform/center-with-selected-subtitle'/>
              <separator/>
              <menuitem action='waveform/scrolling-with-player'/>
              <menuitem action='waveform/select-with-player'/>
              <menuitem action='waveform/scrolling-with-selection'/>
              <menuitem action='waveform/respect-timing'/>
            </placeholder>
          </menu>
        </menubar>
      </ui>
    )";

    ui_id = ui->add_ui_from_string(submenu);

    // Show/Hide Waveform Editor
    ui->add_ui(ui_id, "/menubar/menu-view/display-placeholder",
               "waveform/display", "waveform/display");

    // HACK
    WaveformManager* wm = get_waveform_manager();

    wm->signal_waveform_changed().connect(
        sigc::mem_fun(*this, &WaveformManagement::on_waveform_changed));

    cfg::signal_changed("waveform")
        .connect(sigc::mem_fun(
            *this, &WaveformManagement::on_config_waveform_changed));

    // Player state
    get_subtitleeditor_window()->get_player()->signal_message().connect(
        sigc::mem_fun(*this, &WaveformManagement::update_ui_from_player));
  }

  void deactivate() {
    se_dbg(SE_DBG_PLUGINS);

    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui->remove_ui(ui_id);
    ui->remove_action_group(action_group);
  }

  void update_ui() {
    se_dbg(SE_DBG_PLUGINS);

    bool has_waveform = get_waveform_manager()->has_waveform();

    bool has_document = (get_current_document() != NULL);

    action_group->get_action("waveform/save")->set_sensitive(has_waveform);
    action_group->get_action("waveform/close")->set_sensitive(has_waveform);
    action_group->get_action("waveform/zoom-in")->set_sensitive(has_waveform);
    action_group->get_action("waveform/zoom-out")->set_sensitive(has_waveform);
    action_group->get_action("waveform/zoom-selection")
        ->set_sensitive(has_waveform);
    action_group->get_action("waveform/zoom-all")->set_sensitive(has_waveform);

    action_group->get_action("waveform/scrolling-with-player")
        ->set_sensitive(has_waveform);
    action_group->get_action("waveform/select-with-player")
        ->set_sensitive(has_document);
    action_group->get_action("waveform/scrolling-with-selection")
        ->set_sensitive(has_waveform);
    action_group->get_action("waveform/respect-timing")
        ->set_sensitive(has_waveform);

    action_group->get_action("waveform/center-with-selected-subtitle")
        ->set_sensitive(has_waveform && has_document);
  }

  void on_waveform_changed() {
    se_dbg(SE_DBG_PLUGINS);
    Glib::RefPtr<Waveform> wf = get_waveform_manager()->get_waveform();
    Glib::ustring uri;
    if (wf){
      uri = wf->get_uri();
	  // When waveform is first created, there is a wf object and the condition 
	  // evaluates to true, but the file is not yet saved, so the uri is empty.
	  // In such a case, do not add to recent files, as it added an empty uri, making it unsuable
	  if (!uri.empty())
		add_in_recent_manager(uri);
	}	
    update_ui();
  }

  // Update the ui state from the player state.
  void update_ui_from_player(Player::Message msg) {
    switch (msg) {
      case Player::STATE_NONE:
      case Player::STREAM_READY: {
        Player* player = get_subtitleeditor_window()->get_player();
        bool has_player_file = (player->get_state() != Player::NONE);
        action_group->get_action("waveform/generate-from-player-file")
            ->set_sensitive(has_player_file);
        action_group->get_action("waveform/generate-dummy")
            ->set_sensitive(has_player_file);
      } break;
      default:
        break;
    }
  }

 protected:
  WaveformManager* get_waveform_manager() {
    return get_subtitleeditor_window()->get_waveform_manager();
  }

  // Launch the Dialog Open Waveform
  // and try to open the Waveform.
  // If is not a Waveform file launch the
  // Waveform generator.
  void on_open_waveform() {
    se_dbg(SE_DBG_PLUGINS);

    DialogOpenWaveform dialog;
    if (dialog.run() == Gtk::RESPONSE_OK) {
      dialog.hide();

      Glib::ustring uri = dialog.get_uri();
      Glib::RefPtr<Waveform> wf = Waveform::create_from_file(uri);
      if (wf) {
        get_waveform_manager()->set_waveform(wf);
        add_in_recent_manager(wf->get_uri());
        update_player_from_waveform();
      } else {
        wf = generate_waveform_from_file(uri);
        if (wf) {
          get_waveform_manager()->set_waveform(wf);
          on_save_waveform();
          update_player_from_waveform();
        }
      }
    }
  }

  // Generate a waveform from the current file in the player.
  void on_generate_from_player_file() {
    Glib::ustring uri = get_subtitleeditor_window()->get_player()->get_uri();
    if (uri.empty() == false) {
      // get_waveform_manager()->generate_waveform(uri);
      Glib::RefPtr<Waveform> wf = generate_waveform_from_file(uri);
      if (wf) {
        get_waveform_manager()->set_waveform(wf);
        on_save_waveform();
      }
    }
  }

  // Generate an Sine Waveform
  void on_generate_dummy() {
    Player* player = get_subtitleeditor_window()->get_player();
    if (player->get_state() == Player::NONE)
      return;

    // Create and initialize Waveform
    Glib::RefPtr<Waveform> wf(new Waveform);
    wf->m_video_uri = player->get_uri();
    wf->m_n_channels = 1;
    wf->m_duration = player->get_duration();

    // Create Sine Waveform
    int second = SubtitleTime(0, 0, 1, 0).totalmsecs;
    wf->m_channels[0].resize(wf->m_duration);

    double freq = (wf->m_duration % second) / 2;
    double amp = 0.5;
    double rate = SubtitleTime(0, 1, 0, 0).totalmsecs;
    double rfreq = 2.0 * 3.141592653589793 * freq;

    for (unsigned int i = 1; i <= wf->m_duration; ++i) {
      double a = amp - (amp * (i % second) * 0.001);
      wf->m_channels[0][i - 1] = a * sin(rfreq * (i / rate));
    }

    get_waveform_manager()->set_waveform(wf);
  }

  void on_save_waveform() {
    se_dbg(SE_DBG_PLUGINS);

    Glib::RefPtr<Waveform> wf = get_waveform_manager()->get_waveform();
    if (wf) {
      DialogFileChooser ui(_("Save Waveform"), Gtk::FILE_CHOOSER_ACTION_SAVE,
                           "dialog-save-waveform");
      ui.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
      ui.add_button(Gtk::Stock::OK, Gtk::RESPONSE_OK);
      ui.set_default_response(Gtk::RESPONSE_OK);
      ui.set_filename_from_another_uri(wf->get_video_uri(), "wf");

      if (ui.run() == Gtk::RESPONSE_OK) {
        Glib::ustring uri = ui.get_uri();

        wf->save(uri);
        add_in_recent_manager(uri);
      }
    }
  }

  void on_close_waveform() {
    se_dbg(SE_DBG_PLUGINS);

    Glib::RefPtr<Waveform> wf(NULL);

    get_waveform_manager()->set_waveform(wf);
  }

  // Update the video player with the new Waveform
  // only if it's different.
  void update_player_from_waveform() {
    Glib::RefPtr<Waveform> wf = get_waveform_manager()->get_waveform();
    if (wf && get_subtitleeditor_window()->get_player()->get_uri() !=
                  wf->m_video_uri) {
      get_subtitleeditor_window()->get_player()->open(wf->m_video_uri);
    }
  }

  void on_center_with_selected_subtitle() {
    se_dbg(SE_DBG_PLUGINS);

    get_waveform_manager()->center_with_selected_subtitle();
  }

  void on_zoom_in() {
    se_dbg(SE_DBG_PLUGINS);

    get_waveform_manager()->zoom_in();
  }

  void on_zoom_out() {
    se_dbg(SE_DBG_PLUGINS);

    get_waveform_manager()->zoom_out();
  }

  void on_zoom_selection() {
    se_dbg(SE_DBG_PLUGINS);

    get_waveform_manager()->zoom_selection();
  }

  void on_zoom_all() {
    se_dbg(SE_DBG_PLUGINS);

    get_waveform_manager()->zoom_all();
  }

  void on_scrolling_with_player() {
    se_dbg(SE_DBG_PLUGINS);

    Glib::RefPtr<Gtk::ToggleAction> action =
        Glib::RefPtr<Gtk::ToggleAction>::cast_static(
            action_group->get_action("waveform/scrolling-with-player"));
    if (action) {
      bool state = action->get_active();
      cfg::set_boolean("waveform", "scrolling-with-player", state);
    }
  }

  void on_select_with_player() {
    se_dbg(SE_DBG_PLUGINS);

    Glib::RefPtr<Gtk::ToggleAction> action =
        Glib::RefPtr<Gtk::ToggleAction>::cast_static(
            action_group->get_action("waveform/select-with-player"));
    if (action) {
      bool state = action->get_active();
      cfg::set_boolean("waveform", "select-with-player", state);
    }
  }

  void on_scrolling_with_selection() {
    se_dbg(SE_DBG_PLUGINS);

    Glib::RefPtr<Gtk::ToggleAction> action =
        Glib::RefPtr<Gtk::ToggleAction>::cast_static(
            action_group->get_action("waveform/scrolling-with-selection"));
    if (action) {
      bool state = action->get_active();
      cfg::set_boolean("waveform", "scrolling-with-selection", state);
    }
  }

  void on_respect_timing() {
    se_dbg(SE_DBG_PLUGINS);

    Glib::RefPtr<Gtk::ToggleAction> action =
        Glib::RefPtr<Gtk::ToggleAction>::cast_static(
            action_group->get_action("waveform/respect-timing"));
    if (action) {
      bool state = action->get_active();
      cfg::set_boolean("waveform", "respect-timing", state);
    }
  }

  void on_waveform_display() {
    se_dbg(SE_DBG_PLUGINS);

    Glib::RefPtr<Gtk::ToggleAction> action =
        Glib::RefPtr<Gtk::ToggleAction>::cast_static(
            action_group->get_action("waveform/display"));
    if (action) {
      bool state = action->get_active();
      if (cfg::get_boolean("waveform", "display") != state) {
        cfg::set_boolean("waveform", "display", state);
      }
    }
  }

  void on_config_waveform_changed(const Glib::ustring& key,
                                  const Glib::ustring& value) {
    if (key == "display") {
      bool state = utility::string_to_bool(value);

      Glib::RefPtr<Gtk::ToggleAction> action =
          Glib::RefPtr<Gtk::ToggleAction>::cast_static(
              action_group->get_action("waveform/display"));
      if (action) {
        if (action->get_active() != state)
          action->set_active(state);
      }
    }
  }

  void add_in_recent_manager(const Glib::ustring& uri) {
    se_dbg_msg(SE_DBG_PLUGINS, "uri=%s", uri.c_str());

    Gtk::RecentManager::Data data;
    data.app_name = Glib::get_application_name();
    data.app_exec = Glib::get_prgname();
    data.groups.push_back("subtitleeditor-waveform");
    data.is_private = false;
    Gtk::RecentManager::get_default()->add_item(uri, data);
  }

  // Open a recent video
  void on_recent_item_activated() {
    se_dbg(SE_DBG_PLUGINS);

    Glib::RefPtr<Gtk::Action> action =
        action_group->get_action("waveform/recent-files");

    Glib::RefPtr<Gtk::RecentAction> recentAction =
        Glib::RefPtr<Gtk::RecentAction>::cast_static(action);

    Glib::RefPtr<Gtk::RecentInfo> cur = recentAction->get_current_item();
    if (cur) {
      se_dbg_msg(SE_DBG_PLUGINS, "uri=%s", cur->get_uri().c_str());

      Glib::RefPtr<Waveform> wf = Waveform::create_from_file(cur->get_uri());
      if (wf)
        get_waveform_manager()->set_waveform(wf);
    }
  }

 protected:
  Gtk::UIManager::ui_merge_id ui_id;
  Glib::RefPtr<Gtk::ActionGroup> action_group;
};


REGISTER_EXTENSION(WaveformManagement)
