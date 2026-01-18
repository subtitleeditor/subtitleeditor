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

#include "statusbar.h"

#include <iostream>

Statusbar::Statusbar(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& /*builder*/) : Gtk::Statusbar(cobject) {
}

Statusbar::~Statusbar() {
   if (m_connection_timeout)
      m_connection_timeout.disconnect();
}

void Statusbar::push_text(const Glib::ustring& text) {
   if (m_connection_timeout)
      m_connection_timeout.disconnect();

   pop_text();

   push(text);
}

void Statusbar::pop_text() {
   if (m_connection_timeout)
      m_connection_timeout.disconnect();

   pop();
}

// affiche un message pendant 3 sec
void Statusbar::flash_message(const Glib::ustring& text) {
   if (m_connection_timeout)
      m_connection_timeout.disconnect();

   push_text(text);

   m_connection_timeout = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Statusbar::on_timeout), 3000);
}

bool Statusbar::on_timeout() {
   pop_text();
   m_connection_timeout.disconnect();
   return false;
}
