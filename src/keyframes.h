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

#include <glibmm.h>

#include <vector>

class KeyFrames : public std::vector<long> {
  public:
   static Glib::RefPtr<KeyFrames> create_from_file(const Glib::ustring& uri);

   bool open(const Glib::ustring& uri);

   bool save(const Glib::ustring& uri);

   void set_uri(const Glib::ustring& uri);

   Glib::ustring get_uri() const;

   void set_video_uri(const Glib::ustring& uri);

   Glib::ustring get_video_uri() const;

  public:
   void reference() const;

   void unreference() const;

   KeyFrames();

   ~KeyFrames();

  protected:
   mutable int ref_count_{0};
   Glib::ustring m_uri;
   Glib::ustring m_video_uri;
};
