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

#include "styles.h"

#include "document.h"
#include "utility.h"

Styles::Styles(Document& doc) : m_document(doc) {
}

Styles::~Styles() {
}

unsigned int Styles::size() {
   return m_document.get_style_model()->children().size();
}

Style Styles::get(unsigned int num) {
   Gtk::TreeIter iter = m_document.get_style_model()->get_iter(to_string(num));
   return Style(&m_document, iter);
}

Style Styles::first() {
   return Style(&m_document, m_document.get_style_model()->children().begin());
}

Style Styles::last() {
   Gtk::TreeNodeChildren rows = m_document.get_style_model()->children();
   if (!rows.empty()) {
#warning "VERIFIER ça -1"
      return Style(&m_document, rows[rows.size() - 1]);
   }
   return Style();
}

Style Styles::append() {
   Style style(&m_document, m_document.get_style_model()->append());
   m_document.emit_signal("style-inserted");
   return style;
}

void Styles::remove(const Style& style) {
   m_document.get_style_model()->erase(style.m_iter);
   m_document.emit_signal("style-removed");
}
