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
#include <map>
#include "cfg.h"
#include "statusbar.h"

class MenuBar : public Gtk::VBox {
 public:
  MenuBar();

  void create(Gtk::Window &window, Statusbar &statusbar);

  Glib::RefPtr<Gtk::UIManager> get_ui_manager();

 protected:
  void create_ui_from_file();

  // Use to show the tooltip in the statusbar.
  void connect_proxy(const Glib::RefPtr<Gtk::Action> &action,
                     Gtk::Widget *widget);

 protected:
  Statusbar *m_statusbar{nullptr};
  Glib::RefPtr<Gtk::UIManager> m_refUIManager;
};
