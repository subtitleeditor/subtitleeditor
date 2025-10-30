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

#include <gtkmm.h>
#include "cfg.h"
#include "document.h"
#include "extension.h"
#include "subtitleeditorwindow.h"

class Action : public Extension, public sigc::trackable {
 public:
  Action();

  virtual ~Action();

  virtual void activate();

  virtual void deactivate();

  virtual void update_ui();

  // static method

  static SubtitleEditorWindow* get_subtitleeditor_window();

  static Document* get_current_document();

  static Glib::RefPtr<Gtk::UIManager> get_ui_manager();
};
