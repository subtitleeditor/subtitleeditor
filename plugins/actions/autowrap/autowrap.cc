//
// Autowrap plugin Copyright 2024 Tomáš Pártl, tomaspartl@centrum.cz.
// This file is a part of subtitleeditor Copyright @ 2005-2024 kitone and others.
//
// https://kitone.github.io/subtitleeditor/
// https://github.com/kitone/subtitleeditor/
//
// This plugin automatically wraps the text in a subtitle
// into lines of appropriate length.
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

class AutoWrapPlugin : public Action {
 public:
  AutoWrapPlugin() {
    activate();
    update_ui();
  }

 	~AutoWrapPlugin()
	{
		deactivate();
	}

	/*
	 */
	void activate()
	{
		se_dbg(SE_DBG_PLUGINS);

		// actions
		action_group = Gtk::ActionGroup::create("AutoWrapPlugin");

    action_group->add(Gtk::Action::create(
        "menu-autowrap", _("Wrap Text")));

		action_group->add(
				Gtk::Action::create("autowrap-wide", _("Wrap Text Wide"),
				_("Wraps the text fitting as many words on each line as possible.")),
					sigc::mem_fun(*this, &AutoWrapPlugin::on_autowrap_wide));

		action_group->add(
				Gtk::Action::create("autowrap-evenly", _("Wrap Text Evenly"),
				_("Wraps the text into lines of a similar width.")),
					sigc::mem_fun(*this, &AutoWrapPlugin::on_autowrap_evenly));

		// ui
    Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

    ui->insert_action_group(action_group);

    Glib::ustring submenu = R"(
      <ui>
        <menubar name='menubar'>
          <menu name='menu-tools' action='menu-tools'>
            <placeholder name='autowrap-placeholder'>
              <menu action='menu-autowrap'>
                <menuitem action='autowrap-wide'/>
                <menuitem action='autowrap-evenly'/>
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

		action_group->get_action("menu-autowrap")->set_sensitive(visible);
	}

protected:

	void on_autowrap_wide()
	{
		autowrap( cfg::get_int("timing", "max-characters-per-line"), false );
	}

	void on_autowrap_evenly()
	{
		autowrap( cfg::get_int("timing", "max-characters-per-line"), true );
	}

	void autowrap( size_t maxcpl, bool evenly )
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

		doc->start_command( _("Wrap text into lines") );

    size_t linelen = maxcpl;

    for (auto& subtitle : selection) {
      Glib::ustring text = subtitle.get_text();
      if( evenly ) {
        size_t tlen = (int) text.size();
        size_t minlines = ( ( tlen - 1 ) / maxcpl ) + 1;
        linelen = tlen / minlines;
      }
      autowrap_text( text, linelen );
      subtitle.set_text(text);
    }

		doc->emit_signal("subtitle-text-changed");
		doc->finish_command();
	}

  /** this class is a segment of something
   *  and all it does is store an index and a length.
   */
  class Segment {
    public:
      size_t index;
      size_t length;

      Segment( size_t _index = 0, size_t _length = 0 ) {
        index = _index;
        length = _length;
      }
  };

  /** a sequence of segments
   */
  typedef std::vector<Segment> SegList;

  /** a lazy debugging function
   * that dumps the content of a SegList to stdout.
   */
  void dump_seglist( const SegList &sl ) {
    for( auto& s : sl ) {
      printf("%li : %li , ", s.index, s.length);
    }
  };

  /** rewraps the supplied text IN PLACE
    * so that there's no more than maxcpl characters
    * between newlines.
    * It only breaks lines at spaces and newlines,
    * so if there's a word linger than maxcpl,
    * that word will end up on a line of its own
    * and that line will be longer than maxcpl (obviously).
    */
  void autowrap_text( Glib::ustring &text, size_t maxcpl ) {
    SegList words = make_word_list( text );
    SegList lines = word_wrap( words, maxcpl );
    rewrap_text( text, words, lines );
  }

  /** turns an unstring into a SegList
   * where every segment is one word,
   * the words are in sequence
   * and there are no words missing.
   */
  SegList make_word_list( const Glib::ustring &text ) {
    SegList res;

    size_t tlen = text.size();
    if( tlen == 0 ) return res;

    size_t wstart = 0;
 		size_t space = text.find_first_of(" \n", 0 );
 		while( space != Glib::ustring::npos ) {
 		  res.push_back( Segment( wstart, space - wstart ) );
 		  wstart = space + 1;
 		  if( wstart >= tlen ) break;
   		space = text.find_first_of(" \n", wstart );
 		}

    if( (wstart + 1 ) < tlen ) {
 		  res.push_back( Segment( wstart, tlen - wstart ) );
    }
    return res;
  }

  /** takes a SegList and arranges it into lines
   * of no more than maxcpl characters.
   * The result is another Seglist of Segments
   * of the original SegList.
   */
  SegList word_wrap( const SegList &words, size_t maxcpl  ) {
    SegList res;
    size_t numwords = words.size();
    if( numwords == 0 ) return res;

    size_t linelen = 0;
    size_t firstword = 0;
    size_t wi = 0;

    while( wi < numwords ) {
      size_t newlen = linelen + words[ wi ].length + ( (linelen == 0) ? 0: 1 );
      if( newlen >= maxcpl ) {

        if( newlen > maxcpl ) {
          if( wi > firstword ) {
            --wi;
          }
        }

        res.push_back( Segment( firstword, wi - firstword + 1 ) );
        ++wi;
        firstword = wi;
        linelen = 0;
      } else {
        linelen = newlen;
        ++wi;
      }
    }

    if( wi > firstword ) {
        res.push_back( Segment( firstword, wi - firstword + 1 ) );
    }

    return res;
  }

  /** This function rewraps a ustring according to a SegList of lines
   * in a SegList of words in the ustring.
   * The supplied ustring is changed in place.
   * Its length doesn't change.
   */
  void rewrap_text( Glib::ustring &text, const SegList &words, const SegList &lines ) {
    size_t tlen = text.size();
    size_t prev_end = 0;

    //fill spaces between words with... spaces ;)
    for (auto& word : words) {
      text.replace( prev_end, word.index - prev_end, word.index - prev_end, ' ');
      prev_end = word.index + word.length;
    }

    //put newlines at ends of lines
    for (auto& line : lines) {
      if( line.length > 0 ) {
        const Segment &lastword = words[ line.index + line.length - 1 ];
        size_t nli = lastword.index + lastword.length;
        if( nli < tlen ) {
          text.replace( nli, 1, 1, '\n' );
        }
      }
    }
  }

protected:
	Gtk::UIManager::ui_merge_id ui_id;
	Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(AutoWrapPlugin)
