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

class Overlapping : public ErrorChecking {
 public:
  Overlapping()
      : ErrorChecking("overlapping", _("Overlapping"),
                      _("An error is detected when the subtitle overlap on "
                        "next subtitle.")) {
  }

  virtual void init() {
    // mode = number
  }

  // Check if the currentSub overlap on the next.
  bool execute(Info &info) {
    if (!info.nextSub)
      return false;

    if (info.currentSub.get_end() <= info.nextSub.get_start())
      return false;

    long overlap =
        (info.currentSub.get_end() - info.nextSub.get_start()).totalmsecs;

    if (info.tryToFix) {
      // not implemented
      return false;
    }

    info.error = build_message(
        _("Subtitle overlap on next subtitle: <b>%ims overlap</b>"), overlap);

    info.solution =
        _("<b>Automatic correction:</b> unavailable, correct the error "
          "manually.");

    return true;
  }
};
