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
#include "timeutility.h"

class AdjustTimePlugin : public Action {
 public:
  AdjustTimePlugin() {
    activate();
    update_ui();
  }

  ~AdjustTimePlugin() {
    deactivate();
  }

  void activate() {
    se_dbg(SE_DBG_PLUGINS);

    // actions
    action_group = Gtk::ActionGroup::create("AdjustTimePlugin");

    // -- 100 ms --

    // menu add
    action_group->add(Gtk::Action::create(
        "menu-adjust-time-add", Gtk::Stock::ADD, _("Add 100 Milliseconds")));

    action_group->add(
        Gtk::Action::create(
            "add-to-start", _("To Start"),
            _("Add 100 Milliseconds to start for all subtitles selected")),
        sigc::mem_fun(*this, &AdjustTimePlugin::on_add_to_start));

    action_group->add(
        Gtk::Action::create(
            "add-to-duration", _("To Duration"),
            _("Add 100 Milliseconds to duration for all subtitles selected")),
        sigc::mem_fun(*this, &AdjustTimePlugin::on_add_to_duration));

    action_group->add(
        Gtk::Action::create(
            "add-to-start-and-to-duration", _("To Start And To Duration"),
            _("Add 100 Milliseconds to all subtitles selected")),
        sigc::mem_fun(*this,
                      &AdjustTimePlugin::on_add_to_start_and_to_duration));

    // menu remove
    action_group->add(Gtk::Action::create("menu-adjust-time-remove",
                                          Gtk::Stock::REMOVE,
                                          _("Remove 100 Milliseconds")));

    action_group->add(
        Gtk::Action::create(
            "remove-from-start", _("From Start"),
            _("Remove 100 Milliseconds from start for all subtitles selected")),
        sigc::mem_fun(*this, &AdjustTimePlugin::on_remove_from_start));

    action_group->add(
        Gtk::Action::create("remove-from-duration", _("From Duration"),
                            _("Remove 100 Milliseconds from duration for all "
                              "subtitles selected")),
        sigc::mem_fun(*this, &AdjustTimePlugin::on_remove_from_duration));

    action_group->add(
        Gtk::Action::create(
            "remove-from-start-and-from-duration",
            _("From Start And From Duration"),
            _("Remove 100 Milliseconds from all subtitles selected")),
        sigc::mem_fun(
            *this, &AdjustTimePlugin::on_remove_from_start_and_from_duration));

    // -- 1 frame --

    // menu add
    action_group->add(Gtk::Action::create("menu-adjust-time-add-frame",
                                          Gtk::Stock::ADD, _("Add 1 Frame")));

    action_group->add(
        Gtk::Action::create(
            "add-frame-to-start", _("To Start"),
            _("Add 1 Frame to start for all subtitles selected")),
        sigc::mem_fun(*this, &AdjustTimePlugin::on_add_frame_to_start));

    action_group->add(
        Gtk::Action::create(
            "add-frame-to-duration", _("To Duration"),
            _("Add 1 Frame to duration for all subtitles selected")),
        sigc::mem_fun(*this, &AdjustTimePlugin::on_add_frame_to_duration));

    action_group->add(
        Gtk::Action::create("add-frame-to-start-and-to-duration",
                            _("To Start And To Duration"),
                            _("Add 1 Frame to all subtitles selected")),
        sigc::mem_fun(
            *this, &AdjustTimePlugin::on_add_frame_to_start_and_to_duration));

    // menu remove
    action_group->add(Gtk::Action::create("menu-adjust-time-remove-frame",
                                          Gtk::Stock::REMOVE,
                                          _("Remove 1 Frame")));

    action_group->add(
        Gtk::Action::create(
            "remove-frame-from-start", _("From Start"),
            _("Remove 1 Frame from start for all subtitles selected")),
        sigc::mem_fun(*this, &AdjustTimePlugin::on_remove_frame_from_start));

    action_group->add(
        Gtk::Action::create(
            "remove-frame-from-duration", _("From Duration"),
            _("Remove 1 Frame from duration for all subtitles selected")),
        sigc::mem_fun(*this, &AdjustTimePlugin::on_remove_frame_from_duration));

    action_group->add(
        Gtk::Action::create("remove-frame-from-start-and-from-duration",
                            _("From Start And From Duration"),
                            _("Remove 1 Frame from all subtitles selected")),
        sigc::mem_fun(
            *this,
            &AdjustTimePlugin::on_remove_frame_from_start_and_from_duration));

    // ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui->insert_action_group(action_group);

    Glib::ustring submenu = R"(
      <ui>
        <menubar name='menubar'>
          <menu name='menu-timings' action='menu-timings'>
            <placeholder name='adjust-time'>
              <menu action='menu-adjust-time-add'>
                <menuitem action='add-to-start'/>
                <menuitem action='add-to-duration'/>
                <menuitem action='add-to-start-and-to-duration'/>
              </menu>
              <menu action='menu-adjust-time-remove'>
                <menuitem action='remove-from-start'/>
                <menuitem action='remove-from-duration'/>
                <menuitem action='remove-from-start-and-from-duration'/>
              </menu>
              <menu action='menu-adjust-time-add-frame'>
                <menuitem action='add-frame-to-start'/>
                <menuitem action='add-frame-to-duration'/>
                <menuitem action='add-frame-to-start-and-to-duration'/>
              </menu>
              <menu action='menu-adjust-time-remove-frame'>
                <menuitem action='remove-frame-from-start'/>
                <menuitem action='remove-frame-from-duration'/>
                <menuitem action='remove-frame-from-start-and-from-duration'/>
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

    action_group->get_action("menu-adjust-time-add")->set_sensitive(visible);
    action_group->get_action("menu-adjust-time-remove")->set_sensitive(visible);
    action_group->get_action("menu-adjust-time-add-frame")
        ->set_sensitive(visible);
    action_group->get_action("menu-adjust-time-remove-frame")
        ->set_sensitive(visible);
  }

