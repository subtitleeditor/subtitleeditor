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

   ~AutoWrapPlugin() {
      deactivate();
   }

   /*
    */
   void activate() {
      se_dbg(SE_DBG_PLUGINS);

      // actions
      action_group = Gtk::ActionGroup::create("AutoWrapPlugin");

      action_group->add(Gtk::Action::create("menu-autowrap", _("Wrap Text"), _("Reflow the text of the selected subtitles ")));

      action_group->add(Gtk::Action::create("autowrap-wide",
                                            _("Wrap Text Wide"),
                                            _("Reflow the text of selected subtitles fitting as many words on each line as possible while respecting "
                                              "the maximum characters per line setting")),
                        sigc::mem_fun(*this, &AutoWrapPlugin::on_autowrap_wide));

      action_group->add(Gtk::Action::create("autowrap-evenly",
                                            _("Wrap Text Evenly"),
                                            _("Reflow the text of the selected subtitles into lines of a similar width while respecting the "
                                              "maximum characters per line setting")),
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
   void deactivate() {
      se_dbg(SE_DBG_PLUGINS);

      Glib::RefPtr<Gtk::UIManager> ui = get_ui_manager();

      ui->remove_ui(ui_id);
      ui->remove_action_group(action_group);
   }

   /*
    */
   void update_ui() {
      se_dbg(SE_DBG_PLUGINS);

      bool visible = (get_current_document() != NULL);

      action_group->get_action("menu-autowrap")->set_sensitive(visible);
   }

  protected:
   void on_autowrap_wide() {
      autowrap(cfg::get_int("timing", "max-characters-per-line"), false);
   }

   void on_autowrap_evenly() {
      autowrap(cfg::get_int("timing", "max-characters-per-line"), true);
   }

   void autowrap(size_t maxcpl, bool evenly) {
      se_dbg(SE_DBG_PLUGINS);

      Document* doc = get_current_document();
      g_return_if_fail(doc);

      Subtitles subtitles = doc->subtitles();
      std::vector<Subtitle> selection = subtitles.get_selection();

      if (selection.empty()) {
         doc->flash_message(_("Please select at least one subtitle."));
         return;
      }

      doc->start_command(_("Wrap text into lines"));

      size_t linelen = maxcpl;

      for (auto& subtitle : selection) {
         Glib::ustring text = subtitle.get_text();
         autowrap_text(text, linelen, evenly);
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

      Segment(size_t _index = 0, size_t _length = 0) {
         index = _index;
         length = _length;
      }
   };
   /** a sequence of segments
    */
   typedef std::vector<Segment> SegList;

   static size_t iterdiff(Glib::ustring::iterator start, Glib::ustring::iterator end) {
      size_t res = 0;
      while (start != end) {
         ++res;
         ++start;
      }
      return res;
   };

   class Word {
     public:
      Glib::ustring::iterator start;
      Glib::ustring::iterator end;

      Word() {};
      Word(Glib::ustring::iterator _start, Glib::ustring::iterator _end) {
         start = _start;
         end = _end;
      }

      size_t length() {
         return iterdiff(start, end);
      };
   };
   typedef std::vector<Word> WordList;

   /** returns the length of a line that consists
    * of words from a WordList
    */
   size_t line_length(Segment& line, WordList& words) {
      size_t res = 0;
      size_t end = line.index + line.length;
      for (size_t i = line.index; i < end; ++i) {
         res += words[i].length();
      }
      res += (line.length > 0) ? (line.length - 1) : 0;
      return res;
   }

   /** a lazy debugging function
    * that dumps the content of a SegList to stdout.
    */
   void dump_seglist(const SegList& sl) {
      for (auto& s : sl) {
         printf("%li : %li , ", s.index, s.length);
      }
   };

   /** Rewraps the supplied text IN PLACE
    * so that there's no more than maxcpl characters
    * between newlines.
    * It only breaks lines at spaces and newlines,
    * so if there's a word linger than maxcpl,
    * that word will end up on a line of its own
    * and that line will be longer than maxcpl (obviously).
    */
   void autowrap_text(Glib::ustring& text, size_t maxcpl, bool balance = false) {
      WordList words = make_word_list(text);
      SegList lines = word_wrap(words, maxcpl);
      if (balance)
         balance_wrap(lines, words, maxcpl);
      rewrap_text(text, words, lines);
   }

   /** Moves the last word of the previous line
    * to the start of the next line
    * if it makes the line difference smaller
    * and if the bottom line doesn't exceed maxcpl.
    * It works this way through all lines from the bottom up.
    */
   bool snake_words_down(SegList& lines, WordList& words, size_t maxcpl) {
      bool res = false;

      if (lines.size() < 2)
         return false;

      size_t li = lines.size() - 1;
      size_t botlen = line_length(lines[li], words);
      size_t toplen = 0;

      while (li > 0) {
         toplen = line_length(lines[li - 1], words);
         int lendiff = (int)toplen - (int)botlen;

         if (lendiff > 0) {
            size_t snakewordlen = (words[lines[li - 1].index + lines[li - 1].length - 1]).length();
            size_t newbotlen = botlen + snakewordlen + ((botlen > 0) ? 1 : 0);
            size_t newtoplen = toplen - snakewordlen + ((toplen > 1) ? 1 : 0);
            int newlendiff = (int)newtoplen - (int)newbotlen;

            if ((std::abs(newlendiff) <= lendiff) && (newbotlen <= maxcpl)) {
               res = true;
               lines[li].index -= 1;
               lines[li].length += 1;
               lines[li - 1].length -= 1;
               toplen = newtoplen;
            }
         }
         botlen = toplen;
         --li;
      }

      return res;
   };

   /** Balances a SegList of lines by snaking words down
    *  as long as possible.
    */
   void balance_wrap(SegList& lines, WordList& words, size_t maxcpl) {
      if (lines.size() < 2)
         return;
      bool changed = true;
      while (changed) {
         changed = snake_words_down(lines, words, maxcpl);
      }
   }

   static Glib::ustring::iterator Iter_find_first_of(Glib::ustring& text,
                                                     Glib::ustring::iterator sit,
                                                     Glib::ustring::iterator eit,
                                                     Glib::ustring what) {
      size_t s = iterdiff(text.begin(), sit);
      size_t i = text.find_first_of(what, s);
      if (i == Glib::ustring::npos) {
         return eit;
      }
      std::advance(sit, (int)i - (int)s);
      return sit;
   }

   WordList make_word_list(Glib::ustring& text) {
      WordList res;

      size_t tlen = text.size();
      if (tlen == 0)
         return res;

      Glib::ustring::iterator sit = text.begin();
      Glib::ustring::iterator eit = text.end();

      Glib::ustring::iterator spaceit = Iter_find_first_of(text, sit, eit, " \n");
      while (spaceit != eit) {
         res.push_back(Word(sit, spaceit));
         sit = spaceit;
         ++sit;
         if (sit == eit)
            break;
         spaceit = Iter_find_first_of(text, sit, eit, " \n");
      }

      if (sit != eit) {
         res.push_back(Word(sit, eit));
      }
      return res;
   }

   /** takes a WordList and arranges it into lines
    * of no more than maxcpl characters.
    * The result is a Seglist of Segments
    * of the original WordList.
    */
   SegList word_wrap(WordList& words, size_t maxcpl) {
      SegList res;
      size_t numwords = words.size();
      if (numwords == 0)
         return res;

      size_t linelen = 0;
      size_t firstword = 0;
      size_t wi = 0;

      while (wi < numwords) {
         size_t newlen = linelen + words[wi].length() + ((linelen == 0) ? 0 : 1);
         if (newlen >= maxcpl) {
            if (newlen > maxcpl) {
               if (wi > firstword) {
                  --wi;
               }
            }

            res.push_back(Segment(firstword, wi - firstword + 1));
            ++wi;
            firstword = wi;
            linelen = 0;
         } else {
            linelen = newlen;
            ++wi;
         }
      }

      if (wi > firstword) {
         res.push_back(Segment(firstword, wi - firstword));
      }

      return res;
   }

   /** This function rewraps a ustring according to a SegList of lines
    * in a WordList of words in the ustring.
    * The supplied ustring is changed in place.
    * Its length doesn't change.
    */
   void rewrap_text(Glib::ustring& text, const WordList& words, const SegList& lines) {
      Glib::ustring::iterator prev_endit = text.begin();
      Glib::ustring::iterator eit = text.end();

      // fill spaces between words with... spaces ;)
      for (auto& word : words) {
         size_t replen = iterdiff(prev_endit, word.start);
         text.replace(prev_endit, word.start, replen, ' ');
         prev_endit = word.end;
      }

      // put newlines at ends of lines
      for (auto& line : lines) {
         if (line.length > 0) {
            const Word& lastword = words[line.index + line.length - 1];
            if (lastword.end != eit) {
               Glib::ustring::iterator afterend = lastword.end;
               ++afterend;
               text.replace(lastword.end, afterend, 1, '\n');
            }
         }
      }
   }

  protected:
   Gtk::UIManager::ui_merge_id ui_id;
   Glib::RefPtr<Gtk::ActionGroup> action_group;
};

REGISTER_EXTENSION(AutoWrapPlugin)
