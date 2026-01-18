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

#include "stylemodel.h"

class Document;

class Style {
   friend class Styles;

  public:
   Style();
   Style(Document* doc, const Gtk::TreeIter& iter);
   ~Style();

   operator bool() const;

   Style& operator++();

   Style& operator--();

   bool operator==(const Style& style) const;

   bool operator!=(const Style& style) const;

   void set(const Glib::ustring& name, const Glib::ustring& value);

   Glib::ustring get(const Glib::ustring& name) const;

   void set(const std::map<Glib::ustring, Glib::ustring>& values);

   void get(std::map<Glib::ustring, Glib::ustring>& values);

   void copy_to(Style& style);

  protected:
   static StyleColumnRecorder column;
   Document* m_document{nullptr};
   Gtk::TreeIter m_iter;
};
