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
#include <utility.h>

// format:
// number
// start --> end
// text
// (empty line)
class SubRip : public SubtitleFormatIO {
  public:
   void open(Reader& file) {
      Glib::RefPtr<Glib::Regex> re_num = Glib::Regex::create("^\\d+$");

      Glib::RefPtr<Glib::Regex> re_time = Glib::Regex::create("^(\\d+):(\\d+):(\\d+),(\\d+)\\s-->\\s(\\d+):(\\d+):(\\d+),(\\d+)");

      int start[4], end[4];
      Subtitles subtitles = document()->subtitles();

      Glib::ustring line;

      while (file.getline(line)) {
         // Read the subtitle time "start --> end"
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

            Glib::ustring text;
            int count = 0;

            // Read the text lines
            while (file.getline(line) && !line.empty()) {
               if (count > 0)
                  text += '\n';

               text += line;

               ++count;
            }

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
      unsigned int count = 1;
      for (Subtitle sub = document()->subtitles().get_first(); sub; ++sub, ++count) {
         Glib::ustring text = sub.get_text();

         file.write(Glib::ustring::compose("%1\n%2 --> %3\n%4\n\n", count, time_to_subrip(sub.get_start()), time_to_subrip(sub.get_end()), text));
      }
   }

   Glib::ustring time_to_subrip(const SubtitleTime& t) {
      return build_message("%02i:%02i:%02i,%03i", t.hours(), t.minutes(), t.seconds(), t.mseconds());
   }
};

class SubRipPlugin : public SubtitleFormat {
  public:
   SubtitleFormatInfo get_info() {
      SubtitleFormatInfo info;
      info.name = "SubRip";
      info.extension = "srt";

      info.pattern =
         "\\d\\R"
         "\\d+:\\d+:\\d+,\\d+\\s-->\\s\\d+:\\d+:\\d+,\\d+"
         "(\\sX1:\\d+ X2:\\d+ Y1:\\d+ Y2:\\d+)?\\s*"  // Be cool with
                                                      // coordinates, this is not
                                                      // documented
         "\\R";

      return info;
   }

   SubtitleFormatIO* create() {
      SubRip* sf = new SubRip();
      return sf;
   }
};

REGISTER_EXTENSION(SubRipPlugin)
