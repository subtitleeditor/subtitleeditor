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

class SelectionPlugin : public Action {
 public:
  SelectionPlugin() {
    activate();
    update_ui();
  }

  ~SelectionPlugin() {
    deactivate();
  }

  void activate() {
    se_dbg(SE_DBG_PLUGINS);

    // actions
    action_group = Gtk::ActionGroup::create("SelectionPlugin");

    action_group->add(
        Gtk::Action::create("select-first-subtitle", Gtk::Stock::GO_UP,
                            _("Select _First Subtitle"),
                            _("Select the first subtitle")),
        sigc::mem_fun(*this, &SelectionPlugin::on_select_first_subtitle));

    action_group->add(
        Gtk::Action::create("select-last-subtitle", Gtk::Stock::GO_DOWN,
                            _("Select _Last Subtitle"),
                            _("Select the last subtitle")),
        sigc::mem_fun(*this, &SelectionPlugin::on_select_last_subtitle));

    action_group->add(
        Gtk::Action::create("select-previous-subtitle", Gtk::Stock::GO_BACK,
                            _("Select _Previous Subtitle"),
                            _("Select the previous subtitle")),
        sigc::mem_fun(*this, &SelectionPlugin::on_select_previous_subtitle));

    action_group->add(
        Gtk::Action::create("select-next-subtitle", Gtk::Stock::GO_FORWARD,
                            _("Select _Next Subtitle"),
                            _("Select the next subtitle")),
        sigc::mem_fun(*this, &SelectionPlugin::on_select_next_subtitle));

    action_group->add(
        Gtk::Action::create(
            "select-all-subtitles", Gtk::StockID("gtk-select-all"),
            _("Select _All Subtitles"), _("Select all subtitles")),
        Gtk::AccelKey("<Control>A"),
        sigc::mem_fun(*this, &SelectionPlugin::on_select_all_subtitles));

    action_group->add(
        Gtk::Action::create("unselect-all-subtitles",
                            _("_Unselect All Subtitles"),
                            _("Unselect all the subtitles")),
        Gtk::AccelKey("<Shift><Control>A"),
        sigc::mem_fun(*this, &SelectionPlugin::on_unselect_all_subtitles));

    action_group->add(
        Gtk::Action::create("invert-subtitles-selection",
                            _("In_vert Selection"),
                            _("Invert subtitles selection")),
        Gtk::AccelKey("<Control>I"),
        sigc::mem_fun(*this, &SelectionPlugin::on_invert_selection));

    action_group->add(
        Gtk::Action::create("select-to-start",
                            _("Select to Start"),
                            _("Select all subtitles from current to start")),
        sigc::mem_fun(*this, &SelectionPlugin::on_select_to_start));

    action_group->add(
        Gtk::Action::create("select-to-end",
                            _("Select to End"),
                            _("Select all subtitles from current to end")),
        sigc::mem_fun(*this, &SelectionPlugin::on_select_to_end));

    // ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui->insert_action_group(action_group);

    Glib::ustring submenu = R"(
      <ui>
        <menubar name='menubar'>
          <menu name='menu-selection' action='menu-selection'>
            <placeholder name='selection'>
              <menuitem action='select-first-subtitle'/>
              <menuitem action='select-last-subtitle'/>
              <menuitem action='select-previous-subtitle'/>
              <menuitem action='select-next-subtitle'/>
              <separator/>
              <menuitem action='select-all-subtitles'/>
              <menuitem action='unselect-all-subtitles'/>
              <menuitem action='invert-subtitles-selection'/>
              <menuitem action='select-to-start'/>
              <menuitem action='select-to-end'/>
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

    action_group->get_action("select-first-subtitle")->set_sensitive(visible);
    action_group->get_action("select-last-subtitle")->set_sensitive(visible);
    action_group->get_action("select-previous-subtitle")
        ->set_sensitive(visible);
    action_group->get_action("select-next-subtitle")->set_sensitive(visible);
    action_group->get_action("select-all-subtitles")->set_sensitive(visible);
    action_group->get_action("unselect-all-subtitles")->set_sensitive(visible);
    action_group->get_action("invert-subtitles-selection")
        ->set_sensitive(visible);
    action_group->get_action("select-to-start") ->set_sensitive(visible);
    action_group->get_action("select-to-end") ->set_sensitive(visible);
  }

