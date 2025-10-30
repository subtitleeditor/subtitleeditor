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

class InterfacePage : public PreferencePage {
 public:
  InterfacePage(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& xml)
      : PreferencePage(cobject) {
    init_widget(xml, "check-use-dynamic-keyboard-shortcuts", "interface",
                "use-dynamic-keyboard-shortcuts");
    init_widget(xml, "check-maximize-window", "interface", "maximize-window");
    init_widget(xml, "check-ask-to-save-on-exit", "interface",
                "ask-to-save-on-exit");
    init_widget(xml, "spin-max-undo", "interface", "max-undo");

    init_widget(xml, "check-center-subtitle", "subtitle-view",
                "property-alignment-center");
    init_widget(xml, "check-show-character-per-line", "subtitle-view",
                "show-character-per-line");
    init_widget(xml, "check-enable-rubberband-selection", "subtitle-view",
                "enable-rubberband-selection");
    init_widget(xml, "check-used-ctrl-enter-to-confirm-change", "subtitle-view",
                "used-ctrl-enter-to-confirm-change");
    init_widget(xml, "check-enable-goto-subtitle-number", "subtitle-view",
                "enable-goto-subtitle-number");
    init_widget(xml, "check-do-not-disable-actions-during-editing",
                "subtitle-view", "do-not-disable-actions-during-editing");

    init_widget(xml, "check-create-backup-copy", "interface",
                "create-backup-copy");
    init_widget(xml, "check-autosave", "interface", "used-autosave");
    init_widget(xml, "spin-autosave", "interface", "autosave-minutes");
  }
};
