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
#include <gtkmm_utility.h>
#include <gui/comboboxsubtitleformat.h>
#include <gui/dialogfilechooser.h>
#include <player.h>
#include <utility.h>
#include <widget_config_utility.h>

class DialogExternalVideoPreferences : public Gtk::Dialog {
 public:
  DialogExternalVideoPreferences(BaseObjectType *cobject,
                                 const Glib::RefPtr<Gtk::Builder> &xml)
      : Gtk::Dialog(cobject) {
    Gtk::Entry *entry = NULL;
    xml->get_widget("entry-video-player-command", entry);
    widget_config::read_config_and_connect(entry, "external-video-player",
                                           "command");

    xml->get_widget("check-use-format", m_checkUseFormat);
    widget_config::read_config_and_connect(
        m_checkUseFormat, "external-video-player", "use-format");

    xml->get_widget("check-use-video-player-file", m_checkUseVideoPlayerFile);
    widget_config::read_config_and_connect(m_checkUseVideoPlayerFile,
                                           "external-video-player",
                                           "use-video-player-file");

    xml->get_widget_derived("combo-format", m_comboFormat);
    widget_config::read_config_and_connect(m_comboFormat,
                                           "external-video-player", "format");

    xml->get_widget("spin-offset", m_spinOffset);
    widget_config::read_config_and_connect(m_spinOffset,
                                           "external-video-player", "offset");

    utility::set_transient_parent(*this);
  }

  static void create() {
    std::unique_ptr<DialogExternalVideoPreferences> dialog(
        gtkmm_utility::get_widget_derived<DialogExternalVideoPreferences>(
            SE_DEV_VALUE(SE_PLUGIN_PATH_UI, SE_PLUGIN_PATH_DEV),
            "dialog-external-video-player-preferences.ui",
            "dialog-external-video-player-preferences"));

    dialog->run();
  }

 protected:
  Gtk::CheckButton *m_checkUseFormat;
  Gtk::CheckButton *m_checkUseVideoPlayerFile;
  ComboBoxSubtitleFormat *m_comboFormat;
  Gtk::SpinButton *m_spinOffset;
};

class ExternalVideoPlayer : public Action {
 public:
  ExternalVideoPlayer() {
    activate();
    update_ui();
  }

  ~ExternalVideoPlayer() {
    deactivate();
  }

  void activate() {
    // actions
    action_group = Gtk::ActionGroup::create("ExternalVideoPlayer");

    action_group->add(Gtk::Action::create("menu-external-video-player",
                                          Gtk::Stock::MEDIA_PLAY,
                                          _("_External Video Player")));

    action_group->add(
        Gtk::Action::create("external-video-player/open", Gtk::Stock::OPEN,
                            _("_Open Movie"),
                            _("Open movie with external video player")),
        Gtk::AccelKey("<Shift><Control>P"),
        sigc::mem_fun(*this, &ExternalVideoPlayer::on_open_movie));

    action_group->add(
        Gtk::Action::create("external-video-player/play",
                            Gtk::Stock::MEDIA_PLAY, _("_Play Movie"),
                            _("Play movie with external video player")),
        Gtk::AccelKey("<Control>space"),
        sigc::mem_fun(*this, &ExternalVideoPlayer::on_play_movie));

    action_group->add(
        Gtk::Action::create("external-video-player/preferences",
                            Gtk::Stock::PREFERENCES, "",
                            _("External video player preferences")),
        sigc::mem_fun(*this, &ExternalVideoPlayer::create_configure_dialog));

    // ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui->insert_action_group(action_group);

    Glib::ustring submenu = R"(
      <ui>
        <menubar name='menubar'>
          <menu name='menu-extensions' action='menu-extensions'>
            <placeholder name='placeholder'>
              <menu action='menu-external-video-player'>
                <menuitem action='external-video-player/open'/>
                <menuitem action='external-video-player/play'/>
                <separator/>
                <menuitem action='external-video-player/preferences'/>
              </menu>
            </placeholder>
          </menu>
        </menubar>
      </ui>
    )";

