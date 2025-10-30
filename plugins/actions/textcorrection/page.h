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
#include <gtkmm_utility.h>

class AssistantPage : public Gtk::VBox {
 public:
  AssistantPage(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>&)
      : Gtk::VBox(cobject) {
  }

  AssistantPage() : Gtk::VBox(false, 6) {
    set_border_width(12);
  }

  virtual void prepare() {
  }

  virtual void apply(Document*) {
  }

  virtual void save_cfg() {
  }
};
