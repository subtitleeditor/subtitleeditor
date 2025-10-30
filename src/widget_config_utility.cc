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
#include "color.h"
#include "debug.h"
#include "widget_config_utility.h"

namespace widget_config {

void on_check_button(Gtk::CheckButton *widget, const Glib::ustring &group,
                     const Glib::ustring &key) {
  cfg::set_boolean(group, key, widget->get_active());
}

void on_font_button(Gtk::FontButton *widget, const Glib::ustring &group,
                    const Glib::ustring &key) {
  cfg::set_string(group, key, widget->get_font_name());
}

void on_color_button(Gtk::ColorButton *widget, const Glib::ustring &group,
                     const Glib::ustring &key) {
  Color color;
  color.getFromColorButton(*widget);

  cfg::set_string(group, key, color.to_string());
}

void on_range(Gtk::Range *range, const Glib::ustring &group,
              const Glib::ustring &key) {
  cfg::set_double(group, key, range->get_value());
}

void on_entry(Gtk::Entry *spin, const Glib::ustring &group,
              const Glib::ustring &key) {
  cfg::set_string(group, key, spin->get_text());
}

void on_spin_button(Gtk::SpinButton *spin, const Glib::ustring &group,
                    const Glib::ustring &key) {
  cfg::set_double(group, key, spin->get_value());
}

void on_combobox_text(Gtk::ComboBoxText *combo, const Glib::ustring &group,
                      const Glib::ustring &key) {
  cfg::set_string(group, key, combo->get_active_text());
}

void connect(Gtk::Widget *widget, const Glib::ustring &group,
             const Glib::ustring &key) {
  if (auto check = dynamic_cast<Gtk::CheckButton *>(widget)) {
    check->signal_toggled().connect(
        sigc::bind<Gtk::CheckButton *, Glib::ustring, Glib::ustring>(
            sigc::ptr_fun(&on_check_button), check, group, key));
  } else if (auto range = dynamic_cast<Gtk::Range *>(widget)) {
    range->signal_value_changed().connect(
        sigc::bind<Gtk::Range *, Glib::ustring, Glib::ustring>(
            sigc::ptr_fun(&on_range), range, group, key));
  } else if (auto spin = dynamic_cast<Gtk::SpinButton *>(widget)) {
    spin->signal_value_changed().connect(
        sigc::bind<Gtk::SpinButton *, Glib::ustring, Glib::ustring>(
            sigc::ptr_fun(&on_spin_button), spin, group, key));
  } else if (auto entry = dynamic_cast<Gtk::Entry *>(widget)) {
    entry->signal_changed().connect(
        sigc::bind<Gtk::Entry *, Glib::ustring, Glib::ustring>(
            sigc::ptr_fun(&on_entry), entry, group, key));
  } else if (auto font = dynamic_cast<Gtk::FontButton *>(widget)) {
    font->signal_font_set().connect(
        sigc::bind<Gtk::FontButton *, Glib::ustring, Glib::ustring>(
            sigc::ptr_fun(&on_font_button), font, group, key));
  } else if (auto color = dynamic_cast<Gtk::ColorButton *>(widget)) {
    color->signal_color_set().connect(
        sigc::bind<Gtk::ColorButton *, Glib::ustring, Glib::ustring>(
            sigc::ptr_fun(&on_color_button), color, group, key));
  } else if (auto combobox = dynamic_cast<Gtk::ComboBoxText *>(widget)) {
    combobox->signal_changed().connect(
        sigc::bind<Gtk::ComboBoxText *, Glib::ustring, Glib::ustring>(
            sigc::ptr_fun(&on_combobox_text), combobox, group, key));
  }
}

void read_config(Gtk::Widget *widget, const Glib::ustring &group,
                 const Glib::ustring &key) {
  if (auto check = dynamic_cast<Gtk::CheckButton *>(widget)) {
    check->set_active(cfg::get_boolean(group, key));
  } else if (auto range = dynamic_cast<Gtk::Range *>(widget)) {
    if (cfg::has_key(group, key)) {
      range->set_value(cfg::get_double(group, key));
    }
  } else if (auto spin = dynamic_cast<Gtk::SpinButton *>(widget)) {
    if (cfg::has_key(group, key)) {
      spin->set_value(cfg::get_double(group, key));
    }
  } else if (auto entry = dynamic_cast<Gtk::Entry *>(widget)) {
    if (cfg::has_key(group, key)) {
      entry->set_text(cfg::get_string(group, key));
    }
  } else if (auto font = dynamic_cast<Gtk::FontButton *>(widget)) {
    if (cfg::has_key(group, key)) {
      font->set_font_name(cfg::get_string(group, key));
    }
  } else if (auto colorbutton = dynamic_cast<Gtk::ColorButton *>(widget)) {
    Color color(cfg::get_string(group, key));

    color.initColorButton(*colorbutton);
  } else if (auto combobox = dynamic_cast<Gtk::ComboBoxText *>(widget)) {
    if (cfg::has_key(group, key)) {
      combobox->set_active_text(cfg::get_string(group, key));
    }
  }
}

void read_config_and_connect(Gtk::Widget *widget, const Glib::ustring &group,
                             const Glib::ustring &key) {
  g_return_if_fail(widget);

  read_config(widget, group, key);
  connect(widget, group, key);
}

}  // namespace widget_config
