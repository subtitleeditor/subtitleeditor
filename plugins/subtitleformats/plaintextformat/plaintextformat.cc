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

// format: blank lines separate subtitles or every line is a new subtitle
// depending on the preferences set in:
// plain-text/export-bl-between-subtitles
// plain-text/import-bl-between-subtitles
class PlainTextFormat : public SubtitleFormatIO {
 public:
  void open(Reader &file) {
    Subtitles subtitles = document()->subtitles();
    Glib::ustring line;
    bool usebl = cfg::get_boolean("plain-text", "import-bl-between-subtitles");

    if (!usebl) {
      // ignore blank lines
      while (file.getline(line)) {
        Subtitle sub = subtitles.append();
        sub.set_text(line);
      }
    } else {
      // separate subtitles at blank lines
      Glib::ustring subtext;
      subtext.clear();
      int textlines = 0;

      while (file.getline(line)) {
        if (line.empty()) {
          if (textlines > 0) {
            Subtitle sub = subtitles.append();
            sub.set_text(subtext);
            subtext.clear();
            textlines = 0;
          }
        } else {
          if (textlines > 0)
            subtext += "\n";
          subtext += line;
          textlines++;
        }
      }

      // if the file didn't end with a blank line, we need to append leftover
      // lines as one more subtitle
      if (textlines > 0) {
        Subtitle sub = subtitles.append();
        sub.set_text(subtext);
        subtext.clear();
      }
    }  // separate with blank lines
  }

  void save(Writer &file) {
    Document *doc = document();
    bool usebl = cfg::get_boolean("plain-text", "export-bl-between-subtitles");

    // how many subtitles does this document have?
    int subcnt = doc->subtitles().size();
    if (subcnt <= 0)  // no subtitles, nothing to do
      return;

    // initialize the output loop
    subcnt--;  // output all subtitles except the last one.
    int i = 0;
    Subtitle sub = doc->subtitles().get_first();

    while (i < subcnt) {
      file.write(sub.get_text() + "\n");
      if (usebl)
        file.write("\n");
      ++sub;
      i++;
    }

    // Now, output the last subtitle with no blank line appended.
    file.write(sub.get_text() + "\n");
  }
};

class PlainTextFormatPlugin : public SubtitleFormat {
 public:
  SubtitleFormatInfo get_info() {
    SubtitleFormatInfo info;
    info.name = "Plain Text Format";
    info.extension = "txt";

    // The Plaint Text Format can import any text file regardless of its
    // contents, so the actual pattern would be ".*". But then it would steal
    // all subtitle files, such as .srt, .mpsub, etc, from their correct format
    // interpreters and digest them all as plain text, which would be wrong. For
    // that reason, it must never identify any file as its own and let the more
    // picky subtitle formats decide if they want to process a file or not.
    info.pattern = "nEvEr MaTcH a PlAiN-texT fILe autOmatIcallY";

    return info;
  }

  SubtitleFormatIO *create() {
    PlainTextFormat *sf = new PlainTextFormat();
    return sf;
  }
};

REGISTER_EXTENSION(PlainTextFormatPlugin)
