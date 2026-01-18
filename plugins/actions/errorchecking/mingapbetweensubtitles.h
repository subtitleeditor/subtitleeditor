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

class MinGapBetweenSubtitles : public ErrorChecking {
  public:
   MinGapBetweenSubtitles()
       : ErrorChecking("min-gap-between-subtitles",
                       _("Minimum Gap between Subtitles"),
                       _("Detects and fixes subtitles when the minimum gap "
                         "between subtitles is too short.")) {
      m_minGBS = 100;
   }

   virtual void init() {
      m_minGBS = cfg::get_int("timing", "min-gap-between-subtitles");
   }

   bool execute(Info& info) {
      if (!info.nextSub)
         return false;

      long gap = (info.nextSub.get_start() - info.currentSub.get_end()).totalmsecs;

      if (gap >= m_minGBS)
         return false;

      long middle = info.currentSub.get_end().totalmsecs + (gap / 2);
      long halfGBS = m_minGBS / 2;

      SubtitleTime new_current(middle - halfGBS);
      SubtitleTime new_next(middle + halfGBS);

      if (info.tryToFix) {
         info.currentSub.set_end(new_current);
         info.nextSub.set_start(new_next);

         return true;
      }

      // only error & solution
      info.error = build_message(_("Too short gap between subtitle: <b>%ims</b>"), gap);

      info.solution = build_message(_("<b>Automatic correction:</b> to clip current subtitle "
                                      "end to %s and to move next subtitle start to %s."),
                                    new_current.str().c_str(),
                                    new_next.str().c_str());

      return true;
   }

  protected:
   int m_minGBS;
};