    ui_id = ui->add_ui_from_string(submenu);
  }

  void deactivate() {
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui->remove_ui(ui_id);
    ui->remove_action_group(action_group);
  }

  bool is_configurable() {
    return true;
  }

  void create_configure_dialog() {
    DialogExternalVideoPreferences::create();
  }

  void on_open_movie() {
    DialogOpenVideo ui;
    if (ui.run() == Gtk::RESPONSE_OK)
      m_movie_uri = ui.get_uri();
  }

  void on_play_movie() {
    Document *doc = get_current_document();

    g_return_if_fail(doc);

    if (cfg::get_boolean("external-video-player", "use-video-player-file")) {
      Player *player = get_subtitleeditor_window()->get_player();
      if (player->get_state() != Player::NONE)
        m_movie_uri = player->get_uri();
    }

    // If the user call directly the action 'play movie' without video
    // we propose to choose one
    if (m_movie_uri.empty())
      on_open_movie();

    // Check again if we have now a movie
    if (m_movie_uri.empty()) {
      doc->flash_message(_("Please select a movie."));
      return;
    }

    // Save the document in a temporary directory
    save_to_temporary_file(doc, get_tmp_file_as_uri());

    // create the command
    SubtitleTime time = get_start_position(doc);

    Glib::ustring command = get_command();

    utility::replace(command, "#video_file",
                     Glib::filename_from_uri(m_movie_uri));
    utility::replace(command, "#video_uri", m_movie_uri);
    utility::replace(command, "#subtitle_file", get_tmp_file());
    utility::replace(command, "#subtitle_uri", get_tmp_file_as_uri());
    utility::replace(command, "#seconds", convert_to_second_string(time));
    utility::replace(command, "#mseconds", convert_to_msecond_string(time));
    utility::replace(command, "#time", convert_to_time_string(time));

    std::cout << "COMMAND: " << command << std::endl;

    try {
      se_dbg_msg(SE_DBG_PLUGINS, command.c_str());
      Glib::spawn_command_line_async(command);
    } catch (const Glib::Error &ex) {
      dialog_error(_("Failed to launch the external player."),
                   build_message(_("%s\n\nCommand: <i>%s</i>"),
                                 ex.what().c_str(), command.c_str()));
    }
  }

 protected:
  // Return the command pipe from the config (or default)
  Glib::ustring get_command() {
    // load the config or use the default command
    if (cfg::has_key("external-video-player", "command")) {
      return cfg::get_string("external-video-player", "command");
    }
    // write the default command in the config
	// 2025: I suspect the below is never called
    auto default_cmd = R"(
      mpv --sub-file="#subtitle_uri" --start="#time" "#video_uri" --sub-auto=no --osd-level=2
    )";

    cfg::set_string("external-video-player", "command", default_cmd);

    return default_cmd;
  }

  // If the user has specified a subtitle format use it
  Glib::ustring get_prefered_subtitle_format() {
    if (cfg::get_boolean("external-video-player", "use-format")) {
      if (cfg::has_key("external-video-player", "format")) {
        return cfg::get_string("external-video-player", "format");
      }
    }
    return Glib::ustring();
  }

  SubtitleTime get_prefered_offset() {
    int offset = 4000;
    if (cfg::has_key("external-video-player", "offset")) {
      offset = cfg::get_int("external-video-player", "offset");
    }
    return SubtitleTime(offset);
  }

  Glib::ustring get_tmp_file() {
    return Glib::build_filename(Glib::get_tmp_dir(), "subtitle_preview");
  }

  Glib::ustring get_tmp_file_as_uri() {
    return Glib::filename_to_uri(get_tmp_file());
  }

  SubtitleTime get_start_position(Document *document) {
    std::vector<Subtitle> selection = document->subtitles().get_selection();

    if (selection.empty())
      return SubtitleTime(0);

    Subtitle sub = selection[0];

    SubtitleTime time = sub.get_start() - get_prefered_offset();
    if (time.totalmsecs < 0)
      return SubtitleTime(0);
    return time;
  }

  Glib::ustring convert_to_time_string(const SubtitleTime &time) {
    return time.str();
  }

  Glib::ustring convert_to_second_string(const SubtitleTime &time) {
    long p = time.hours() * 3600 + time.minutes() * 60 + time.seconds();
    return to_string(p);
  }

  Glib::ustring convert_to_msecond_string(const SubtitleTime &time) {
    return to_string(time.totalmsecs);
  }

  void save_to_temporary_file(Document *document, const Glib::ustring &uri) {
    Glib::ustring prefered_format = get_prefered_subtitle_format();

    // FIXME: fixes this shit after the new subtitle format system

    // Old values
    Glib::ustring old_format = document->getFormat();
    Glib::ustring old_filename = document->getFilename();

    if (!prefered_format.empty())
      document->setFormat(prefered_format);

    document->save(uri);

    // Restore default values
    document->setFormat(old_format);
    document->setFilename(old_filename);
  }

 protected:
  Gtk::UIManager::ui_merge_id ui_id;
  Glib::RefPtr<Gtk::ActionGroup> action_group;

  Glib::ustring m_movie_uri;
};

REGISTER_EXTENSION(ExternalVideoPlayer)
