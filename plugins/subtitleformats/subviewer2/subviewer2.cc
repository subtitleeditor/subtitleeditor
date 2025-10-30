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

class SubViewer2 : public SubtitleFormatIO {
 public:
  void open(Reader &file) {
    Glib::RefPtr<Glib::Regex> re_time = Glib::Regex::create(
        "^(\\d+):(\\d+):(\\d+)\\.(\\d+),(\\d+):(\\d+):(\\d+)\\.(\\d+)");

    Subtitles subtitles = document()->subtitles();

    Glib::ustring line;
    int start[4], end[4];

    while (file.getline(line)) {
      // Read the subtitle time "start --> end"
      if (!re_time->match(line))
        continue;

      std::vector<Glib::ustring> group = re_time->split(line);

      start[0] = utility::string_to_int(group[1]);
      start[1] = utility::string_to_int(group[2]);
      start[2] = utility::string_to_int(group[3]);
      start[3] = utility::string_to_int(group[4]);

      end[0] = utility::string_to_int(group[5]);
      end[1] = utility::string_to_int(group[6]);
      end[2] = utility::string_to_int(group[7]);
      end[3] = utility::string_to_int(group[8]);

      if (file.getline(line)) {
        utility::replace(line, "[br]", "\n");

        // Append a subtitle
        Subtitle sub = subtitles.append();

        sub.set_text(line);
        sub.set_start_and_end(
            SubtitleTime(start[0], start[1], start[2], start[3] * 10),
            SubtitleTime(end[0], end[1], end[2], end[3] * 10));
      }
    }
  }

  void save(Writer &file) {
    ScriptInfo &script = document()->get_script_info();

    file.write(Glib::ustring::compose(
        "[INFORMATION]\n"
        "[TITLE]%1\n"
        "[AUTHOR]%2\n"
        "[COMMENT]%3\n"
        "[END INFORMATION]\n"
        "[SUBTITLE]\n",
        script.data["Title"], script.data["OriginalEditing"],
        script.data["Comment"]));

    for (Subtitle sub = document()->subtitles().get_first(); sub; ++sub) {
      Glib::ustring text = sub.get_text();

      utility::replace(text, "\n", "[br]");

      file.write(Glib::ustring::compose(
          "%1,%2\n%3\n\n", to_subviewer_time(sub.get_start()),
          to_subviewer_time(sub.get_end()), text));
    }
  }

  Glib::ustring to_subviewer_time(const SubtitleTime &t) {
    auto hundredths = static_cast<int>((t.mseconds() + 5) / 10);
    return build_message("%02i:%02i:%02i.%02i", t.hours(), t.minutes(),
                         t.seconds(), hundredths);
  }
};

class SubViewer2Plugin : public SubtitleFormat {
 public:
  SubtitleFormatInfo get_info() {
    SubtitleFormatInfo info;
    info.name = "SubViewer 2.0";
    info.extension = "sub";

    info.pattern = "\\d{2}:\\d{2}:\\d{2}.\\d+,\\d{2}:\\d{2}:\\d{2}.\\d+\\s*\\R";

    return info;
  }

  SubtitleFormatIO *create() {
    SubViewer2 *sf = new SubViewer2();
    return sf;
  }
};

REGISTER_EXTENSION(SubViewer2Plugin)
