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
#include <gtkmm.h>
#include <i18n.h>

class TimeModeManagement : public Action {
  public:
   TimeModeManagement() {
      activate();
      update_ui();
   }

   ~TimeModeManagement() {
      deactivate();
   }

   void activate() {
      se_dbg(SE_DBG_PLUGINS);

      // actions
      action_group = Gtk::ActionGroup::create("TimeModeManagement");

      Gtk::RadioAction::Group group_timing_mode;

      action_group->add(Gtk::RadioAction::create(group_timing_mode, "times", _("_Timecodes"), _("Use timecodes")),
                        sigc::bind(sigc::mem_fun(*this, &TimeModeManagement::on_set_edit_timing_mode), TIME));

      action_group->add(Gtk::RadioAction::create(group_timing_mode, "frames", _("_Frames"), _("Use frames")),
                        sigc::bind(sigc::mem_fun(*this, &TimeModeManagement::on_set_edit_timing_mode), FRAME));

      // Framerate
      Gtk::RadioAction::Group group_framerate;

      action_group->add(Gtk::Action::create("menu-framerate", _("_Framerate"), _("Set subtitle framerate")));

      action_group->add(Gtk::RadioAction::create(
                           group_framerate, "set-framerate-23.976", get_framerate_label(FRAMERATE_23_976), _("Set subtitle framerate to 23.976")),
                        sigc::bind(sigc::mem_fun(*this, &TimeModeManagement::on_set_framerate), FRAMERATE_23_976));

      action_group->add(
         Gtk::RadioAction::create(group_framerate, "set-framerate-24", get_framerate_label(FRAMERATE_24), _("Set subtitle framerate to 24")),
         sigc::bind(sigc::mem_fun(*this, &TimeModeManagement::on_set_framerate), FRAMERATE_24));

      action_group->add(
         Gtk::RadioAction::create(group_framerate, "set-framerate-25", get_framerate_label(FRAMERATE_25), _("Set subtitle framerate to 25")),
         sigc::bind(sigc::mem_fun(*this, &TimeModeManagement::on_set_framerate), FRAMERATE_25));

      action_group->add(
         Gtk::RadioAction::create(group_framerate, "set-framerate-29.97", get_framerate_label(FRAMERATE_29_97), _("Set subtitle framerate to 29.7")),
         sigc::bind(sigc::mem_fun(*this, &TimeModeManagement::on_set_framerate), FRAMERATE_29_97));

      action_group->add(
         Gtk::RadioAction::create(group_framerate, "set-framerate-30", get_framerate_label(FRAMERATE_30), _("Set subtitle framerate to 30")),
         sigc::bind(sigc::mem_fun(*this, &TimeModeManagement::on_set_framerate), FRAMERATE_30));

      // ui
      Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

      ui->insert_action_group(action_group);

      Glib::ustring submenu = R"(
      <ui>
        <menubar name='menubar'>
          <menu name='menu-timings' action='menu-timings'>
            <placeholder name='time-mode-management'>
              <menuitem action='times'/>
              <menuitem action='frames'/>
              <separator/>
              <menu action='menu-framerate'>
                <menuitem action='set-framerate-23.976'/>
                <menuitem action='set-framerate-24'/>
                <menuitem action='set-framerate-25'/>
                <menuitem action='set-framerate-29.97'/>
                <menuitem action='set-framerate-30'/>
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

      Document* doc = get_current_document();

      bool visible = (doc != NULL);

      action_group->get_action("times")->set_sensitive(visible);
      action_group->get_action("frames")->set_sensitive(visible);

      action_group->get_action("set-framerate-23.976")->set_sensitive(visible);
      action_group->get_action("set-framerate-24")->set_sensitive(visible);
      action_group->get_action("set-framerate-25")->set_sensitive(visible);
      action_group->get_action("set-framerate-29.97")->set_sensitive(visible);
      action_group->get_action("set-framerate-30")->set_sensitive(visible);

      if (doc != NULL) {
         // update the timing mode radio
         TIMING_MODE mode = doc->get_edit_timing_mode();

         Glib::RefPtr<Gtk::Action> edit_mode_action = action_group->get_action((mode == FRAME) ? "frames" : "times");
         if (edit_mode_action) {
            Glib::RefPtr<Gtk::RadioAction> radio = Glib::RefPtr<Gtk::RadioAction>::cast_static(edit_mode_action);
            if (radio)
               radio->set_active(true);
         }

         // update the framerate radio
         FRAMERATE framerate = doc->get_framerate();

         Glib::ustring action_name;

         if (framerate == FRAMERATE_23_976)
            action_name = "set-framerate-23.976";
         else if (framerate == FRAMERATE_24)
            action_name = "set-framerate-24";
         else if (framerate == FRAMERATE_25)
            action_name = "set-framerate-25";
         else if (framerate == FRAMERATE_29_97)
            action_name = "set-framerate-29.97";
         else if (framerate == FRAMERATE_30)
            action_name = "set-framerate-30";

         Glib::RefPtr<Gtk::Action> action = action_group->get_action(action_name);
         if (!action)
            return;

         Glib::RefPtr<Gtk::RadioAction> radio_action = Glib::RefPtr<Gtk::RadioAction>::cast_static(action);
         if (radio_action)
            radio_action->set_active(true);
      }
   }

  protected:
   void on_set_edit_timing_mode(TIMING_MODE mode) {
      se_dbg(SE_DBG_PLUGINS);

      Document* doc = get_current_document();

      if (doc->get_edit_timing_mode() != mode)
         doc->set_edit_timing_mode(mode);
   }

   void on_set_framerate(FRAMERATE framerate) {
      se_dbg(SE_DBG_PLUGINS);

      Document* doc = get_current_document();

      if (doc->get_framerate() != framerate)
         doc->set_framerate(framerate);
   }

  protected:
   Gtk::UIManager::ui_merge_id ui_id;
   Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(TimeModeManagement)
