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

/*
// SBV format :

0:00:03.490,0:00:07.430
>> FISHER: All right. So, let's begin.
This session is: Going Social

0:00:07.430,0:00:11.600
with the YouTube APIs. I am
Jeff Fisher,

0:00:11.600,0:00:14.009
and this is Johann Hartmann,
we're presenting today.

0:00:14.009,0:00:15.889
[pause]
*/

class SBV : public SubtitleFormatIO {
  public:
   void open(Reader& file) {
      Glib::RefPtr<Glib::Regex> re_time = Glib::Regex::create("^(\\d):(\\d+):(\\d+)\\.(\\d+),(\\d):(\\d+):(\\d+)\\.(\\d+)");

      int start[4], end[4];
      Subtitles subtitles = document()->subtitles();

      Glib::ustring line;

      while (file.getline(line)) {
         // Read the subtitle times
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
            // Read the text lines
            while (file.getline(line) && !line.empty()) {
               if (!text.empty())
                  text += '\n';
               text += line;
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
      for (Subtitle sub = document()->subtitles().get_first(); sub; ++sub) {
         Glib::ustring text = sub.get_text();

         file.write(
            Glib::ustring::compose("%1,%2\n"
                                   "%3\n\n",
                                   time_to_sbv(sub.get_start()),
                                   time_to_sbv(sub.get_end()),
                                   text));
      }
   }

   Glib::ustring time_to_sbv(const SubtitleTime& t) {
      return build_message("%i:%02i:%02i.%03i", t.hours(), t.minutes(), t.seconds(), t.mseconds());
   }
};

class SBVPlugin : public SubtitleFormat {
  public:
   SubtitleFormatInfo get_info() {
      SubtitleFormatInfo info;
      info.name = "SBV";
      info.extension = "sbv";
      info.pattern = "\\d:\\d{2}:\\d{2}\\.\\d{3},\\d:\\d{2}:\\d{2}\\.\\d{3}\\R";

      return info;
   }

   SubtitleFormatIO* create() {
      SBV* sf = new SBV();
      return sf;
   }
};

REGISTER_EXTENSION(SBVPlugin)
