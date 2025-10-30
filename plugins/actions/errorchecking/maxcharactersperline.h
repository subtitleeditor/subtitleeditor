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

#include "errorchecking.h"

class MaxCharactersPerLine : public ErrorChecking {
 public:
  MaxCharactersPerLine()
      : ErrorChecking("max-characters-per-line",
                      _("Maximum Characters per Line"),
                      _("An error is detected if a line is too long.")) {
    m_maxCPL = 40;
  }

  virtual void init() {
    m_maxCPL = cfg::get_int("timing", "max-characters-per-line");
  }

  virtual bool execute(Info &info) {
    std::istringstream iss(info.currentSub.get_characters_per_line_text());
    std::string line;

    while (std::getline(iss, line)) {
      int number = utility::string_to_int(line);

      if (number > m_maxCPL) {
        if (info.tryToFix) {
          info.currentSub.set_text(
              word_wrap(info.currentSub.get_text(), m_maxCPL));
          return true;
        }

        info.error = build_message(
            ngettext("Subtitle has a too long line: <b>1 character</b>",
                     "Subtitle has a too long line: <b>%i characters</b>",
                     number),
            number);
        info.solution = build_message(
            _("<b>Automatic correction:</b>\n%s"),
            word_wrap(info.currentSub.get_text(), m_maxCPL).c_str());
        return true;
      }
    }

    return false;
  }

  Glib::ustring word_wrap(Glib::ustring str, Glib::ustring::size_type width) {
    Glib::ustring::size_type curWidth = width;
    Glib::ustring::size_type spacePos;
    while (curWidth < str.length()) {
      spacePos = str.rfind(' ', curWidth);
      if (spacePos == Glib::ustring::npos)
        spacePos = str.find(' ', curWidth);
      if (spacePos != Glib::ustring::npos) {
        str.replace(spacePos, 1, "\n");
        curWidth = spacePos + width + 1;
      } else break;
    }
    return str;
  }

 protected:
  int m_maxCPL;
};
