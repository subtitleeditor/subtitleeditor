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

#include "preferencepage.h"

class TimingPage : public PreferencePage {
  public:
   TimingPage(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& xml) : PreferencePage(cobject) {
      init_widget(xml, "spin-min-characters-per-second", "timing", "min-characters-per-second");
      init_widget(xml, "spin-max-characters-per-second", "timing", "max-characters-per-second");
      init_widget(xml, "spin-ideal-characters-per-second", "timing", "ideal-characters-per-second");
      init_widget(xml, "spin-min-gap-between-subtitles", "timing", "min-gap-between-subtitles");
      init_widget(xml, "spin-min-display", "timing", "min-display");
      init_widget(xml, "spin-max-characters-per-line", "timing", "max-characters-per-line");
      init_widget(xml, "spin-max-line-per-subtitle", "timing", "max-line-per-subtitle");
      init_widget(xml, "check-ignore-space", "timing", "ignore-space");
      init_widget(xml, "check-do-auto-timing-check", "timing", "do-auto-timing-check");
   }
};