 protected:
  enum TYPE { START, END, START_AND_END };

  enum UNITS { MSEC, FRAMES };

  void on_add_to_start() {
    se_dbg(SE_DBG_PLUGINS);

    adjust(START, 100);
  }

  void on_add_to_duration() {
    se_dbg(SE_DBG_PLUGINS);

    adjust(END, 100);
  }

  void on_add_to_start_and_to_duration() {
    se_dbg(SE_DBG_PLUGINS);

    adjust(START_AND_END, 100);
  }

  void on_remove_from_start() {
    se_dbg(SE_DBG_PLUGINS);

    adjust(START, -100);
  }

  void on_remove_from_duration() {
    se_dbg(SE_DBG_PLUGINS);

    adjust(END, -100);
  }

  void on_remove_from_start_and_from_duration() {
    se_dbg(SE_DBG_PLUGINS);

    adjust(START_AND_END, -100);
  }

  // --- FRAME VERSIONS ---

  void on_add_frame_to_start() {
    se_dbg(SE_DBG_PLUGINS);

    adjust(START, 1, FRAMES);
  }

  void on_add_frame_to_duration() {
    se_dbg(SE_DBG_PLUGINS);

    adjust(END, 1, FRAMES);
  }

  void on_add_frame_to_start_and_to_duration() {
    se_dbg(SE_DBG_PLUGINS);

    adjust(START_AND_END, 1, FRAMES);
  }

  void on_remove_frame_from_start() {
    se_dbg(SE_DBG_PLUGINS);

    adjust(START, -1, FRAMES);
  }

  void on_remove_frame_from_duration() {
    se_dbg(SE_DBG_PLUGINS);

    adjust(END, -1, FRAMES);
  }

  void on_remove_frame_from_start_and_from_duration() {
    se_dbg(SE_DBG_PLUGINS);

    adjust(START_AND_END, -100);
  }

  bool adjust(TYPE type, const long &time_msecs = 100, UNITS units = MSEC) {
    long timeshift = time_msecs;

    se_dbg(SE_DBG_PLUGINS);

    Document *doc = get_current_document();

    g_return_val_if_fail(doc, false);

    Subtitles subtitles = doc->subtitles();

    std::vector<Subtitle> selection = subtitles.get_selection();

    if (selection.size() == 0) {
      doc->flash_message(_("Please select at least a subtitle."));
      return false;
    }

    doc->start_command(_("Adjust time"));

    if (units == FRAMES) {
      float framerate = get_framerate_value(doc->get_framerate());
      float msecperframe = (float)1000 / framerate;
      float floatshift = (float)timeshift * msecperframe;
      timeshift = (long)floatshift;
    }

    if (type == START) {
      for (unsigned int i = 0; i < selection.size(); ++i) {
        Subtitle subtitle = selection[i];

        subtitle.set_start(
            SubtitleTime(subtitle.get_start().totalmsecs + timeshift));
      }
    } else if (type == END) {
      for (unsigned int i = 0; i < selection.size(); ++i) {
        Subtitle subtitle = selection[i];

        subtitle.set_end(
            SubtitleTime(subtitle.get_end().totalmsecs + timeshift));
      }
    } else if (type == START_AND_END) {
      for (unsigned int i = 0; i < selection.size(); ++i) {
        Subtitle subtitle = selection[i];

        subtitle.set_start_and_end(
            SubtitleTime(subtitle.get_start().totalmsecs + timeshift),
            SubtitleTime(subtitle.get_end().totalmsecs + timeshift));
      }
    }

    doc->emit_signal("subtitle-time-changed");
    doc->finish_command();

    return true;
  }

 protected:
  Gtk::UIManager::ui_merge_id ui_id;
  Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(AdjustTimePlugin)
