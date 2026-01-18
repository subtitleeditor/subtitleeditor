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

#include <extension/subtitleformat.h>
#include <player.h>
#include <subtitleeditorwindow.h>
#include <utility.h>

// format:
// {start_frame}{end_frame}text
class MicroDVD : public SubtitleFormatIO {
  public:
   void open(Reader& file) {
      Glib::RefPtr<Glib::Regex> re = Glib::Regex::create("^\\{(\\d+)\\}\\{(\\d+)\\}(.*?)$");

      Glib::RefPtr<Glib::Regex> tags = Glib::Regex::create("\\{[yY]:(b|i|u)\\}(.*?)$", Glib::REGEX_MULTILINE);

      // init to frame mode
      document()->set_timing_mode(FRAME);
      document()->set_edit_timing_mode(FRAME);

      // Try to define the default value of the framerate from the player
      Player* player = SubtitleEditorWindow::get_instance()->get_player();
      if (player->get_state() != Player::NONE) {
         float player_framerate = player->get_framerate();
         if (player_framerate > 0)
            document()->set_framerate(get_framerate_from_value(player_framerate));
      }

      // Read subtitles
      Subtitles subtitles = document()->subtitles();

      int frame_start, frame_end;
      Glib::ustring line;
      Glib::ustring text;

      while (file.getline(line)) {
         if (!re->match(line))
            continue;

         std::vector<Glib::ustring> group = re->split(line);

         frame_start = utility::string_to_int(group[1]);
         frame_end = utility::string_to_int(group[2]);
         text = group[3];

         utility::replace(text, "|", "\n");

         text = tags->replace(text, 0, "<\\1>\\2</\\1>", (Glib::RegexMatchFlags)0);

         // Append a subtitle
         Subtitle sub = subtitles.append();

         sub.set_text(text);
         sub.set_start_frame(frame_start);
         sub.set_end_frame(frame_end);
      }
   }

   void save(Writer& file) {
      Glib::RefPtr<Glib::Regex> tags = Glib::Regex::create("<(b|i|u)>(.*?)</\\1>");

      for (Subtitle sub = document()->subtitles().get_first(); sub; ++sub) {
         Glib::ustring text = sub.get_text();

         utility::replace(text, "\n", "|");

         text = tags->replace(text, 0, "{y:\\1}\\2", (Glib::RegexMatchFlags)0);

         // {start_frame}{end_frame}text
         file.write(Glib::ustring::compose("{%1}{%2}%3\n", sub.get_start_frame(), sub.get_end_frame(), text));
      }
   }
};

class MicroDVDPlugin : public SubtitleFormat {
  public:
   SubtitleFormatInfo get_info() {
      SubtitleFormatInfo info;

      info.name = "MicroDVD";
      info.extension = "sub";
      info.pattern = "^\\{\\d+\\}\\{\\d+\\}.*?\\R";

      return info;
   }

   SubtitleFormatIO* create() {
      MicroDVD* sf = new MicroDVD();
      return sf;
   }
};

REGISTER_EXTENSION(MicroDVDPlugin)