 protected:
  void on_select_first_subtitle() {
    se_dbg(SE_DBG_PLUGINS);

    execute(FIRST);
  }

  void on_select_last_subtitle() {
    se_dbg(SE_DBG_PLUGINS);

    execute(LAST);
  }

  void on_select_previous_subtitle() {
    se_dbg(SE_DBG_PLUGINS);

    execute(PREVIOUS);
  }

  void on_select_next_subtitle() {
    se_dbg(SE_DBG_PLUGINS);

    execute(NEXT);
  }

  void on_select_all_subtitles() {
    se_dbg(SE_DBG_PLUGINS);

    execute(ALL);
  }

  void on_unselect_all_subtitles() {
    se_dbg(SE_DBG_PLUGINS);

    execute(UNSELECT);
  }

  void on_invert_selection() {
    se_dbg(SE_DBG_PLUGINS);

    execute(INVERT);
  }

  void on_select_to_start() {
    se_dbg(SE_DBG_PLUGINS);

    execute(TOSTART);
  }

  void on_select_to_end() {
    se_dbg(SE_DBG_PLUGINS);

    execute(TOEND);
  }

 protected:
  enum TYPE { FIRST, LAST, PREVIOUS, NEXT, ALL, INVERT, UNSELECT, TOSTART, TOEND };

  bool execute(TYPE type) {
    se_dbg(SE_DBG_PLUGINS);

    Document *doc = get_current_document();

    g_return_val_if_fail(doc, false);

    Subtitles subtitles = doc->subtitles();

    if (subtitles.size() == 0)
      return false;

    if (type == ALL) {
      subtitles.select_all();
      return true;
    } else if (type == UNSELECT) {
      subtitles.unselect_all();
      return true;
    } else if (type == INVERT) {
      subtitles.invert_selection();
      return true;
    } else if (type == PREVIOUS || type == NEXT) {
      std::vector<Subtitle> selection = subtitles.get_selection();

      if (selection.empty()) {
        Subtitle sub = subtitles.get_first();
        if (sub)
          subtitles.select(sub);
      } else {
        Subtitle first = selection[0];

        Subtitle sub = (type == PREVIOUS) ? subtitles.get_previous(first)
                                          : subtitles.get_next(first);

        if (sub)
          subtitles.select(sub);
      }
    } else if (type == FIRST || type == LAST) {
      Subtitle sub =
          (type == FIRST) ? subtitles.get_first() : subtitles.get_last();
      if (sub)
        subtitles.select(sub);
    } else if ( type == TOSTART ) {
      std::vector<Subtitle> selection = subtitles.get_selection();
      if (!selection.empty()) {
    		Subtitle sub = selection[ selection.size() - 1 ];
        sub = subtitles.get_previous( sub );
      	while( sub ) {
      	 selection.push_back( sub );
      	 sub = subtitles.get_previous( sub );
      	}
      	subtitles.select( selection );
    	}
    } else if ( type == TOEND ) {
      std::vector<Subtitle> selection = subtitles.get_selection();
      if (!selection.empty()) {
    		Subtitle sub = selection[ selection.size() - 1 ];
        sub = subtitles.get_next( sub );
      	while( sub ) {
      	 selection.push_back( sub );
      	 sub = subtitles.get_next( sub );
      	}
      	subtitles.select( selection );
    	}
    }
    return false;
  }

 protected:
  Gtk::UIManager::ui_merge_id ui_id;
  Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(SelectionPlugin)
