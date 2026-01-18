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
#include <gui/dialogutility.h>
#include <player.h>
#include <subtitleeditorwindow.h>
#include <utility.h>

class SpruceSTL : public SubtitleFormatIO {
   double m_framerate_value;

  public:
   void open(Reader& file) {
      // Ask for the framerate value
      FramerateChooserDialog fcd(FramerateChooserDialog::IMPORT);

      // Define the default value of the framerate from the player
      Player* player = SubtitleEditorWindow::get_instance()->get_player();
      if (player->get_state() != Player::NONE) {
         float player_framerate = player->get_framerate();
         if (player_framerate > 0)
            fcd.set_default_framerate(get_framerate_from_value(player_framerate));
      }
      FRAMERATE framerate = fcd.execute();
      m_framerate_value = get_framerate_value(framerate);

      document()->set_framerate(framerate);

      Glib::RefPtr<Glib::Regex> re = Glib::Regex::create(
         "^(\\d+):(\\d+):(\\d+):(\\d+)\\s,\\s(\\d+):(\\d+):(\\d+):(\\d+)\\s,\\s+"
         "(.*?)$");

      int start[4], end[4];
      Subtitles subtitles = document()->subtitles();

      Glib::ustring line, text;
      while (file.getline(line)) {
         if (re->match(line)) {
            std::vector<Glib::ustring> group = re->split(line);

            start[0] = utility::string_to_int(group[1]);
            start[1] = utility::string_to_int(group[2]);
            start[2] = utility::string_to_int(group[3]);
            start[3] = utility::string_to_int(group[4]);

            end[0] = utility::string_to_int(group[5]);
            end[1] = utility::string_to_int(group[6]);
            end[2] = utility::string_to_int(group[7]);
            end[3] = utility::string_to_int(group[8]);

            // last value are frame, not time
            start[3] = static_cast<int>(start[3] * 1000 / m_framerate_value);
            end[3] = static_cast<int>(end[3] * 1000 / m_framerate_value);

            text = group[9];
            utility::replace(text, "|", "\n");

            // Append a subtitle
            Subtitle sub = subtitles.append();

            sub.set_text(text);
            sub.set_start_and_end(SubtitleTime(start[0], start[1], start[2], start[3]), SubtitleTime(end[0], end[1], end[2], end[3]));
         } else {
            se_dbg_msg(SE_DBG_PLUGINS, "can not match time line: '%s'", line.c_str());
         }
      }
   }

   void save(Writer& file) {
      // Ask for the framerate value
      FramerateChooserDialog fcd(FramerateChooserDialog::EXPORT);
      fcd.set_default_framerate(document()->get_framerate());

      m_framerate_value = get_framerate_value(fcd.execute());

      for (Subtitle sub = document()->subtitles().get_first(); sub; ++sub) {
         Glib::ustring text = sub.get_text();

         utility::replace(text, "\n", "|");

         file.write(Glib::ustring::compose("%1 , %2 , %3\n", to_sprucestl_time(sub.get_start()), to_sprucestl_time(sub.get_end()), text));
      }
   }

   Glib::ustring to_sprucestl_time(const SubtitleTime& t) {
      int frame = static_cast<int>(t.mseconds() * m_framerate_value * 0.001);

      return build_message("%02i:%02i:%02i:%02i", t.hours(), t.minutes(), t.seconds(), frame);
   }
};

class SpruceSTLPlugin : public SubtitleFormat {
  public:
   SubtitleFormatInfo get_info() {
      SubtitleFormatInfo info;
      info.name = "Spruce STL";
      info.extension = "stl";
      info.pattern =
         "\\d\\d:\\d\\d:\\d\\d:\\d\\d"
         "\\s,\\s"
         "\\d\\d:\\d\\d:\\d\\d:\\d\\d"
         "\\s,\\s+"
         ".*?\\R";
      return info;
   }

   SubtitleFormatIO* create() {
      SpruceSTL* sf = new SpruceSTL();
      return sf;
   }
};

REGISTER_EXTENSION(SpruceSTLPlugin)
