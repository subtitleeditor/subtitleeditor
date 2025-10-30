#pragma once

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

class AdobeEncoreDVD : public SubtitleFormatIO {
 public:
  explicit AdobeEncoreDVD(FRAMERATE framerate) : m_framerate(framerate) {
    m_framerate_value = get_framerate_value(m_framerate);
  }

  void open(Reader &file) {
    Glib::RefPtr<Glib::Regex> re = Glib::Regex::create(
        "\\d+\\s(\\d+)[:;](\\d+)[:;](\\d+)[:;](\\d+)\\s(\\d+)[:;](\\d+)[:;]("
        "\\d+)[:;](\\d+)\\s(.*?)$");

    Subtitles subtitles = document()->subtitles();

    int start[4], end[4];
    Glib::ustring line;
    Glib::ustring text;

    Subtitle sub;

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

        text = group[9];

        // last 00 are frame, not time!
        start[3] = static_cast<int>(start[3] * 1000 / m_framerate_value);
        end[3] = static_cast<int>(end[3] * 1000 / m_framerate_value);

        // Append a subtitle
        sub = subtitles.append();

        sub.set_text(text);
        sub.set_start_and_end(
            SubtitleTime(start[0], start[1], start[2], start[3]),
            SubtitleTime(end[0], end[1], end[2], end[3]));
      } else if (sub) {
        // this is another line of the previous subtitle
        sub.set_text(sub.get_text() + "\n" + line);
      }
    }
  }

  void save(Writer &file) {
    for (Subtitle sub = document()->subtitles().get_first(); sub; ++sub) {
      Glib::ustring text = sub.get_text();

      file.write(Glib::ustring::compose(
          "%1 %2 %3 %4\n", sub.get_num(), to_encore_dvd_time(sub.get_start()),
          to_encore_dvd_time(sub.get_end()), text));
    }
  }

  // Convert time from SE to Encore DVD
  // 0:00:00.000 -> 00[:;]00[:;]00[:;]00 (last 00 are frames, not time!)
  Glib::ustring to_encore_dvd_time(const SubtitleTime &t) {
    int frame = static_cast<int>(t.mseconds() * m_framerate_value * 0.001);

    return build_message((m_framerate == FRAMERATE_25) ? "%02i:%02i:%02i:%02i"
                                                       : "%02i;%02i;%02i;%02i",
                         t.hours(), t.minutes(), t.seconds(), frame);
  }

 protected:
  FRAMERATE m_framerate;
  double m_framerate_value;
};
