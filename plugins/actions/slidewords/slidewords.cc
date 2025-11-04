//
// SlideWords plugin Copyright 2022 Tomáš Pártl, tomaspartl@centrum.cz.
// This file is a part of subtitleeditor Copyright @ 2005-2022, kitone.
//
// https://subtitleeditor.github.io/subtitleeditor/
// https://github.com/subtitleeditor/subtitleeditor/
//
// This plugin allows the user to slide words between lines and subtitles.
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
#include <utility.h>

class SlideWordsPlugin : public Action {
 public:
  SlideWordsPlugin() {
    activate();
    update_ui();
  }

 	~SlideWordsPlugin()
	{
		deactivate();
	}

	/*
	 */
	void activate()
	{
		se_dbg(SE_DBG_PLUGINS);

		// actions
		action_group = Gtk::ActionGroup::create("SlideWordsPlugin");

    action_group->add(Gtk::Action::create(
        "menu-slide-words", _("Slide Words"), _("Move one word between lines or between two subtitles")));

		action_group->add(
				Gtk::Action::create("slide-word-next-line", _("Slide Word To Next Line"),
				_("Slides one word from the end of the first line to the beginning of the second one (this also works when exactly two subtitles are selected, moving the words between them then)")),
					sigc::mem_fun(*this, &SlideWordsPlugin::on_slide_word_next_line));

		action_group->add(
				Gtk::Action::create("slide-word-prev-line", _("Slide Word To Previous Line"),
				_("Slides one word from the beginning of the second line to the end of the first one (this also works when exactly two subtitles are selected, moving the words between them then)")),
					sigc::mem_fun(*this, &SlideWordsPlugin::on_slide_word_prev_line));

		// ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui->insert_action_group(action_group);

    Glib::ustring submenu = R"(
      <ui>
        <menubar name='menubar'>
          <menu name='menu-edit' action='menu-edit'>
            <placeholder name='slide-words'>
              <menu action='menu-slide-words'>
                <menuitem action='slide-word-next-line'/>
                <menuitem action='slide-word-prev-line'/>
              </menu>
            </placeholder>
          </menu>
        </menubar>
      </ui>
    )";

    ui_id = ui->add_ui_from_string(submenu);
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

		action_group->get_action("menu-slide-words")->set_sensitive(visible);
	}

protected:

	void on_slide_word_next_line()
	{
		slide_word_x_line( true ); 
	}

	void on_slide_word_prev_line()
	{
		slide_word_x_line( false ); 
	}

	void slide_word_x_line( bool next )
	{
		se_dbg(SE_DBG_PLUGINS);

		Document *doc = get_current_document();
		g_return_if_fail(doc);

    Subtitles subtitles = doc->subtitles();
    std::vector<Subtitle> selection = subtitles.get_selection();

    if (selection.empty()) {
      doc->flash_message(_("Please select at least one subtitle."));
      return;
    }

		doc->start_command( next ? _("Slide one word to the next line") : _("Slide one word to the previous line"));

		if( selection.size() == 1 ) {
	      Glib::ustring text = selection[0].get_text();
	      if( next ) {
	      	slide_word_next_line( text );
	      } else {
	      	slide_word_prev_line( text );
	      }
	      selection[0].set_text(text);
		} else if( selection.size() == 2 ) {
	    Glib::ustring text1 = selection[0].get_text();
	    Glib::ustring text2 = selection[1].get_text();
	    if( next ) {
	    	slide_word_next_text( text1, text2 );
	    } else {
	    	slide_word_prev_text( text1, text2 );
	    }
	    selection[0].set_text(text1);
	    selection[1].set_text(text2);
		} else {
	    for (auto& subtitle : selection) {
	      Glib::ustring text = subtitle.get_text();
	      if( next ) {
	      	slide_word_next_line( text );
	      } else {
	      	slide_word_prev_line( text );
	      }
	      subtitle.set_text(text);
	    }
		}

		doc->emit_signal("subtitle-text-changed");
		doc->finish_command();
	}

	void slide_word_next_line( Glib::ustring &text ) {
		size_t end = text.find('\n');
		size_t space = text.rfind(' ', end);
	
		if( space != Glib::ustring::npos ) {
			text.replace( space, 1, "\n" );
		}
		if( end != Glib::ustring::npos ) {
			text.replace( end, 1, " " );
		}
	}

	void slide_word_prev_line( Glib::ustring &text ) {

		size_t end = 0;
		while( end == 0 ) {
			end = text.find('\n');
			if( end == 0 ) {
				text.erase( 0, 1 );
			}
		}

		size_t space = text.find(' ', ( end == Glib::ustring::npos ) ? 0 : end);

		if( space != Glib::ustring::npos ) {
			text.replace( space, 1, "\n" );
		}
		if( end != Glib::ustring::npos ) {
			text.replace( end, 1, " " );
		}
	}

	void slide_word_next_text( Glib::ustring &text1, Glib::ustring &text2 ) {
		if( text1.size() == 0 ) { return; }

		size_t end = text1.rfind('\n');
		while(( end != Glib::ustring::npos )&&( end == text1.size() - 1 )) {
			text1.erase( end, 1 );
			end = text1.rfind('\n');
		}
		size_t space = text1.find_last_of(" \n\t");
		size_t start = space + 1;
		if( space == Glib::ustring::npos ) {
			start = 0;
			space = 0;
		}

		bool addspace = (( text2.size() == 0 )||( text2[0] == ' ' )||( text2[0] == '\n' )) ? false : true;
		if( addspace ) {
			text2.insert(0, " ");
		}
		text2.insert( 0, text1, start, Glib::ustring::npos );
		text1.erase( space );
	}

	void slide_word_prev_text( Glib::ustring &text1, Glib::ustring &text2 ) {
		size_t space = text2.find_first_of(" \n\t");
		while( space == 0 ) {
			text2.erase( 0, 1 );
			space = text2.find(' ');
		}

		if( text2.size() == 0 ) { return; }

		size_t end = space;
		size_t erase = space;
		if( space != Glib::ustring::npos ) {
			erase = space + 1;
		}

		bool addspace = (( text1.size() == 0 )||( text1[ text1.size() - 1 ] == ' ' )||( text2[ text1.size() - 1 ] == '\n' )) ? false : true;
		if( addspace ) {
			text1.append(" ");
		}
		text1.append( text2, 0, end );
		text2.erase( 0, erase );
	}

protected:
	Gtk::UIManager::ui_merge_id ui_id;
	Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(SlideWordsPlugin)
