// subtitleeditor -- a tool to create or edit subtitle
//
// https://kitone.github.io/subtitleeditor/
// https://github.com/kitone/subtitleeditor/
//
// subtitleeditor is Copyright @ 2005-2018, kitone
// this file is Copyright 2024 Eltomito <tomaspartl@centrum.cz>
//
// This subtitleeditor plugin finds the subtitle
// at the current player position
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
//#include <i18n.h>
#include <player.h>
#include <utility.h>

class FindByTimePlugin : public Action {
 public:
  FindByTimePlugin() {
    activate();
    update_ui();
  }

 	~FindByTimePlugin()
	{
		deactivate();
	}

	/*
	 */
	void activate()
	{
		se_dbg(SE_DBG_PLUGINS);

		// actions
		action_group = Gtk::ActionGroup::create("FindByTimePlugin");

		action_group->add(
				Gtk::Action::create("find-by-time", _("Find Subtitle By Time"),
				_("Finds the subtitle at the current player position.")),
					sigc::mem_fun(*this, &FindByTimePlugin::on_find_by_time));

		// ui
		Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

		ui_id = ui->new_merge_id();

		ui->insert_action_group(action_group);

		ui->add_ui(ui_id, "/menubar/menu-tools/find-by-time", "find-by-time", "find-by-time");
	}

	/*
	 */
	void deactivate()
	{
		se_dbg(SE_DBG_PLUGINS);

		Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

		ui->remove_ui(ui_id);
		ui->remove_action_group(action_group);
	}

	/*
	 */
	void update_ui()
	{
		se_dbg(SE_DBG_PLUGINS);

		bool visible = (get_current_document() != NULL);

		action_group->get_action("find-by-time")->set_sensitive(visible);
	}

protected:

	/*
	 */
	void on_find_by_time()
	{
		se_dbg(SE_DBG_PLUGINS);

		Document *doc = get_current_document();
		g_return_if_fail(doc);

    Subtitles subs = doc->subtitles();

    long playerpos = get_subtitleeditor_window()->get_player()->get_position();

    Subtitle cursub = subs.get_first();
    if( !cursub ) {
			doc->flash_message(_("No subtitles."));
      return;
    }

    Subtitle closest = cursub;
    long closestdist = LONG_MAX;

    while( cursub ) {
      long s = cursub.get_start().totalmsecs;
      long e = cursub.get_end().totalmsecs;
      long dist = ( playerpos >= s ) ? ( ( playerpos < e ) ? 0 : ( playerpos - e ) ) : ( s - playerpos ) ;
      if( dist == 0 ) {
        closest = cursub;
        closestdist = 0;
        break;
      }
      if( dist < closestdist ) {
        closest = cursub;
        closestdist = dist;
      }      
			cursub = subs.get_next( cursub );
		}
		if( closestdist != LONG_MAX ) {
      doc->subtitles().select( closest );
  		doc->emit_signal("subtitle-selection-changed");
    }
	}

protected:
	Gtk::UIManager::ui_merge_id ui_id;
	Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(FindByTimePlugin)
