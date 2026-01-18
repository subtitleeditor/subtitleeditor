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

#include <cstdio>

class MPsub : public SubtitleFormatIO {
  public:
   void open(Reader& file) {
      Glib::RefPtr<Glib::Regex> re = Glib::Regex::create("^(-?\\d+(?:\\.\\d+)?) (-?\\d+(?:\\.\\d+)?)\\s*$");

      Subtitles subtitles = document()->subtitles();

      Glib::ustring line;
      double previous_end = 0;

      TIMING_MODE mode = TIME;
      float framerate = 0;  // FORMAT=%f

      while (file.getline(line)) {
         if (re->match(line)) {
            std::vector<Glib::ustring> group = re->split(line);

            double dstart = utility::string_to_double(group[1]);
            double dduration = utility::string_to_double(group[2]);

            // Sets times
            double start_value = previous_end + dstart;
            double end_value = start_value + dduration;

            previous_end = end_value;

            // text
            int count = 0;
            Glib::ustring text;

            while (file.getline(line) && !line.empty()) {
               if (count > 0)
                  text += "\n";

               text += line;

               ++count;
            }

            // Append a subtitle
            Subtitle sub = subtitles.append();

            sub.set_text(text);
            if (mode == TIME) {
               sub.set_start(static_cast<long>(start_value * 1000));
               sub.set_end(static_cast<long>(end_value * 1000.0));
            } else {  // FRAME
               sub.set_start_frame((long)start_value);
               sub.set_end_frame((long)end_value);
            }
         } else if (std::sscanf(line.c_str(), "FORMAT=%f", &framerate) == 1) {
            // init to frame mode
            document()->set_timing_mode(FRAME);
            document()->set_edit_timing_mode(FRAME);
            document()->set_framerate(get_framerate_from_value(framerate));

            mode = FRAME;
         } else if (line.find("FORMAT=TIME") != Glib::ustring::npos) {
            mode = TIME;
         }
      }
   }

   void save(Writer& file) {
      // TODO: FRAME support
      // header
      file.write(
         Glib::ustring::compose("FORMAT=TIME\n"
                                "# This script was created by subtitleeditor (%1)\n"
                                "# https://subtitleeditor.github.io/subtitleeditor/\n"
                                "\n",
                                Glib::ustring(VERSION)));

      SubtitleTime start, end, previous_end;
      Glib::ustring text;

      // subtitles
      for (Subtitle sub = document()->subtitles().get_first(); sub; ++sub) {
         text = sub.get_text();

         start = sub.get_start();
         end = sub.get_end();

         double s = static_cast<double>((start - previous_end).totalmsecs) * 0.001;
         double d = static_cast<double>(sub.get_duration().totalmsecs) * 0.001;

         // start duration
         // text
         // (empty line)
         file.write(Glib::ustring::compose("%1 %2\n%3\n\n", s, d, text));

         previous_end = end;
      }
   }
};

class MPsubPlugin : public SubtitleFormat {
  public:
   SubtitleFormatInfo get_info() {
      SubtitleFormatInfo info;

      info.name = "MPsub";
      info.extension = "sub";
      info.pattern = "^FORMAT=(TIME|[0-9])";

      return info;
   }

   SubtitleFormatIO* create() {
      MPsub* sf = new MPsub();
      return sf;
   }
};

REGISTER_EXTENSION(MPsubPlugin)
