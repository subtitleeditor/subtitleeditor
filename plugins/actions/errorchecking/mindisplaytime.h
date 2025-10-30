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

class MinDisplayTime : public ErrorChecking {
 public:
  MinDisplayTime()
      : ErrorChecking("min-display-time", _("Minimum Display Time"),
                      _("Detects and fixes subtitles when the duration is "
                        "inferior to the specified value.")) {
    m_min_display = 1000;  // a second
  }

  virtual void init() {
    m_min_display = cfg::get_int("timing", "min-display");
  }

  bool execute(Info &info) {
    SubtitleTime duration = info.currentSub.get_duration();

    if (duration.totalmsecs >= m_min_display)
      return false;

    SubtitleTime new_end =
        info.currentSub.get_start() + SubtitleTime(m_min_display);

    if (info.tryToFix) {
      info.currentSub.set_end(new_end);
      return true;
    }

    info.error =
        build_message(_("Subtitle display time is too short: <b>%s</b>"),
                      duration.str().c_str());

    info.solution = build_message(
        _("<b>Automatic correction:</b> to change current subtitle end to %s."),
        new_end.str().c_str());

    return true;
  }

 protected:
  int m_min_display;
};
