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

class MPL2 : public SubtitleFormatIO {
 public:
  void open(Reader &file) {
    Glib::RefPtr<Glib::Regex> re =
        Glib::Regex::create("^\\[(\\d+)\\]\\[(\\d+)\\](.*?)$");

    Subtitles subtitles = document()->subtitles();

    Glib::ustring line;
    long start, end;
    Glib::ustring text;
    int ds = 100;  // decaseconds (0,1 s)

    while (file.getline(line)) {
      if (!re->match(line))
        continue;

      std::vector<Glib::ustring> group = re->split(line);

      start = utility::string_to_int(group[1]);
      end = utility::string_to_int(group[2]);
      text = group[3];

      // Append a subtitle
      Subtitle sub = subtitles.append();

      utility::replace(text, "|", "\n");

      sub.set_text(text);
      sub.set_start(SubtitleTime(start * ds));
      sub.set_end(SubtitleTime(end * ds));
    }
  }

  void save(Writer &file) {
    Glib::ustring text;
    double ds = 100;

    // subtitles
    for (Subtitle sub = document()->subtitles().get_first(); sub; ++sub) {
      text = sub.get_text();

      utility::replace(text, "\n", "|");

      long start = (long)(sub.get_start().totalmsecs / ds);
      long end = (long)(sub.get_end().totalmsecs / ds);

      // [start][end]text
      file.write(Glib::ustring::compose("[%1][%2]%3\n", start, end, text));
    }
  }
};

class MPL2Plugin : public SubtitleFormat {
 public:
  SubtitleFormatInfo get_info() {
    SubtitleFormatInfo info;

    info.name = "MPL2";
    info.extension = "txt";
    info.pattern = "^\\[(\\d+)\\]\\[(\\d+)\\](.*?)\\R";

    return info;
  }

  SubtitleFormatIO *create() {
    MPL2 *sf = new MPL2();
    return sf;
  }
};

REGISTER_EXTENSION(MPL2Plugin)
