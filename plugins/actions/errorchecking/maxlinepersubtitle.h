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

class MaxLinePerSubtitle : public ErrorChecking {
 public:
  MaxLinePerSubtitle()
      : ErrorChecking(
            "max-line-per-subtitle", _("Maximum Lines per Subtitle"),
            _("An error is detected if a subtitle has too many lines.")) {
    m_maxLPS = 2;
  }

  virtual void init() {
    m_maxLPS = cfg::get_int("timing", "max-line-per-subtitle");
  }

  virtual bool execute(Info &info) {
    std::istringstream iss(info.currentSub.get_characters_per_line_text());
    std::string line;

    int count = 0;
    while (std::getline(iss, line)) {
      ++count;
    }

    if (count <= m_maxLPS)
      return false;

    if (info.tryToFix) {
      // not implemented
      return false;
    }

    info.error = build_message(
        ngettext("Subtitle has too many lines: <b>1 line</b>",
                 "Subtitle has too many lines: <b>%i lines</b>", count),
        count);
    info.solution =
        _("<b>Automatic correction:</b> unavailable, correct the error "
          "manually.");
    return true;
  }

 protected:
  int m_maxLPS;
};
