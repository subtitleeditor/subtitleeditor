/*
 *
 *	slidetimings.cc
 *	- "justice for selected subtitles"
 *	a subtitleeditor plugin by Eltomito <tomaspartl@centrum.cz>
 *
 *	subtitleeditor -- a tool to create or edit subtitle
 *
 *	https://subtitleeditor.github.io/subtitleeditor/
 *	https://github.com/subtitleeditor/subtitleeditor/
 *
 *	Copyright @ 2005-2012, kitone
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <extension/action.h>
#include <i18n.h>
#include <debug.h>
#include <utility.h>

class SlideTimingsPlugin : public Action
{
public:

	SlideTimingsPlugin()
	{
		activate();
		update_ui();
	}

	~SlideTimingsPlugin()
	{
		deactivate();
	}

	/*
	 */
	void activate()
	{
		se_dbg(SE_DBG_PLUGINS);

		// actions
		action_group = Gtk::ActionGroup::create("SlideTimingsPlugin");

		action_group->add(
				Gtk::Action::create("bump-up", _("_Bump Text Up"),
				_("Moves the text field to the previous subtitle for all subtitles from the current one to the end.")),
					sigc::mem_fun(*this, &SlideTimingsPlugin::on_bump_up));

		action_group->add(
				Gtk::Action::create("bump-down", _("_Bump Text Down"),
				_("Moves the text field to the next subtitle for all subtitles from the current one to the end.")),
					sigc::mem_fun(*this, &SlideTimingsPlugin::on_bump_down));

		// ui
		Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

		ui_id = ui->new_merge_id();

		ui->insert_action_group(action_group);

		ui->add_ui(ui_id, "/menubar/menu-timings/bump-up", "bump-up", "bump-up");
		ui->add_ui(ui_id, "/menubar/menu-timings/bump-down", "bump-down", "bump-down");
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

		action_group->get_action("bump-up")->set_sensitive(visible);
		action_group->get_action("bump-down")->set_sensitive(visible);
	}

protected:

	/*
	 */
	void on_bump_down()
	{
		se_dbg(SE_DBG_PLUGINS);

		Document *doc = get_current_document();
		g_return_if_fail(doc);

		std::vector<Subtitle> selection = doc->subtitles().get_selection();
		if(selection.empty())
		{
			doc->flash_message(_("Please select at least one subtitle."));
			return;
		}

		doc->start_command(_("Bump Down"));

		if( selection.size() == 1 ) {
			select_to_end( doc, selection );
      Subtitle overflow = doc->subtitles().insert_after( selection[ selection.size()-1 ] );
      selection.push_back( overflow );
		}

		creep_to_pos( selection.rbegin(), selection.rend() );

		doc->emit_signal("subtitle-time-changed");
		doc->finish_command();
	}

	/*
	 */
	void on_bump_up()
	{
		se_dbg(SE_DBG_PLUGINS);

		Document *doc = get_current_document();
		g_return_if_fail(doc);

		std::vector<Subtitle> selection = doc->subtitles().get_selection();
		if(selection.empty())
		{
			doc->flash_message(_("Please select at least one subtitle."));
			return;
		}

		doc->start_command(_("Bump Up"));

		if( selection.size() == 1 ) {
			select_to_end( doc, selection );
		}

		creep_to_pos( selection.begin(), selection.end() );

		doc->emit_signal("subtitle-time-changed");
		doc->finish_command();
	}

	template <class Iterator>
	void creep_to_pos( Iterator pos, Iterator end )
	{
		if( pos == end ) return;
		Iterator lastpos = pos;
		++pos;
		while( pos != end ) {
			lastpos->set_text( pos->get_text() );
			++pos;
			++lastpos;
		}
		lastpos->set_text("");
	}

	/*
	 * Adds to the selection all the subtitles that come after it in the subtitles in the document. 
	 */
	void select_to_end( Document *doc, std::vector<Subtitle> &selection )
	{
		Subtitles subtitles = doc->subtitles();
		Subtitle sub = selection[ selection.size() - 1 ]; 
		sub = subtitles.get_next( sub );
		while( sub ) {
			selection.push_back( sub ); 
			sub = subtitles.get_next( sub );
		}
	}

protected:
	Gtk::UIManager::ui_merge_id ui_id;
	Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(SlideTimingsPlugin)

