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

#include <i18n.h>

#include "errorchecking.h"

class MaxCharactersPerSecond : public ErrorChecking {
  public:
   MaxCharactersPerSecond()
       : ErrorChecking("max-characters-per-second",
                       _("Maximum Characters per Second"),
                       _("Detects and fixes subtitles when the number of characters per "
                         "second is superior to the specified value.")) {
      m_maxCPS = 25;
   }

   virtual void init() {
      m_maxCPS = cfg::get_double("timing", "max-characters-per-second");
   }

   bool execute(Info& info) {
      if ((info.currentSub.check_cps_text(0, m_maxCPS) <= 0) || m_maxCPS == 0)
         return false;

      SubtitleTime duration(utility::get_min_duration_msecs(info.currentSub.get_text(), m_maxCPS));

      if (info.tryToFix) {
         info.currentSub.set_duration(duration);
         return true;
      }

      info.error =
         build_message(_("There are too many characters per second: <b>%.1f chars/s</b>"), info.currentSub.get_characters_per_second_text());

      info.solution = build_message(_("<b>Automatic correction:</b> change "
                                      "current subtitle duration to %s."),
                                    duration.str().c_str());

      return true;
   }

  protected:
   double m_maxCPS;
};
