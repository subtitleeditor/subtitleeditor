// subtitleeditor -- a tool to create or edit subtitle
//
// https://subtitleeditor.github.io/subtitleeditor/
// https://github.com/subtitleeditor/subtitleeditor/
//
// Copyright @ 2005-2018, kitone
// Authors: eltomito <tomaspartl at centrum dot cz>
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
#include <gtkmm_utility.h>
#include <gui/dialogutility.h>
#include <player.h>
#include <subtitleeditorwindow.h>
#include <utility.h>

#include <memory>

// AvidDS (same as BITC, except actual newlines are used as newlines)
class AvidDS : public SubtitleFormatIO {
  public:
   AvidDS() : m_framerate(FRAMERATE_23_976) {
      m_framerate_value = get_framerate_value(m_framerate);
   }

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

      Glib::RefPtr<Glib::Regex> re_time = Glib::Regex::create("^(\\d+):(\\d+):(\\d+):(\\d+)\\s(\\d+):(\\d+):(\\d+):(\\d+)$");

      Subtitles subtitles = document()->subtitles();

      int start[4], end[4];
      Glib::ustring line;
      Glib::ustring text;

      Subtitle sub;

      while (file.getline(line)) {
         if (re_time->match(line)) {
            std::vector<Glib::ustring> group = re_time->split(line);

            start[0] = utility::string_to_int(group[1]);
            start[1] = utility::string_to_int(group[2]);
            start[2] = utility::string_to_int(group[3]);
            start[3] = utility::string_to_int(group[4]);

            end[0] = utility::string_to_int(group[5]);
            end[1] = utility::string_to_int(group[6]);
            end[2] = utility::string_to_int(group[7]);
            end[3] = utility::string_to_int(group[8]);

            // text
            int count = 0;
            text.clear();

            while (file.getline(line) && !line.empty()) {
               if (count > 0)
                  text += "\n";

               text += line;

               ++count;
            }

            // last 00 are frame, not time!
            start[3] = static_cast<int>(start[3] * 1000 / m_framerate_value);
            end[3] = static_cast<int>(end[3] * 1000 / m_framerate_value);

            // Append a subtitle
            sub = subtitles.append();

            sub.set_text(text);
            sub.set_start_and_end(SubtitleTime(start[0], start[1], start[2], start[3]), SubtitleTime(end[0], end[1], end[2], end[3]));
         }
      }
   }

   void save(Writer& file) {
      // Ask for the framerate value
      FramerateChooserDialog fcd(FramerateChooserDialog::EXPORT);
      fcd.set_default_framerate(document()->get_framerate());

      m_framerate_value = get_framerate_value(fcd.execute());

      // write header
      file.write(
         "@ File created by SubtitleEditor "
         "(https://subtitleeditor.github.io/subtitleeditor/)\n\n<begin subtitles>\n");

      for (Subtitle sub = document()->subtitles().get_first(); sub; ++sub) {
         Glib::ustring text = sub.get_text();

         // utility::replace(text, "\n", "|");

         file.write(Glib::ustring::compose("%1 %2\n%3\n\n", to_bitc_time(sub.get_start()), to_bitc_time(sub.get_end()), text));
      }

      // write footer
      file.write("<end subtitles>\n");
   }

   // Convert time from SE to BITC
   // 0:00:00.000 -> 00:00:00:00 (last 00 are frames, not time!)
   Glib::ustring to_bitc_time(const SubtitleTime& t) {
      int frame = static_cast<int>(t.mseconds() * m_framerate_value * 0.001);

      return build_message("%02i:%02i:%02i:%02i", t.hours(), t.minutes(), t.seconds(), frame);
   }

  protected:
   FRAMERATE m_framerate;
   double m_framerate_value;
};

class AvidDSPlugin : public SubtitleFormat {
  public:
   SubtitleFormatInfo get_info() {
      SubtitleFormatInfo info;

      info.name = "Avid DS";
      info.extension = "txt";
      info.pattern = "^<begin subtitles>$";
      return info;
   }

   SubtitleFormatIO* create() {
      return new AvidDS;
   }
};

REGISTER_EXTENSION(AvidDSPlugin)
